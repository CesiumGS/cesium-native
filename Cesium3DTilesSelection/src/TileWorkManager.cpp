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

WorkInstance* TileWorkManager::createWorkInstance(Order* order) {
  bool workHasTileProcessing =
      std::holds_alternative<TileProcessingData>(order->processingData);

  TileSource tileSource;
  if (workHasTileProcessing) {
    TileProcessingData workTileProcessing =
        std::get<TileProcessingData>(order->processingData);
    tileSource = workTileProcessing.pTile;
  } else {
    RasterProcessingData workRasterProcessing =
        std::get<RasterProcessingData>(order->processingData);
    tileSource = workRasterProcessing.pRasterTile;
  }

  // Assert any work isn't already owned by this manager
  assert(_ownedWork.find(tileSource) == _ownedWork.end());

  WorkInstance internalWork = {
      tileSource,
      std::move(order->requestData),
      std::move(order->processingData),
      std::move(order->group),
      std::move(order->priority)};

  auto returnPair = _ownedWork.emplace(tileSource, std::move(internalWork));
  assert(returnPair.second);

  WorkInstance* workPointer = &returnPair.first->second;

  // Put this in the appropriate starting queue
  if (workPointer->requestData.url.empty())
    _processingQueue.push_back(workPointer);
  else
    _requestQueue.push_back(workPointer);

  return workPointer;
}

void TileWorkManager::requestsToInstances(
    const std::vector<Order*>& orders,
    std::vector<const WorkInstance*>& instancesCreated) {

  for (Order* order : orders) {
    WorkInstance* newInstance = createWorkInstance(order);

    instancesCreated.push_back(newInstance);

    // Create child work, if exists. Link parent->child with raw pointers
    // Only support one level deep, for now
    for (Order& childWork : order->childOrders) {
      WorkInstance* newChildInstance = createWorkInstance(&childWork);
      newInstance->children.insert(newChildInstance);
      newChildInstance->parent = newInstance;

      instancesCreated.push_back(newChildInstance);
    }
  }
}

void TileWorkManager::TryAddWork(
    std::vector<Order>& orders,
    size_t maxSimultaneousRequests,
    std::vector<const WorkInstance*>& instancesCreated) {
  if (orders.empty())
    return;

  // Request work will always go to that queue first
  // Work with only processing can bypass it
  std::vector<Order*> requestOrders;
  std::vector<Order*> processingOrders;
  for (Order& order : orders) {
    if (order.requestData.url.empty())
      processingOrders.push_back(&order);
    else
      requestOrders.push_back(&order);
  }

  // Figure out how much url work we will accept.
  // We want some work to be ready to go in between frames
  // so the dispatcher doesn't starve while we wait for a tick
  size_t betweenFrameBuffer = 10;
  size_t maxCountToQueue = maxSimultaneousRequests + betweenFrameBuffer;
  size_t pendingRequestCount = this->GetPendingRequestsCount();

  std::vector<Order*> requestOrdersToSubmit;
  if (pendingRequestCount >= maxCountToQueue) {
    // No request slots open, we can at least insert our processing work
  } else {
    size_t slotsOpen = maxCountToQueue - pendingRequestCount;
    if (slotsOpen >= requestOrders.size()) {
      // We can take all incoming work
      requestOrdersToSubmit = requestOrders;
    } else {
      // We can only take part of the incoming work
      // Just submit the highest priority
      requestOrdersToSubmit = requestOrders;

      std::sort(
          begin(requestOrdersToSubmit),
          end(requestOrdersToSubmit),
          [](Order* a, Order* b) { return (*a) < (*b); });

      requestOrdersToSubmit.resize(slotsOpen);
    }
  }

  if (requestOrdersToSubmit.empty() && processingOrders.empty())
    return;

  {
    std::lock_guard<std::mutex> lock(_requestsLock);
    this->_maxSimultaneousRequests = maxSimultaneousRequests;

    // Copy load requests into internal work we will own
    requestsToInstances(requestOrdersToSubmit, instancesCreated);
    requestsToInstances(processingOrders, instancesCreated);
  }

  if (requestOrdersToSubmit.size()) {
    SPDLOG_LOGGER_INFO(
        this->_pLogger,
        "Sending request work to dispatcher: {} entries",
        requestOrdersToSubmit.size());

    transitionQueuedWork();
  }
}

void TileWorkManager::RequeueWorkForRequest(WorkInstance* requestWork) {
  {
    std::lock_guard<std::mutex> lock(_requestsLock);

    // Assert this work is already owned by this manager
    assert(_ownedWork.find(requestWork->tileSource) != _ownedWork.end());

    // It goes in the request queue
    _requestQueue.push_back(requestWork);
  }

  transitionQueuedWork();
}

void TileWorkManager::SignalWorkComplete(WorkInstance* work) {
  std::lock_guard<std::mutex> lock(_requestsLock);

  // Assert this work is already owned by this manager
  assert(_ownedWork.find(work->tileSource) != _ownedWork.end());

  // Assert this is not in any other queues
#ifndef NDEBUG
  for (auto element : _requestQueue)
    assert(element->tileSource != work->tileSource);

  for (auto urlWorkVecPair : _inFlightRequests)
    for (auto element : urlWorkVecPair.second)
      assert(element->tileSource != work->tileSource);

  for (auto element : _processingQueue)
    assert(element->tileSource != work->tileSource);
#endif

  // If this work has parent work, remove this reference
  // Work with child work waits until the children are done
  if (work->parent) {
    // Child should be in parent's list, remove it
    auto& childSet = work->parent->children;
    assert(childSet.find(work) != childSet.end());
    childSet.erase(work);
  }

  // Done work should have no registered child work
  assert(work->children.empty());

  // Remove it
  _ownedWork.erase(work->tileSource);
}

void TileWorkManager::onRequestFinished(
    uint16_t responseStatusCode,
    gsl::span<const std::byte> responseBytes,
    const WorkInstance* request) {
  std::lock_guard<std::mutex> lock(_requestsLock);

  if (_exitSignaled)
    return;

  // Find this request
  auto foundIt = _inFlightRequests.find(request->requestData.url);
  assert(foundIt != _inFlightRequests.end());

  // Handle results
  std::vector<WorkInstance*>& requestWorkVec = foundIt->second;
  for (WorkInstance* requestWork : requestWorkVec) {

    if (responseStatusCode == 0) {
      // A response code of 0 is not a valid HTTP code
      // and probably indicates a non-network error.
      // Put this work in a failed queue to be handled later
      _failedWork.push_back(requestWork);
      continue;
    }

    // Add new entry
    assert(
        requestWork->responsesByUrl.find(requestWork->requestData.url) ==
        requestWork->responsesByUrl.end());
    ResponseData& responseData =
        requestWork->responsesByUrl[requestWork->requestData.url];

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
    _processingQueue.push_back(requestWork);
  }

  // Remove it
  _inFlightRequests.erase(foundIt);
}

void TileWorkManager::dispatchRequest(WorkInstance* request) {
  this->_pAssetAccessor
      ->get(
          this->_asyncSystem,
          request->requestData.url,
          request->requestData.headers)
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
    std::vector<WorkInstance*>& workNeedingDispatch) {
  // Take from back of queue (highest priority).
  assert(_requestQueue.size() > 0);
  WorkInstance* request = _requestQueue.back();
  _requestQueue.pop_back();

  // Move to in flight registry
  auto foundIt = _inFlightRequests.find(request->requestData.url);
  if (foundIt == _inFlightRequests.end()) {
    // Request doesn't exist, set up a new one
    std::vector<WorkInstance*> newWorkVec;
    newWorkVec.push_back(request);
    _inFlightRequests[request->requestData.url] = newWorkVec;

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
    std::vector<WorkInstance*>& outCompleted,
    std::vector<WorkInstance>& outFailed) {
  std::lock_guard<std::mutex> lock(_requestsLock);

  // All failed requests go out
  if (_failedWork.empty()) {
    // Failed work immediately releases ownership to caller
    for (auto work : _failedWork) {
      auto foundIt = _ownedWork.find(work->tileSource);
      assert(foundIt != _ownedWork.end());

      outFailed.push_back(std::move(foundIt->second));

      _ownedWork.erase(foundIt);
    }
  }

  // If no room for completed work, stop here
  if (maxCount == 0)
    return;

  // Return completed work, up to the count specified
  size_t processingCount = _processingQueue.size();
  if (processingCount == 0)
    return;

  // TODO - This list should be a map so it is always sorted
  // Reverse sort so highest priority is at back
  std::sort(_processingQueue.rbegin(), _processingQueue.rend());

  size_t numberToTake = std::min(processingCount, maxCount);

  // Start from the back
  std::vector<WorkInstance*>::iterator it = _processingQueue.end();
  while (1) {
    --it;

    WorkInstance* work = *it;
    if (!work->children.empty()) {
      // Can't take this work yet
      // Child work has to register completion first
    } else {
      // Move this work to output. Erase from queue
      std::vector<WorkInstance*>::iterator eraseIt = it;
      outCompleted.push_back(*eraseIt);
      _processingQueue.erase(eraseIt);
    }

    if (outCompleted.size() >= numberToTake)
      break;

    if (it == _processingQueue.begin())
      break;
  }
}

void TileWorkManager::transitionQueuedWork() {
  std::vector<WorkInstance*> workNeedingDispatch;
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

  for (WorkInstance* requestWork : workNeedingDispatch)
    dispatchRequest(requestWork);
}

} // namespace Cesium3DTilesSelection
