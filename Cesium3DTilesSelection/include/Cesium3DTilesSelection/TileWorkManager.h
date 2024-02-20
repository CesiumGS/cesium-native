#pragma once

#include "Tile.h"
#include "TilesetContentLoader.h"

#include <set>

namespace Cesium3DTilesSelection {
class TilesetMetadata;

struct TileProcessingData {
  Tile* pTile = nullptr;
  TileProcessingCallback tileCallback = {};
  std::vector<CesiumGeospatial::Projection> projections{};
};

struct RasterProcessingData {
  RasterMappedTo3DTile* pRasterTile = nullptr;
  CesiumRasterOverlays::RasterProcessingCallback rasterCallback = {};
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
    CesiumAsync::RequestData requestData = {};

    ProcessingData processingData = {};

    TileLoadPriorityGroup group = TileLoadPriorityGroup::Normal;
    double priority = 0;

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
    TileSource uniqueId = {};

    Order order = {};

    Work* parent = nullptr;

    std::set<Work*> children = {};

    std::vector<CesiumAsync::RequestData> pendingRequests = {};
    CesiumAsync::UrlAssetRequestMap completedRequests = {};

    void fillResponseDataMap(CesiumAsync::UrlResponseDataMap& responseDataMap) {
      for (auto& pair : completedRequests) {
        responseDataMap.emplace(
            pair.first,
            CesiumAsync::ResponseData{
                pair.second.get(),
                pair.second->response()});
      }
    }

    CesiumAsync::RequestData* getNextRequest() {
      // Next request always comes from the back
      // Order isn't important here
      if (pendingRequests.empty()) {
        return nullptr;
      } else {
        assert(!pendingRequests.back().url.empty());
        return &pendingRequests.back();
      }
    }
  };

  static void TryAddOrders(
      std::shared_ptr<TileWorkManager>& thiz,
      std::vector<Order>& orders,
      size_t maxSimultaneousRequests,
      std::vector<const Work*>& workCreated);

  static void RequeueWorkForRequest(
      std::shared_ptr<TileWorkManager>& thiz,
      Work* requestWork);

  struct FailedOrder {
    std::string failureReason = "";
    Order order = {};
  };

  void TakeProcessingWork(
      size_t maxCount,
      std::vector<Work*>& outCompleted,
      std::vector<FailedOrder>& outFailed);

  void SignalWorkComplete(Work* work);

  void GetPendingCount(size_t& pendingRequests, size_t& pendingProcessing);
  size_t GetActiveWorkCount();

  void GetLoadingWorkStats(
      size_t& requestCount,
      size_t& inFlightCount,
      size_t& processingCount,
      size_t& failedCount);

  void Shutdown();

private:
  static void throttleOrders(
      size_t existingCount,
      size_t maxCount,
      std::vector<Order*>& inOutOrders);

  static void transitionQueuedWork(std::shared_ptr<TileWorkManager>& thiz);

  void onRequestFinished(
      std::shared_ptr<CesiumAsync::IAssetRequest>& pCompletedRequest);

  void stageWork(Work* pWork);

  void workToProcessingQueue(Work* pWork);

  Work* createWorkFromOrder(Order* pOrder);

  void ordersToWork(
      const std::vector<Order*>& orders,
      std::vector<const Work*>& instancesCreated);

  std::mutex _requestsLock;

  bool _shutdownSignaled = false;
  std::map<TileSource, Work> _ownedWork;
  std::vector<Work*> _requestQueue;
  std::map<std::string, std::vector<Work*>> _inFlightRequests;
  std::vector<Work*> _processingQueue;

  using FailedWorkPair = std::pair<std::string, Work*>;
  using FailedWorkVec = std::vector<FailedWorkPair>;
  FailedWorkVec _failedWork;

  CesiumAsync::AsyncSystem _asyncSystem;
  std::shared_ptr<CesiumAsync::IAssetAccessor> _pAssetAccessor;
  std::shared_ptr<spdlog::logger> _pLogger;
  size_t _maxSimultaneousRequests;
};

} // namespace Cesium3DTilesSelection
