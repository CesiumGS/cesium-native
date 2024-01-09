#pragma once

#include "Tile.h"
#include "TilesetContentLoader.h"

namespace Cesium3DTilesSelection {
class TilesetContentManager;
class TilesetMetadata;

typedef std::variant<Tile*, RasterMappedTo3DTile*> TileWorkRef;

enum class TileLoadPriorityGroup {
  /**
   * @brief Low priority tiles that aren't needed right now, but
   * are being preloaded for the future.
   */
  Preload = 0,

  /**
   * @brief Medium priority tiles that are needed to render the current view
   * the appropriate level-of-detail.
   */
  Normal = 1,

  /**
   * @brief High priority tiles that are causing extra detail to be rendered
   * in the scene, potentially creating a performance problem and aliasing
   * artifacts.
   */
  Urgent = 2
};

struct TileLoadWork {
  TileWorkRef workRef;

  RequestData requestData;
  TileProcessingCallback tileCallback;
  RasterProcessingCallback rasterCallback;

  std::vector<CesiumGeospatial::Projection> projections;
  TileLoadPriorityGroup group;
  double priority;

  ResponseDataMap responsesByUrl;

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

  void QueueRequestWork(
      const std::vector<TileLoadWork>& work,
      const std::vector<TileLoadWork>& passThroughWork,
      size_t maxSimultaneousRequests);

  void PassThroughWork(const std::vector<TileLoadWork>& work);

  void TakeCompletedWork(
      size_t maxCount,
      std::vector<TileLoadWork>& outCompleted,
      std::vector<TileLoadWork>& outFailed);

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

  // Thread safe members
  std::mutex _requestsLock;
  bool _exitSignaled = false;
  std::vector<TileLoadWork> _queuedWork;
  std::map<std::string, std::vector<TileLoadWork>> _inFlightWork;
  std::vector<TileLoadWork> _doneWork;
  std::vector<TileLoadWork> _failedWork;

  CesiumAsync::AsyncSystem _asyncSystem;

  std::shared_ptr<CesiumAsync::IAssetAccessor> _pAssetAccessor;

  std::shared_ptr<spdlog::logger> _pLogger;

  size_t _maxSimultaneousRequests;
};

} // namespace Cesium3DTilesSelection
