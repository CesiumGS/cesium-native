#pragma once

#include "Tile.h"
#include "TilesetContentLoader.h"

#include <set>

namespace Cesium3DTilesSelection {
class TilesetMetadata;

struct TileProcessingData {
  Tile* pTile;
  TileProcessingCallback tileCallback;
};

struct RasterProcessingData {
  RasterMappedTo3DTile* pRasterTile;
  RasterProcessingCallback rasterCallback;
};

typedef std::variant<Tile*, RasterMappedTo3DTile*> TileSource;

typedef std::variant<TileProcessingData, RasterProcessingData> ProcessingData;

struct TileLoadWork {
  RequestData requestData;

  ProcessingData processingData;

  std::vector<CesiumGeospatial::Projection> projections;
  TileLoadPriorityGroup group;
  double priority;

  std::vector<TileLoadWork> childWork;

  bool operator<(const TileLoadWork& rhs) const noexcept {
    if (this->group == rhs.group)
      return this->priority < rhs.priority;
    else
      return this->group > rhs.group;
  }
};

struct WorkInstance {
  TileSource tileSource;

  RequestData requestData;

  ProcessingData processingData;

  std::vector<CesiumGeospatial::Projection> projections;
  TileLoadPriorityGroup group;
  double priority;

  bool operator<(const TileLoadWork& rhs) const noexcept {
    if (this->group == rhs.group)
      return this->priority < rhs.priority;
    else
      return this->group > rhs.group;
  }

  WorkInstance* parent;

  std::set<WorkInstance*> children;

  ResponseDataMap responsesByUrl;
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

  void TryAddWork(
      std::vector<TileLoadWork>& loadWork,
      size_t maxSimultaneousRequests,
      std::vector<const WorkInstance*>& instancesCreated);

  void RequeueWorkForRequest(WorkInstance* requestWork);

  void TakeProcessingWork(
      size_t maxCount,
      std::vector<WorkInstance*>& outCompleted,
      std::vector<WorkInstance>& outFailed);

  void SignalWorkComplete(WorkInstance* work);

  size_t GetPendingRequestsCount();
  size_t GetTotalPendingCount();

  void GetRequestsStats(size_t& queued, size_t& inFlight, size_t& done);

private:
  void transitionQueuedWork();
  void dispatchRequest(WorkInstance* request);
  void stageQueuedWork(std::vector<WorkInstance*>& workNeedingDispatch);

  void onRequestFinished(
      uint16_t responseStatusCode,
      gsl::span<const std::byte> responseBytes,
      const WorkInstance* request);

  WorkInstance* createWorkInstance(TileLoadWork* loadWork);

  // Thread safe members
  std::mutex _requestsLock;
  bool _exitSignaled = false;

  std::map<TileSource, WorkInstance> _ownedWork;

  std::vector<WorkInstance*> _requestQueue;
  std::map<std::string, std::vector<WorkInstance*>> _inFlightRequests;
  std::vector<WorkInstance*> _processingQueue;
  std::vector<WorkInstance*> _failedWork;

  CesiumAsync::AsyncSystem _asyncSystem;

  std::shared_ptr<CesiumAsync::IAssetAccessor> _pAssetAccessor;

  std::shared_ptr<spdlog::logger> _pLogger;

  size_t _maxSimultaneousRequests;
};

} // namespace Cesium3DTilesSelection
