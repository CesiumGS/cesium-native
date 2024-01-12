#pragma once

#include "Tile.h"
#include "TilesetContentLoader.h"

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

typedef std::variant<TileProcessingData, RasterProcessingData> ProcessingData;

struct TileLoadWork {
  RequestData requestData;

  ProcessingData processingData;

  std::vector<CesiumGeospatial::Projection> projections;
  TileLoadPriorityGroup group;
  double priority;

  ResponseDataMap responsesByUrl;

  std::vector<TileLoadWork> childWork;

  bool operator<(const TileLoadWork& rhs) const noexcept {
    if (this->group == rhs.group)
      return this->priority < rhs.priority;
    else
      return this->group > rhs.group;
  }
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

  void SetMaxSimultaneousRequests(size_t max);

  void QueueBatch(
      const std::vector<TileLoadWork*>& requestWork,
      const std::vector<TileLoadWork*>& processingWork);

  void QueueSingleRequest(const TileLoadWork& requestWork);

  void TakeProcessingWork(
      size_t maxCount,
      std::vector<TileLoadWork>& outCompleted,
      std::vector<TileLoadWork>& outFailed);

  void SignalWorkComplete(const TileLoadWork& work);

  size_t GetPendingRequestsCount();
  size_t GetTotalPendingCount();

  void GetRequestsStats(size_t& queued, size_t& inFlight, size_t& done);

private:
  void transitionQueuedWork();
  void dispatchRequest(TileLoadWork& request);
  void stageQueuedWork(std::vector<TileLoadWork>& workNeedingDispatch);

  void onRequestFinished(
      uint16_t responseStatusCode,
      gsl::span<const std::byte> responseBytes,
      const TileLoadWork& request);

  bool isProcessingUnique(
      const TileLoadWork& newRequest,
      const TileLoadWork& existingRequest);

  bool isRequestAlreadyQueued(const TileLoadWork& newRequest);
  bool isRequestAlreadyInFlight(const TileLoadWork& newRequest);
  bool isWorkAlreadyProcessing(const TileLoadWork& newProcessing);

  void eraseMatchingChildWork(
      const TileLoadWork& work,
      std::vector<TileLoadWork>& childWork);

  // Thread safe members
  std::mutex _requestsLock;
  bool _exitSignaled = false;
  std::vector<TileLoadWork> _requestQueue;
  std::map<std::string, std::vector<TileLoadWork>> _inFlightRequests;
  std::vector<TileLoadWork> _processingQueue;
  std::vector<TileLoadWork> _failedWork;

  CesiumAsync::AsyncSystem _asyncSystem;

  std::shared_ptr<CesiumAsync::IAssetAccessor> _pAssetAccessor;

  std::shared_ptr<spdlog::logger> _pLogger;

  size_t _maxSimultaneousRequests;
};

} // namespace Cesium3DTilesSelection
