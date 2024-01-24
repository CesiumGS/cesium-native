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

typedef std::variant<Tile*, RasterMappedTo3DTile*> TileSource;

typedef std::variant<TileProcessingData, RasterProcessingData> ProcessingData;

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

  struct Order {
    RequestData requestData;

    ProcessingData processingData;

    TileLoadPriorityGroup group;
    double priority;

    std::vector<Order> childOrders;

    bool operator<(const Order& rhs) const noexcept {
      if (this->group == rhs.group)
        return this->priority < rhs.priority;
      else
        return this->group > rhs.group;
    }
  };

  struct Work {
    TileSource tileSource;

    RequestData requestData;

    ProcessingData processingData;

    TileLoadPriorityGroup group;
    double priority;

    bool operator<(const Work& rhs) const noexcept {
      if (this->group == rhs.group)
        return this->priority < rhs.priority;
      else
        return this->group > rhs.group;
    }

    Work* parent;

    std::set<Work*> children;

    ResponseDataMap responsesByUrl;
  };

  void TryAddWork(
      std::vector<Order>& orders,
      size_t maxSimultaneousRequests,
      std::vector<const Work*>& workCreated);

  void RequeueWorkForRequest(Work* requestWork);

  void TakeProcessingWork(
      size_t maxCount,
      std::vector<Work*>& outCompleted,
      std::vector<Work>& outFailed);

  void SignalWorkComplete(Work* work);

  size_t GetPendingRequestsCount();
  size_t GetTotalPendingCount();

  void GetRequestsStats(size_t& queued, size_t& inFlight, size_t& done);

private:
  void transitionQueuedWork();
  void dispatchRequest(Work* requestWork);
  void stageQueuedWork(std::vector<Work*>& workNeedingDispatch);

  void onRequestFinished(
      uint16_t responseStatusCode,
      gsl::span<const std::byte> responseBytes,
      const Work* finishedWork);

  Work* createWorkFromOrder(Order* order);

  void ordersToWork(
      const std::vector<Order*>& orders,
      std::vector<const Work*>& instancesCreated);

  // Thread safe members
  std::mutex _requestsLock;
  bool _exitSignaled = false;

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
