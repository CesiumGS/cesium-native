#include "Cesium3DTilesSelection/TileWorkManager.h"

#include "CesiumAsync/IAssetResponse.h"

using namespace CesiumAsync;

namespace Cesium3DTilesSelection {

TileWorkManager::~TileWorkManager() noexcept {
  assert(_requestsPending.empty());
  assert(_requestsInFlight.empty());
  assert(_processingPending.empty());
  assert(_failedWork.empty());
  assert(_doneWork.empty());
}

void TileWorkManager::Shutdown() {
  std::lock_guard<std::mutex> lock(_requestsLock);

  // We could have requests in flight
  // Let them complete, but signal no more work should be done
  _shutdownSignaled = true;

  _requestsPending.clear();
  _requestsInFlight.clear();
  _processingPending.clear();
  _processingInFlight.clear();
  _failedWork.clear();
  _doneWork.clear();
}

void TileWorkManager::workToProcessingQueue(Work* pWork) {
#ifndef NDEBUG
  for (auto element : _processingPending)
    assert(element->uniqueId != pWork->uniqueId);
#endif

  _processingPending.push_back(pWork);
}

void TileWorkManager::stageWork(Work* pWork) {
  // Assert this work is already owned by this manager
  assert(_ownedWork.find(pWork->uniqueId) != _ownedWork.end());

  CesiumAsync::RequestData* pendingRequest = pWork->getNextRequest();

  if (!pendingRequest) {
    // No pending request, go straight to processing queue
    workToProcessingQueue(pWork);
  } else {
    auto foundIt = _requestsInFlight.find(pendingRequest->url);
    if (foundIt == _requestsInFlight.end()) {
      // The request isn't in flight, queue it
#ifndef NDEBUG
      for (auto element : _requestsPending)
        assert(element->uniqueId != pWork->uniqueId);
#endif
      _requestsPending.push_back(pWork);
    } else {
      // Already in flight, tag along
#ifndef NDEBUG
      for (auto element : foundIt->second)
        assert(element->uniqueId != pWork->uniqueId);
#endif
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

  Work newWork{uniqueId, std::move(*pOrder)};

  if (!newWork.order.requestData.url.empty())
    newWork.pendingRequests.push_back(newWork.order.requestData);

  auto returnPair = _ownedWork.emplace(uniqueId, std::move(newWork));
  assert(returnPair.second);

  Work* pWork = &returnPair.first->second;

  stageWork(pWork);

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

void TileWorkManager::throttleOrders(
    size_t existingCount,
    size_t maxCount,
    std::vector<Order*>& inOutOrders) {
  if (existingCount >= maxCount) {
    // No request slots open, don't submit anything
    inOutOrders.clear();
  } else {
    size_t slotsOpen = maxCount - existingCount;
    if (slotsOpen >= inOutOrders.size()) {
      // We can take all incoming work
      return;
    } else {
      // We can only take part of the incoming work
      // Just submit the highest priority
      // Put highest priority at front of vector
      // then chop off the rest
      std::sort(begin(inOutOrders), end(inOutOrders), [](Order* a, Order* b) {
        return (*a) < (*b);
      });

      inOutOrders.resize(slotsOpen);
    }
  }
}

void TileWorkManager::TryAddOrders(
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

  // Figure out how much url work we will accept
  //
  // For requests, we want some work to be ready to go in between frames
  // so the dispatcher doesn't starve while we wait for a tick
  //
  // For processing we don't want excessive amounts of work queued
  // Ex. Spinning around, content is cached, content requests are beating
  // processing work and queueing up faster than they are consumed
  size_t requestsMax = maxSimultaneousRequests + 10;
  size_t processingMax = maxSimultaneousRequests * 10;

  size_t requestsCount, processingCount;
  thiz->GetPendingCount(requestsCount, processingCount);
  TileWorkManager::throttleOrders(requestsCount, requestsMax, requestOrders);
  TileWorkManager::throttleOrders(
      processingCount,
      processingMax,
      processingOrders);

  if (requestOrders.empty() && processingOrders.empty())
    return;

  {
    std::lock_guard<std::mutex> lock(thiz->_requestsLock);
    thiz->_maxSimultaneousRequests = maxSimultaneousRequests;

    // Copy load requests into internal work we will own
    thiz->ordersToWork(requestOrders, workCreated);
    thiz->ordersToWork(processingOrders, workCreated);
  }

  if (requestOrders.size())
    transitionRequests(thiz);
  if (processingOrders.size())
    transitionProcessing(thiz);
}

void TileWorkManager::RequeueWorkForRequest(
    std::shared_ptr<TileWorkManager>& thiz,
    Work* work) {
  {
    std::lock_guard<std::mutex> lock(thiz->_requestsLock);

    if (thiz->_shutdownSignaled)
      return;

    // This processing work should be in flight, remove it
    auto foundIt = thiz->_processingInFlight.find(work->uniqueId);
    assert(foundIt != thiz->_processingInFlight.end());
    thiz->_processingInFlight.erase(foundIt);

    thiz->stageWork(work);
  }

  transitionRequests(thiz);
}

void TileWorkManager::onWorkComplete(Work* work) {
  std::lock_guard<std::mutex> lock(_requestsLock);

  if (_shutdownSignaled)
    return;

  // Assert this work is already owned by this manager
  assert(_ownedWork.find(work->uniqueId) != _ownedWork.end());

  // This processing work should be in flight, remove it
  auto foundIt = _processingInFlight.find(work->uniqueId);
  assert(foundIt != _processingInFlight.end());
  _processingInFlight.erase(foundIt);

  // Assert this is not in any other queues
#ifndef NDEBUG
  for (auto element : _requestsPending)
    assert(element->uniqueId != work->uniqueId);
  for (auto& urlWorkVecPair : _requestsInFlight)
    for (auto element : urlWorkVecPair.second)
      assert(element->uniqueId != work->uniqueId);
  for (auto element : _processingPending)
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

  // Put in the done list
  _doneWork.push_back(work);
}

void TileWorkManager::SignalWorkComplete(
    std::shared_ptr<TileWorkManager>& thiz,
    Work* work) {
  thiz->onWorkComplete(work);
  transitionProcessing(thiz);
}

void TileWorkManager::onRequestFinished(
    std::shared_ptr<IAssetRequest>& pCompletedRequest) {

  std::lock_guard<std::mutex> lock(_requestsLock);

  if (_shutdownSignaled)
    return;

  const IAssetResponse* response = pCompletedRequest->response();
  uint16_t responseStatusCode = response ? response->statusCode() : 0;

  // Find this request
  auto foundIt = _requestsInFlight.find(pCompletedRequest->url());
  assert(foundIt != _requestsInFlight.end());

  // Handle results
  std::vector<Work*>& requestWorkVec = foundIt->second;
  for (Work* requestWork : requestWorkVec) {
    CesiumAsync::RequestData* workNextRequest = requestWork->getNextRequest();
    assert(workNextRequest);
    assert(pCompletedRequest->url() == workNextRequest->url);

    assert(_ownedWork.find(requestWork->uniqueId) != _ownedWork.end());

    // A response code of 0 is not a valid HTTP code
    // and probably indicates a non-network error.
    // 404 is not found, which is failure
    // Put this work in a failed queue to be handled later
    if (responseStatusCode == 0 || responseStatusCode == 404) {
      std::string errorReason;
      if (responseStatusCode == 0)
        errorReason = "Invalid response for tile content";
      else
        errorReason = "Received status code 404 for tile content";
      _failedWork.emplace_back(
          FailedWorkPair(std::move(errorReason), requestWork));
      continue;
    }

    // Handle requested in the finished work
    const std::string& key = workNextRequest->url;
    assert(
        requestWork->completedRequests.find(key) ==
        requestWork->completedRequests.end());

    requestWork->completedRequests[key] = pCompletedRequest;
    requestWork->pendingRequests.pop_back();

    // Put it back into the appropriate queue
    stageWork(requestWork);
  }

  // Remove it
  _requestsInFlight.erase(foundIt);
}

void TileWorkManager::GetPendingCount(
    size_t& pendingRequests,
    size_t& pendingProcessing) {
  std::lock_guard<std::mutex> lock(_requestsLock);
  pendingRequests = _requestsPending.size() + _requestsInFlight.size();
  pendingProcessing = _processingPending.size();
}

size_t TileWorkManager::GetActiveWorkCount() {
  std::lock_guard<std::mutex> lock(_requestsLock);
  return _requestsPending.size() + _requestsInFlight.size() +
         _processingPending.size() + _processingInFlight.size();
}

void TileWorkManager::GetLoadingWorkStats(
    size_t& requestCount,
    size_t& inFlightCount,
    size_t& processingCount,
    size_t& failedCount) {
  std::lock_guard<std::mutex> lock(_requestsLock);
  requestCount = _requestsPending.size();
  inFlightCount = _requestsInFlight.size();
  processingCount = _processingPending.size();
  failedCount = _failedWork.size();
}

void TileWorkManager::TakeCompletedWork(
    std::vector<DoneOrder>& outCompleted,
    std::vector<FailedOrder>& outFailed) {
  std::lock_guard<std::mutex> lock(_requestsLock);

  for (auto work : _doneWork) {
    // We should own this and it should not be in any other queues
    auto foundIt = _ownedWork.find(work->uniqueId);

#ifndef NDEBUG
    assert(foundIt != _ownedWork.end());
    for (auto element : _requestsPending)
      assert(element->uniqueId != work->uniqueId);
    for (auto& urlWorkVecPair : _requestsInFlight)
      for (auto element : urlWorkVecPair.second)
        assert(element->uniqueId != work->uniqueId);
    for (auto element : _processingPending)
      assert(element->uniqueId != work->uniqueId);
    assert(
        _processingInFlight.find(work->uniqueId) == _processingInFlight.end());
#endif

    outCompleted.emplace_back(
        DoneOrder{std::move(work->tileLoadResult), std::move(work->order)});
    _ownedWork.erase(foundIt);
  }
  _doneWork.clear();

  for (auto workPair : _failedWork) {
    Work* work = workPair.second;

    // We should own this and it should not be in any other queues
    auto foundIt = _ownedWork.find(work->uniqueId);

#ifndef NDEBUG
    assert(foundIt != _ownedWork.end());
    for (auto element : _requestsPending)
      assert(element->uniqueId != work->uniqueId);
    for (auto& urlWorkVecPair : _requestsInFlight)
      for (auto element : urlWorkVecPair.second)
        assert(element->uniqueId != work->uniqueId);
    for (auto element : _processingPending)
      assert(element->uniqueId != work->uniqueId);
    assert(
        _processingInFlight.find(work->uniqueId) == _processingInFlight.end());
#endif

    outFailed.emplace_back(FailedOrder{workPair.first, std::move(work->order)});
    _ownedWork.erase(foundIt);
  }
  _failedWork.clear();
}

void TileWorkManager::transitionRequests(
    std::shared_ptr<TileWorkManager>& thiz) {
  std::vector<Work*> workNeedingDispatch;
  {
    std::lock_guard<std::mutex> lock(thiz->_requestsLock);

    if (thiz->_shutdownSignaled)
      return;

    size_t queueCount = thiz->_requestsPending.size();
    if (queueCount == 0)
      return;

    // We have work to do, check if there's a slot for it
    size_t slotsTotal = thiz->_maxSimultaneousRequests;
    size_t slotsUsed = thiz->_requestsInFlight.size();
    assert(slotsUsed <= slotsTotal);
    if (slotsUsed == slotsTotal)
      return;

    // At least one slot is open
    // Sort our incoming request queue by priority
    // Want highest priority at back of vector
    std::sort(
        begin(thiz->_requestsPending),
        end(thiz->_requestsPending),
        [](Work* a, Work* b) { return b->order < a->order; });

    // Loop through all pending until no more slots (or pending)
    while (!thiz->_requestsPending.empty() && slotsUsed < slotsTotal) {

      // Start from back of queue (highest priority).
      Work* requestWork = thiz->_requestsPending.back();

      CesiumAsync::RequestData* nextRequest = requestWork->getNextRequest();
      assert(nextRequest);

      const std::string& workUrl = nextRequest->url;

      // The first work with this url needs dispatch
      workNeedingDispatch.push_back(requestWork);

      // Gather all work with urls that match this
      using WorkVecIterator = std::vector<Work*>::iterator;
      std::vector<WorkVecIterator> matchingUrlWork;
      auto matchIt = thiz->_requestsPending.end() - 1;
      matchingUrlWork.push_back(matchIt);

      while (matchIt != thiz->_requestsPending.begin()) {
        --matchIt;
        Work* otherWork = *matchIt;
        CesiumAsync::RequestData* otherRequest = otherWork->getNextRequest();
        assert(otherRequest);
        if (otherRequest->url == workUrl)
          matchingUrlWork.push_back(matchIt);
      }

      // Set up a new inflight request
      // Erase related entries from pending queue
      std::vector<Work*> newWorkVec;
      assert(
          thiz->_requestsInFlight.find(workUrl) ==
          thiz->_requestsInFlight.end());
      for (WorkVecIterator& it : matchingUrlWork) {
        newWorkVec.push_back(*it);
        thiz->_requestsPending.erase(it);
      }

      thiz->_requestsInFlight.emplace(workUrl, std::move(newWorkVec));
      ++slotsUsed;
    }
  }

  for (Work* requestWork : workNeedingDispatch) {
    // Keep the manager alive while the load is in progress
    // Capture the shared pointer by value

    CesiumAsync::RequestData* nextRequest = requestWork->getNextRequest();
    assert(nextRequest);

    thiz->_pAssetAccessor
        ->get(thiz->_asyncSystem, nextRequest->url, nextRequest->headers)
        .thenImmediately(
            [thiz](std::shared_ptr<IAssetRequest>&& pCompletedRequest) mutable {
              assert(thiz.get());
              thiz->onRequestFinished(pCompletedRequest);
              transitionRequests(thiz);
            })
        .thenInMainThread([thiz]() mutable { transitionProcessing(thiz); });
  }
}

void TileWorkManager::transitionProcessing(
    std::shared_ptr<TileWorkManager>& thiz) {
  std::vector<Work*> workNeedingDispatch;
  {
    std::lock_guard<std::mutex> lock(thiz->_requestsLock);

    if (thiz->_shutdownSignaled)
      return;

    size_t pendingCount = thiz->_processingPending.size();
    if (pendingCount == 0)
      return;

    // We have work to do, check if there's a slot for it
    size_t slotsTotal = thiz->_maxSimultaneousRequests;
    size_t slotsUsed = thiz->_processingInFlight.size();
    assert(slotsUsed <= slotsTotal);
    if (slotsUsed == slotsTotal)
      return;

    // At least one slot is open
    size_t slotsAvailable = slotsTotal - slotsUsed;

    // Gather iterators we want to erase
    // Go from back to front to avoid reallocations if possible
    // These work items have completed based on priority from any
    // number of previous frames, so we really don't know which ones
    // should go out first. They should all go ASAP.
    using WorkVecIter = std::vector<Work*>::iterator;
    std::vector<WorkVecIter> processingToErase;
    std::vector<Work*>::iterator it = thiz->_processingPending.end();
    while (it != thiz->_processingPending.begin()) {
      --it;
      Work* work = *it;
      if (work->children.empty()) {
        // Move this work to in flight
        assert(
            thiz->_processingInFlight.find(work->uniqueId) ==
            thiz->_processingInFlight.end());
        thiz->_processingInFlight[work->uniqueId] = work;

        // Move this work to output and erase from pending
        workNeedingDispatch.push_back(work);
        processingToErase.push_back(it);
      } else {
        // Can't take this work yet
        // Child work has to register completion first
      }

      if (workNeedingDispatch.size() >= slotsAvailable)
        break;
    }

    // Delete any entries gathered
    for (WorkVecIter eraseIt : processingToErase)
      thiz->_processingPending.erase(eraseIt);
  }

  for (Work* work : workNeedingDispatch) {
    CesiumAsync::UrlResponseDataMap responseDataMap;
    work->fillResponseDataMap(responseDataMap);

    if (std::holds_alternative<TileProcessingData>(
            work->order.processingData)) {
      TileProcessingData& tileProcessing =
          std::get<TileProcessingData>(work->order.processingData);

      thiz->_tileDispatchFunc(tileProcessing, responseDataMap, work);
    } else {
      RasterProcessingData& rasterProcessing =
          std::get<RasterProcessingData>(work->order.processingData);

      thiz->_rasterDispatchFunc(rasterProcessing, responseDataMap, work);
    }
  }
}

void TileWorkManager::SetDispatchFunctions(
    TileDispatchFunc& tileDispatch,
    RasterDispatchFunc& rasterDispatch) {
  std::lock_guard<std::mutex> lock(_requestsLock);
  _tileDispatchFunc = tileDispatch;
  _rasterDispatchFunc = rasterDispatch;
}

} // namespace Cesium3DTilesSelection
