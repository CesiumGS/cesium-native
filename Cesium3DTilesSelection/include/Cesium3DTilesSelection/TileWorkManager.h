#pragma once

#include "Tile.h"
#include "TilesetContentLoader.h"

#include <set>

namespace Cesium3DTilesSelection {
class TilesetMetadata;

struct TileProcessingData {
  Tile* pTile;
  TileProcessingCallback tileCallback;
  std::vector<CesiumGeospatial::Projection> projections;
};

struct RasterProcessingData {
  RasterMappedTo3DTile* pRasterTile;
  RasterProcessingCallback rasterCallback;
};

class TileWorkManager {

public:
  TileWorkManager(
      CesiumAsync::AsyncSystem asyncSystem,
      std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor,
      std::shared_ptr<spdlog::logger> pLogger)
      : _asyncSystem(asyncSystem),
        _pAssetAccessor(pAssetAccessor),
        _pLogger(pLogger) {}
  ~TileWorkManager() noexcept;

  using ProcessingData = std::variant<TileProcessingData, RasterProcessingData>;

  struct Order {
    RequestData requestData;

    ProcessingData processingData;

    TileLoadPriorityGroup group;
    double priority;

    std::vector<Order> childOrders = {};

    bool operator<(const Order& rhs) const noexcept {
      if (this->group == rhs.group)
        return this->priority < rhs.priority;
      else
        return this->group > rhs.group;
    }
  };

  using TileSource = std::variant<Tile*, RasterMappedTo3DTile*>;

  struct Work {
    TileSource uniqueId;

    Order order;

    Work* parent = nullptr;

    std::set<Work*> children = {};

    UrlAssetRequestMap completedRequests = {};

    void fillResponseDataMap(UrlResponseDataMap& responseDataMap) {
      for (auto& pair : completedRequests) {
        responseDataMap.emplace(
            pair.first,
            ResponseData{pair.second.get(), pair.second->response()});
      }
    }
  };

  static void TryAddWork(
      std::shared_ptr<TileWorkManager>& thiz,
      std::vector<Order>& orders,
      size_t maxSimultaneousRequests,
      std::vector<const Work*>& workCreated);

  static void RequeueWorkForRequest(
      std::shared_ptr<TileWorkManager>& thiz,
      Work* requestWork);

  void TakeProcessingWork(
      size_t maxCount,
      std::vector<Work*>& outCompleted,
      std::vector<Work>& outFailed);

  void SignalWorkComplete(Work* work);

  size_t GetPendingRequestsCount();
  size_t GetTotalPendingCount();

  void GetRequestsStats(size_t& queued, size_t& inFlight, size_t& done);

  void Shutdown();

private:
  static void transitionQueuedWork(std::shared_ptr<TileWorkManager>& thiz);

  void onRequestFinished(
      std::shared_ptr<CesiumAsync::IAssetRequest>& pCompletedRequest,
      const Work* finishedWork);

  void workToStartingQueue(Work* pWork);

  Work* createWorkFromOrder(Order* pOrder);

  void ordersToWork(
      const std::vector<Order*>& orders,
      std::vector<const Work*>& instancesCreated);

  // Thread safe members
  std::mutex _requestsLock;
  bool _shutdownSignaled = false;

  std::map<TileSource, Work> _ownedWork;

  std::vector<Work*> _requestQueue;
  std::map<std::string, std::vector<Work*>> _inFlightRequests;
  std::vector<Work*> _processingQueue;
  std::vector<Work*> _failedWork;

  CesiumAsync::AsyncSystem _asyncSystem;

  std::shared_ptr<CesiumAsync::IAssetAccessor> _pAssetAccessor;

  std::shared_ptr<spdlog::logger> _pLogger;

  size_t _maxSimultaneousRequests;
};

} // namespace Cesium3DTilesSelection