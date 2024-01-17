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

void TileWorkManager::SetMaxSimultaneousRequests(size_t max) {
  std::lock_guard<std::mutex> lock(_requestsLock);
  _maxSimultaneousRequests = max;
}

bool TileWorkManager::isProcessingUnique(
    const TileLoadWork& newRequest,
    const TileLoadWork& existingRequest) {
  if (std::holds_alternative<TileProcessingData>(newRequest.processingData) &&
      std::holds_alternative<TileProcessingData>(
          existingRequest.processingData)) {
    TileProcessingData newTileProcessing =
        std::get<TileProcessingData>(newRequest.processingData);
    TileProcessingData existingTileProcessing =
        std::get<TileProcessingData>(existingRequest.processingData);
    return newTileProcessing.pTile != existingTileProcessing.pTile;
  }

  if (std::holds_alternative<RasterProcessingData>(newRequest.processingData) &&
      std::holds_alternative<RasterProcessingData>(
          existingRequest.processingData)) {
    RasterProcessingData newTileProcessing =
        std::get<RasterProcessingData>(newRequest.processingData);
    RasterProcessingData existingTileProcessing =
        std::get<RasterProcessingData>(existingRequest.processingData);
    return newTileProcessing.pRasterTile != existingTileProcessing.pRasterTile;
  }

  // Processing data types are different
  return true;
}

bool TileWorkManager::isWorkAlreadyProcessing(
    const TileLoadWork& newProcessing) {
  for (auto doneRequest : _processingQueue) {
    if (!isProcessingUnique(newProcessing, doneRequest))
      return true;
  }
  return false;
}

bool TileWorkManager::isRequestAlreadyInFlight(const TileLoadWork& newRequest) {
  for (auto urlWorkPair : _inFlightRequests) {
    for (auto work : urlWorkPair.second) {
      if (newRequest.requestData.url != work.requestData.url)
        continue;

      // Urls do match. Do they point to the same tile?
      if (!isProcessingUnique(newRequest, work))
        return true;
    }
  }
  return false;
}

bool TileWorkManager::isRequestAlreadyQueued(const TileLoadWork& newRequest) {
  for (auto existingRequest : _requestQueue) {
    if (newRequest.requestData.url != existingRequest.requestData.url)
      continue;

    // Urls do match. Do they point to the same tile?
    if (!isProcessingUnique(newRequest, existingRequest))
      return true;
  }
  return false;
}

void TileWorkManager::QueueBatch(
    const std::vector<TileLoadWork*>& requestWork,
    const std::vector<TileLoadWork*>& processingWork) {
  if (requestWork.empty() && processingWork.empty())
    return;

  {
    std::lock_guard<std::mutex> lock(_requestsLock);

    for (TileLoadWork* element : requestWork) {
      assert(!isRequestAlreadyQueued(*element));
      assert(!isRequestAlreadyInFlight(*element));
      _requestQueue.push_back(*element);
    }

    for (TileLoadWork* element : processingWork) {
      assert(!isWorkAlreadyProcessing(*element));
      _processingQueue.push_back(*element);
    }
  }

  transitionQueuedWork();
}

void TileWorkManager::QueueSingleRequest(const TileLoadWork& requestWork) {
  {
    std::lock_guard<std::mutex> lock(_requestsLock);

    assert(!isRequestAlreadyQueued(requestWork));
    assert(!isRequestAlreadyInFlight(requestWork));
    _requestQueue.push_back(std::move(requestWork));
  }

  transitionQueuedWork();
}

void TileWorkManager::eraseMatchingChildWork(
    const TileLoadWork& work,
    std::vector<TileLoadWork>& childWork) {
  std::vector<TileLoadWork>::iterator childIt;
  for (childIt = childWork.begin(); childIt != childWork.end(); ++childIt) {
    bool baseWorkEqual = childIt->childWork.size() == work.childWork.size() &&
                         childIt->group == work.group &&
                         childIt->priority == work.priority;
    bool workHasTileProcessing =
        std::holds_alternative<TileProcessingData>(work.processingData);
    bool childHasTileProcessing =
        std::holds_alternative<TileProcessingData>(childIt->processingData);

    if (!baseWorkEqual || workHasTileProcessing != childHasTileProcessing)
      continue;

    if (workHasTileProcessing) {
      TileProcessingData workTileProcessing =
          std::get<TileProcessingData>(work.processingData);
      TileProcessingData childTileProcessing =
          std::get<TileProcessingData>(childIt->processingData);
      if (workTileProcessing.pTile != childTileProcessing.pTile)
        continue;
    } else {
      RasterProcessingData workRasterProcessing =
          std::get<RasterProcessingData>(work.processingData);
      RasterProcessingData childRasterProcessing =
          std::get<RasterProcessingData>(childIt->processingData);
      if (workRasterProcessing.pRasterTile != childRasterProcessing.pRasterTile)
        continue;
    }

    childWork.erase(childIt);
    break;
  }
}

void TileWorkManager::SignalWorkComplete(const TileLoadWork& work) {
  std::lock_guard<std::mutex> lock(_requestsLock);

  // Look for any work whose child matches this. And remove ourselves
  for (TileLoadWork& existingRequest : _requestQueue)
    eraseMatchingChildWork(work, existingRequest.childWork);

  std::map<std::string, std::vector<TileLoadWork>>::iterator mapIt;
  for (mapIt = _inFlightRequests.begin(); mapIt != _inFlightRequests.end();
       ++mapIt)
    for (TileLoadWork& inFlightWork : mapIt->second)
      eraseMatchingChildWork(work, inFlightWork.childWork);

  for (TileLoadWork& doneRequest : _processingQueue)
    eraseMatchingChildWork(work, doneRequest.childWork);
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
  foundIt = _inFlightRequests.find(request.requestData.url);
  assert(foundIt != _inFlightRequests.end());

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
        requestWork.responsesByUrl.find(requestWork.requestData.url) ==
        requestWork.responsesByUrl.end());
    ResponseData& responseData =
        requestWork.responsesByUrl[requestWork.requestData.url];

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

    // Put in processing queue
    _processingQueue.push_back(std::move(requestWork));
  }

  // Remove it
  _inFlightRequests.erase(foundIt);
}

void TileWorkManager::dispatchRequest(TileLoadWork& request) {
  this->_pAssetAccessor
      ->get(
          this->_asyncSystem,
          request.requestData.url,
          request.requestData.headers)
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
  assert(_requestQueue.size() > 0);
  TileLoadWork request = _requestQueue.back();
  _requestQueue.pop_back();

  // Move to in flight registry
  std::map<std::string, std::vector<TileLoadWork>>::iterator foundIt;
  foundIt = _inFlightRequests.find(request.requestData.url);
  if (foundIt == _inFlightRequests.end()) {
    // Request doesn't exist, set up a new one
    std::vector<TileLoadWork> newWorkVec;
    newWorkVec.push_back(request);
    _inFlightRequests[request.requestData.url] = newWorkVec;

    // Copy to our output vector
    workNeedingDispatch.push_back(request);
  } else {
    // Tag on to an existing request. Don't bother staging it. Already is.
    foundIt->second.push_back(request);
  }
}

size_t TileWorkManager::GetPendingRequestsCount() {
  std::lock_guard<std::mutex> lock(_requestsLock);
  return _requestQueue.size() + _inFlightRequests.size();
}

size_t TileWorkManager::GetTotalPendingCount() {
  std::lock_guard<std::mutex> lock(_requestsLock);
  return _requestQueue.size() + _inFlightRequests.size() +
         _processingQueue.size() + _failedWork.size();
}

void TileWorkManager::GetRequestsStats(
    size_t& queued,
    size_t& inFlight,
    size_t& done) {
  std::lock_guard<std::mutex> lock(_requestsLock);
  queued = _requestQueue.size();
  inFlight = _inFlightRequests.size();
  done = _processingQueue.size() + _failedWork.size();
}

void TileWorkManager::TakeProcessingWork(
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
  size_t processingCount = _processingQueue.size();
  if (processingCount == 0)
    return;

  // TODO - This list should be a map so it is always sorted
  // Reverse sort so highest priority is at back
  std::sort(_processingQueue.rbegin(), _processingQueue.rend());

  size_t numberToTake = std::min(processingCount, maxCount);

  // Start from the back
  std::vector<TileLoadWork>::iterator it = _processingQueue.end();
  while (1) {
    --it;

    TileLoadWork& work = *it;
    if (!work.childWork.empty()) {
      // Can't take this work yet
      // Child work has to register completion first
    } else {
      // Move this work to output. Erase from queue
      std::vector<TileLoadWork>::iterator eraseIt = it;
      outCompleted.push_back(std::move(*eraseIt));
      _processingQueue.erase(eraseIt);
    }

    if (outCompleted.size() >= numberToTake)
      break;

    if (it == _processingQueue.begin())
      break;
  }
}

void TileWorkManager::transitionQueuedWork() {
  std::vector<TileLoadWork> workNeedingDispatch;
  {
    std::lock_guard<std::mutex> lock(_requestsLock);

    size_t queueCount = _requestQueue.size();
    if (queueCount > 0) {
      // We have work to do

      size_t slotsTotal = _maxSimultaneousRequests;
      size_t slotsUsed = _inFlightRequests.size();
      if (slotsUsed < slotsTotal) {
        // There are free slots
        size_t slotsAvailable = slotsTotal - slotsUsed;

        // Sort our incoming request queue by priority
        // Sort descending so highest priority is at back of vector
        if (queueCount > 1)
          std::sort(_requestQueue.rbegin(), _requestQueue.rend());

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
