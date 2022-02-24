#include "Cesium3DTilesSelection/RasterOverlay.h"

#include "Cesium3DTilesSelection/RasterOverlayCollection.h"
#include "Cesium3DTilesSelection/RasterOverlayLoadFailureDetails.h"
#include "Cesium3DTilesSelection/RasterOverlayTileProvider.h"
#include "Cesium3DTilesSelection/spdlog-cesium.h"

using namespace CesiumAsync;
using namespace Cesium3DTilesSelection;

namespace {
class PlaceholderTileProvider : public RasterOverlayTileProvider {
public:
  PlaceholderTileProvider(
      RasterOverlay& owner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<IAssetAccessor>& pAssetAccessor) noexcept
      : RasterOverlayTileProvider(owner, asyncSystem, pAssetAccessor) {}

  virtual CesiumAsync::Future<LoadedRasterOverlayImage>
  loadTileImage(RasterOverlayTile& /* overlayTile */) override {
    return this->getAsyncSystem()
        .createResolvedFuture<LoadedRasterOverlayImage>({});
  }
};
} // namespace

RasterOverlay::RasterOverlay(
    const std::string& name,
    const RasterOverlayOptions& options)
    : _name(name),
      _pPlaceholder(),
      _pSelf(),
      _options(options),
      _loadingTileProvider() {}

RasterOverlay::~RasterOverlay() {
  // explicitly set those to nullptr, because RasterOverlayTile destructor
  // relies on nullptr check to retrieve the correct provider. If we let
  // unique_ptr() destructor called implicitly, pTileProvider will get
  // destructed first, but it will never set to be nullptr. So when
  // _pPlaceholder is destroyed, its _pPlaceholder and _tiles member destructor
  // will retrieve _pTileProvider instead of _pPlaceholder and it crashes
  this->_loadingTileProvider.reset();
  this->_pPlaceholder.reset();
}

RasterOverlayTileProvider* RasterOverlay::getTileProvider() noexcept {
  RasterOverlayTileProvider* pResult =
      this->_loadingTileProvider && this->_loadingTileProvider->isReady()
          ? this->_loadingTileProvider->wait().get()
          : nullptr;
  if (!pResult) {
    pResult = this->_pPlaceholder.get();
  }
  return pResult;
}

const RasterOverlayTileProvider*
RasterOverlay::getTileProvider() const noexcept {
  const RasterOverlayTileProvider* pResult =
      this->_loadingTileProvider && this->_loadingTileProvider->isReady()
          ? this->_loadingTileProvider->wait().get()
          : nullptr;
  if (!pResult) {
    pResult = this->_pPlaceholder.get();
  }
  return pResult;
}

CesiumAsync::SharedFuture<std::unique_ptr<RasterOverlayTileProvider>>
RasterOverlay::loadTileProvider(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& pCreditSystem,
    const std::shared_ptr<IPrepareRendererResources>& pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger) {

  if (this->_loadingTileProvider) {
    return *this->_loadingTileProvider;
  }

  CESIUM_TRACE_BEGIN_IN_TRACK("createTileProvider");

  this->_pPlaceholder = std::make_unique<PlaceholderTileProvider>(
      *this,
      asyncSystem,
      pAssetAccessor);

  auto future =
      this->createTileProvider(
              asyncSystem,
              pAssetAccessor,
              pCreditSystem,
              pPrepareRendererResources,
              pLogger,
              this)
          .thenInMainThread([](std::unique_ptr<RasterOverlayTileProvider>&&
                                   pProvider) noexcept {
            CESIUM_TRACE_END_IN_TRACK("createTileProvider");
            return std::move(pProvider);
          })
          .catchInMainThread([pLogger](const std::exception& e) {
            SPDLOG_LOGGER_ERROR(
                pLogger,
                "Exception while creating tile provider: {0}",
                e.what());
            CESIUM_TRACE_END_IN_TRACK("createTileProvider");
            return std::unique_ptr<RasterOverlayTileProvider>();
          })
          .share();

  this->_loadingTileProvider.emplace(std::move(future));

  return *this->_loadingTileProvider;
}

void RasterOverlay::destroySafely(
    std::unique_ptr<RasterOverlay>&& pOverlay) noexcept {
  if (pOverlay) {
    this->_pSelf = std::move(pOverlay);
  } else if (!this->isBeingDestroyed()) {
    // Ownership was not transferred and we're not already being destroyed,
    // so nothing more to do.
    return;
  }

  // Check if it's safe to delete this object yet.
  if (this->_loadingTileProvider && !this->_loadingTileProvider->isReady()) {
    // Loading, so it's not safe to unload yet.
    return;
  }

  RasterOverlayTileProvider* pTileProvider = this->getTileProvider();
  if (!pTileProvider || pTileProvider->getNumberOfTilesLoading() == 0) {
    // No tile provider or no tiles loading, so it's safe to delete!
    this->_pSelf.reset();
  }
}

void RasterOverlay::reportError(
    const AsyncSystem& asyncSystem,
    const std::shared_ptr<spdlog::logger>& pLogger,
    RasterOverlayLoadFailureDetails&& errorDetails) {
  SPDLOG_LOGGER_ERROR(pLogger, errorDetails.message);
  if (this->getOptions().loadErrorCallback) {
    asyncSystem.runInMainThread(
        [this, errorDetails = std::move(errorDetails)]() mutable {
          this->getOptions().loadErrorCallback(std::move(errorDetails));
        });
  }
}
