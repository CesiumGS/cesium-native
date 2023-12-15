#include "Cesium3DTilesSelection/TileWorkManager.h"

#include "CesiumAsync/IAssetResponse.h"

using namespace CesiumAsync;

namespace Cesium3DTilesSelection {

TileWorkManager::~TileWorkManager() noexcept {
  {
    std::lock_guard<std::mutex> lock(_requestsLock);
    _exitSignaled = true;

    // TODO, we can crash here if there are still requests in flight
  }
}

void TileWorkManager::QueueRequestWork(
    const std::vector<TileLoadWork>& work,
    const std::vector<TileLoadWork>& passThroughWork,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders,
    size_t maxSimultaneousRequests) {
  if (work.empty() && passThroughWork.empty())
    return;

  {
    std::lock_guard<std::mutex> lock(_requestsLock);
    _queuedWork.insert(_queuedWork.end(), work.begin(), work.end());

    _doneWork.insert(
        _doneWork.end(),
        passThroughWork.begin(),
        passThroughWork.end());

    _requestHeaders = requestHeaders;

    assert(maxSimultaneousRequests > 0);
    _maxSimultaneousRequests = maxSimultaneousRequests;
  }

  transitionQueuedWork();
}

void TileWorkManager::PassThroughWork(const std::vector<TileLoadWork>& work) {
  if (work.empty())
    return;
  std::lock_guard<std::mutex> lock(_requestsLock);
  _doneWork.insert(_doneWork.end(), work.begin(), work.end());
}

void TileWorkManager::onRequestFinished(
    uint16_t responseStatusCode,
    gsl::span<const std::byte> responseBytes,
    const TileLoadWork& request) {
  std::lock_guard<std::mutex> lock(_requestsLock);

  if (_exitSignaled)
    return;

  // Find this request
  std::map<std::string, std::vector<TileLoadWork>>::iterator foundIt;
  foundIt = _inFlightWork.find(request.requestUrl);
  assert(foundIt != _inFlightWork.end());

  // Handle results
  std::vector<TileLoadWork>& requestWorkVec = foundIt->second;
  for (TileLoadWork& requestWork : requestWorkVec) {

    if (responseStatusCode == 0) {
      // A response code of 0 is not a valid HTTP code
      // and probably indicates a non-network error.
      // Put this work in a failed queue to be handled later
      _failedWork.push_back(std::move(requestWork));
      continue;
    }

    // Add new entry
    assert(
        requestWork.responsesByUrl.find(requestWork.requestUrl) ==
        requestWork.responsesByUrl.end());
    ResponseData& responseData =
        requestWork.responsesByUrl[requestWork.requestUrl];

    // Copy our results
    size_t byteCount = responseBytes.size();
    if (byteCount > 0) {
      responseData.bytes.resize(byteCount);
      std::copy(
          responseBytes.begin(),
          responseBytes.end(),
          responseData.bytes.begin());
    }
    responseData.statusCode = responseStatusCode;

    // Put in done requests
    _doneWork.push_back(std::move(requestWork));
  }

  // Remove it
  _inFlightWork.erase(foundIt);
}

void TileWorkManager::dispatchRequest(TileLoadWork& request) {
  this->_pAssetAccessor
      ->get(this->_asyncSystem, request.requestUrl, this->_requestHeaders)
      .thenImmediately([_this = this, _request = request](
                           std::shared_ptr<IAssetRequest>&& pCompletedRequest) {
        // Add payload to this work
        const IAssetResponse* pResponse = pCompletedRequest->response();
        if (pResponse)
          _this->onRequestFinished(
              pResponse->statusCode(),
              pResponse->data(),
              _request);
        else
          _this->onRequestFinished(0, gsl::span<const std::byte>(), _request);

        _this->transitionQueuedWork();

        return pResponse != NULL;
      });
}

void TileWorkManager::stageQueuedWork(
    std::vector<TileLoadWork>& workNeedingDispatch) {
  // Take from back of queue (highest priority).
  assert(_queuedWork.size() > 0);
  TileLoadWork request = _queuedWork.back();
  _queuedWork.pop_back();

  // Move to in flight registry
  std::map<std::string, std::vector<TileLoadWork>>::iterator foundIt;
  foundIt = _inFlightWork.find(request.requestUrl);
  if (foundIt == _inFlightWork.end()) {
    // Request doesn't exist, set up a new one
    std::vector<TileLoadWork> newWorkVec;
    newWorkVec.push_back(request);
    _inFlightWork[request.requestUrl] = newWorkVec;

    // Copy to our output vector
    workNeedingDispatch.push_back(request);
  } else {
    // Tag on to an existing request. Don't bother staging it. Already is.
    foundIt->second.push_back(request);
  }
}

size_t TileWorkManager::GetPendingRequestsCount() {
  std::lock_guard<std::mutex> lock(_requestsLock);
  return _queuedWork.size() + _inFlightWork.size();
}

size_t TileWorkManager::GetTotalPendingCount() {
  std::lock_guard<std::mutex> lock(_requestsLock);
  return _queuedWork.size() + _inFlightWork.size() + _doneWork.size() +
         _failedWork.size();
}

void TileWorkManager::GetRequestsStats(
    size_t& queued,
    size_t& inFlight,
    size_t& done) {
  std::lock_guard<std::mutex> lock(_requestsLock);
  queued = _queuedWork.size();
  inFlight = _inFlightWork.size();
  done = _doneWork.size() + _failedWork.size();
}

void TileWorkManager::TakeCompletedWork(
    size_t maxCount,
    std::vector<TileLoadWork>& outCompleted,
    std::vector<TileLoadWork>& outFailed) {
  std::lock_guard<std::mutex> lock(_requestsLock);

  // All failed requests go out
  if (_failedWork.empty()) {
    outFailed = _failedWork;
    _failedWork.clear();
  }

  // Return completed work, up to the count specified
  size_t doneCount = _doneWork.size();
  if (doneCount == 0)
    return;

  size_t numberToTake = std::min(doneCount, maxCount);

  // If not taking everything, sort so more important work goes first
  if (numberToTake < doneCount)
    std::sort(_doneWork.begin(), _doneWork.end());

  // Move work to output
  for (auto workIt = _doneWork.begin();
       workIt != _doneWork.begin() + numberToTake;
       ++workIt)
    outCompleted.push_back(std::move(*workIt));

  // Remove these entries from the source
  _doneWork.erase(_doneWork.begin(), _doneWork.begin() + numberToTake);
}

void TileWorkManager::transitionQueuedWork() {
  std::vector<TileLoadWork> workNeedingDispatch;
  {
    std::lock_guard<std::mutex> lock(_requestsLock);

    size_t queueCount = _queuedWork.size();
    if (queueCount > 0) {
      // We have work to do

      size_t slotsTotal = _maxSimultaneousRequests;
      size_t slotsUsed = _inFlightWork.size();
      if (slotsUsed < slotsTotal) {
        // There are free slots
        size_t slotsAvailable = slotsTotal - slotsUsed;

        // Sort our incoming request queue by priority
        // Sort descending so highest priority is at back of vector
        if (queueCount > 1)
          std::sort(_queuedWork.rbegin(), _queuedWork.rend());

        // Stage amount of work specified by caller, or whatever is left
        size_t dispatchCount = std::min(queueCount, slotsAvailable);

        for (size_t index = 0; index < dispatchCount; ++index)
          stageQueuedWork(workNeedingDispatch);
      }
    }
  }

  for (TileLoadWork& requestWork : workNeedingDispatch)
    dispatchRequest(requestWork);
}

} // namespace Cesium3DTilesSelection
