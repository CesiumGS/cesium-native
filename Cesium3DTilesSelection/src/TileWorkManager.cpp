#include "Cesium3DTilesSelection/TileWorkManager.h"

#include "CesiumAsync/IAssetResponse.h"

using namespace CesiumAsync;

namespace Cesium3DTilesSelection {

TileWorkManager::~TileWorkManager() noexcept {
  assert(_requestQueue.empty());
  assert(_inFlightRequests.empty());
  assert(_processingQueue.empty());
  assert(_failedWork.empty());
}

void TileWorkManager::Shutdown() {
  std::lock_guard<std::mutex> lock(_requestsLock);

  // We could have requests in flight
  // Let them complete, but signal no more work should be done
  _shutdownSignaled = true;

  _requestQueue.clear();
  _inFlightRequests.clear();
  _processingQueue.clear();
  _failedWork.clear();
}

void TileWorkManager::workToStartingQueue(Work* pWork) {
  // Assert this work is already owned by this manager
  assert(_ownedWork.find(pWork->uniqueId) != _ownedWork.end());

  if (pWork->order.requestData.url.empty()) {
    _processingQueue.push_back(pWork);
  } else {
    auto foundIt = _inFlightRequests.find(pWork->order.requestData.url);
    if (foundIt == _inFlightRequests.end()) {
      // The request isn't in flight, add it to the queue
      _requestQueue.push_back(pWork);
    } else {
      // Already in flight, tag along
      foundIt->second.push_back(pWork);
    }
  }
}

TileWorkManager::Work* TileWorkManager::createWorkFromOrder(Order* pOrder) {
  bool workHasTileProcessing =
      std::holds_alternative<TileProcessingData>(pOrder->processingData);

  TileSource uniqueId;
  if (workHasTileProcessing) {
    TileProcessingData workTileProcessing =
        std::get<TileProcessingData>(pOrder->processingData);
    uniqueId = workTileProcessing.pTile;
  } else {
    RasterProcessingData workRasterProcessing =
        std::get<RasterProcessingData>(pOrder->processingData);
    uniqueId = workRasterProcessing.pRasterTile;
  }

  // Assert any work isn't already owned by this manager
  assert(_ownedWork.find(uniqueId) == _ownedWork.end());

  auto returnPair =
      _ownedWork.emplace(uniqueId, Work{uniqueId, std::move(*pOrder)});
  assert(returnPair.second);

  Work* pWork = &returnPair.first->second;
  workToStartingQueue(pWork);

  return pWork;
}

void TileWorkManager::ordersToWork(
    const std::vector<Order*>& orders,
    std::vector<const Work*>& instancesCreated) {

  for (Order* order : orders) {
    Work* newInstance = createWorkFromOrder(order);

    instancesCreated.push_back(newInstance);

    // Create child work, if exists. Link parent->child with raw pointers
    // Only support one level deep, for now
    for (Order& childWork : newInstance->order.childOrders) {
      Work* newChildInstance = createWorkFromOrder(&childWork);
      newInstance->children.insert(newChildInstance);
      newChildInstance->parent = newInstance;

      instancesCreated.push_back(newChildInstance);
    }
  }
}

void TileWorkManager::TryAddWork(
    std::shared_ptr<TileWorkManager>& thiz,
    std::vector<Order>& orders,
    size_t maxSimultaneousRequests,
    std::vector<const Work*>& workCreated) {
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
  size_t pendingRequestCount = thiz->GetPendingRequestsCount();

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
      // Put highest priority at front of vector
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
    std::lock_guard<std::mutex> lock(thiz->_requestsLock);
    thiz->_maxSimultaneousRequests = maxSimultaneousRequests;

    // Copy load requests into internal work we will own
    thiz->ordersToWork(requestOrdersToSubmit, workCreated);
    thiz->ordersToWork(processingOrders, workCreated);
  }

  if (requestOrdersToSubmit.size()) {
    SPDLOG_LOGGER_INFO(
        thiz->_pLogger,
        "Sending request work to dispatcher: {} entries",
        requestOrdersToSubmit.size());

    transitionQueuedWork(thiz);
  }
}

void TileWorkManager::RequeueWorkForRequest(
    std::shared_ptr<TileWorkManager>& thiz,
    Work* requestWork) {
  {
    std::lock_guard<std::mutex> lock(thiz->_requestsLock);
    thiz->workToStartingQueue(requestWork);
  }

  transitionQueuedWork(thiz);
}

void TileWorkManager::SignalWorkComplete(Work* work) {
  std::lock_guard<std::mutex> lock(_requestsLock);

  // Assert this work is already owned by this manager
  assert(_ownedWork.find(work->uniqueId) != _ownedWork.end());

  // Assert this is not in any other queues
#ifndef NDEBUG
  for (auto element : _requestQueue)
    assert(element->uniqueId != work->uniqueId);

  for (auto urlWorkVecPair : _inFlightRequests)
    for (auto element : urlWorkVecPair.second)
      assert(element->uniqueId != work->uniqueId);

  for (auto element : _processingQueue)
    assert(element->uniqueId != work->uniqueId);
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
  _ownedWork.erase(work->uniqueId);
}

void TileWorkManager::onRequestFinished(
    std::shared_ptr<IAssetRequest>& pCompletedRequest,
    const Work* finishedWork) {

  std::lock_guard<std::mutex> lock(_requestsLock);

  if (_shutdownSignaled)
    return;

  assert(pCompletedRequest->url() == finishedWork->order.requestData.url);

  const IAssetResponse* response = pCompletedRequest->response();
  uint16_t responseStatusCode = response ? response->statusCode() : 0;

  // Find this request
  auto foundIt = _inFlightRequests.find(finishedWork->order.requestData.url);
  assert(foundIt != _inFlightRequests.end());

  // Handle results
  std::vector<Work*>& requestWorkVec = foundIt->second;
  for (Work* requestWork : requestWorkVec) {

    if (responseStatusCode == 0) {
      // A response code of 0 is not a valid HTTP code
      // and probably indicates a non-network error.
      // Put this work in a failed queue to be handled later
      _failedWork.push_back(requestWork);
      continue;
    }

    // Add new entry
    assert(
        requestWork->completedRequests.find(
            requestWork->order.requestData.url) ==
        requestWork->completedRequests.end());

    std::string& key = requestWork->order.requestData.url;
    requestWork->completedRequests[key] = pCompletedRequest;

    // Put in processing queue
    _processingQueue.push_back(requestWork);
  }

  // Remove it
  _inFlightRequests.erase(foundIt);
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
    std::vector<Work*>& outCompleted,
    std::vector<Work>& outFailed) {
  std::lock_guard<std::mutex> lock(_requestsLock);

  // All failed requests go out
  if (_failedWork.empty()) {
    // Failed work immediately releases ownership to caller
    for (auto work : _failedWork) {
      auto foundIt = _ownedWork.find(work->uniqueId);
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
  // Want highest priority at back
  std::sort(
      begin(_processingQueue),
      end(_processingQueue),
      [](Work* a, Work* b) { return b->order < a->order; });

  size_t numberToTake = std::min(processingCount, maxCount);

  // Start from the back
  auto it = _processingQueue.end();
  while (1) {
    --it;

    Work* work = *it;
    if (!work->children.empty()) {
      // Can't take this work yet
      // Child work has to register completion first
    } else {
      // Move this work to output. Erase from queue
      auto eraseIt = it;
      outCompleted.push_back(*eraseIt);
      _processingQueue.erase(eraseIt);
    }

    if (outCompleted.size() >= numberToTake)
      break;

    if (it == _processingQueue.begin())
      break;
  }
}

void TileWorkManager::transitionQueuedWork(
    std::shared_ptr<TileWorkManager>& thiz) {
  std::vector<Work*> workNeedingDispatch;
  {
    std::lock_guard<std::mutex> lock(thiz->_requestsLock);

    if (thiz->_shutdownSignaled)
      return;

    size_t queueCount = thiz->_requestQueue.size();
    if (queueCount == 0)
      return;

    // We have work to do, check if there's a slot for it
    size_t slotsTotal = thiz->_maxSimultaneousRequests;
    size_t slotsUsed = thiz->_inFlightRequests.size();
    assert(slotsUsed <= slotsTotal);
    if (slotsUsed == slotsTotal)
      return;

    // At least one slot is open
    // Sort our incoming request queue by priority
    // Want highest priority at back of vector
    std::sort(
        begin(thiz->_requestQueue),
        end(thiz->_requestQueue),
        [](Work* a, Work* b) { return b->order < a->order; });

    // Loop through all pending until no more slots (or pending)
    while (!thiz->_requestQueue.empty() && slotsUsed < slotsTotal) {

      // Start from back of queue (highest priority).
      Work* requestWork = thiz->_requestQueue.back();
      const std::string& workUrl = requestWork->order.requestData.url;

      // The first work with this url needs dispatch
      workNeedingDispatch.push_back(requestWork);

      // Gather all work with urls that match this
      using WorkVecIterator = std::vector<Work*>::iterator;
      std::vector<WorkVecIterator> matchingUrlWork;
      auto matchIt = thiz->_requestQueue.end() - 1;
      matchingUrlWork.push_back(matchIt);

      while (matchIt != thiz->_requestQueue.begin()) {
        --matchIt;
        Work* otherWork = *matchIt;
        if (otherWork->order.requestData.url == workUrl)
          matchingUrlWork.push_back(matchIt);
      }

      // Set up a new inflight request
      // Erase related entries from pending queue
      std::vector<Work*> newWorkVec;
      assert(
          thiz->_inFlightRequests.find(workUrl) ==
          thiz->_inFlightRequests.end());
      for (WorkVecIterator& it : matchingUrlWork) {
        newWorkVec.push_back(*it);
        thiz->_requestQueue.erase(it);
      }

      thiz->_inFlightRequests.emplace(workUrl, std::move(newWorkVec));
      ++slotsUsed;
    }
  }

  for (Work* requestWork : workNeedingDispatch) {
    // Keep the manager alive while the load is in progress
    // Capture the shared pointer by value
    thiz->_pAssetAccessor
        ->get(
            thiz->_asyncSystem,
            requestWork->order.requestData.url,
            requestWork->order.requestData.headers)
        .thenImmediately(
            [thiz, _requestWork = requestWork](
                std::shared_ptr<IAssetRequest>&& pCompletedRequest) mutable {
              assert(thiz.get());

              thiz->onRequestFinished(pCompletedRequest, _requestWork);

              transitionQueuedWork(thiz);
            });
  }
}

} // namespace Cesium3DTilesSelection