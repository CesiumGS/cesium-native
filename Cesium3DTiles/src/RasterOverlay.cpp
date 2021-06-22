#include "Cesium3DTiles/RasterOverlay.h"
#include "Cesium3DTiles/RasterOverlayCollection.h"
#include "Cesium3DTiles/RasterOverlayTileProvider.h"
#include "Cesium3DTiles/spdlog-cesium.h"

using namespace CesiumAsync;

namespace {
class PlaceholderTileProvider
    : public Cesium3DTiles::RasterOverlayTileProvider {
public:
  PlaceholderTileProvider(
      Cesium3DTiles::RasterOverlay& owner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<IAssetAccessor>& pAssetAccessor) noexcept
      : Cesium3DTiles::RasterOverlayTileProvider(
            owner,
            asyncSystem,
            pAssetAccessor) {}

  virtual CesiumAsync::Future<Cesium3DTiles::LoadedRasterOverlayImage>
  loadTileImage(
      const CesiumGeometry::QuadtreeTileID& /* tileID */) const override {
    return this->getAsyncSystem()
        .createResolvedFuture<Cesium3DTiles::LoadedRasterOverlayImage>({});
  }
};
} // namespace

namespace Cesium3DTiles {

RasterOverlay::RasterOverlay(const RasterOverlayOptions& options)
    : _pPlaceholder(),
      _pTileProvider(),
      _pSelf(),
      _isLoadingTileProvider(false),
      _options(options) {}

RasterOverlay::~RasterOverlay() {
  // explicitly set those to nullptr, because RasterOverlayTile destructor
  // relies on nullptr check to retrieve the correct provider. If we let
  // unique_ptr() destructor called implicitly, pTileProvider will get
  // destructed first, but it will never set to be nullptr. So when
  // _pPlaceholder is destroyed, its _pPlaceholder and _tiles member destructor
  // will retrieve _pTileProvider instead of _pPlaceholder and it crashes
  this->_pTileProvider = nullptr;
  this->_pPlaceholder = nullptr;
}

RasterOverlayTileProvider* RasterOverlay::getTileProvider() noexcept {
  return this->_pTileProvider ? this->_pTileProvider.get()
                              : this->_pPlaceholder.get();
}

const RasterOverlayTileProvider*
RasterOverlay::getTileProvider() const noexcept {
  return this->_pTileProvider ? this->_pTileProvider.get()
                              : this->_pPlaceholder.get();
}

void RasterOverlay::createTileProvider(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& pCreditSystem,
    const std::shared_ptr<IPrepareRendererResources>& pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger) {

  if (this->_pPlaceholder) {
    return;
  }

  CESIUM_TRACE_NEW_ASYNC();
  CESIUM_TRACE_BEGIN("createTileProvider");

  this->_pPlaceholder = std::make_unique<PlaceholderTileProvider>(
      *this,
      asyncSystem,
      pAssetAccessor);

  this->_isLoadingTileProvider = true;

  this->createTileProvider(
          asyncSystem,
          pAssetAccessor,
          pCreditSystem,
          pPrepareRendererResources,
          pLogger,
          this)
      .thenInMainThread(
          [this](std::unique_ptr<RasterOverlayTileProvider> pProvider) {
            this->_pTileProvider = std::move(pProvider);
            this->_isLoadingTileProvider = false;
            CESIUM_TRACE_END("createTileProvider");
          })
      .catchInMainThread([this, pLogger](const std::exception& e) {
        SPDLOG_LOGGER_ERROR(
            pLogger,
            "Exception while creating tile provider: {0}",
            e.what());
        this->_pTileProvider.reset();
        this->_isLoadingTileProvider = false;
        CESIUM_TRACE_END("createTileProvider");
      });
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
  if (this->_isLoadingTileProvider) {
    // Loading, so it's not safe to unload yet.
    return;
  }

  RasterOverlayTileProvider* pTileProvider = this->getTileProvider();
  if (!pTileProvider || pTileProvider->getNumberOfTilesLoading() == 0) {
    // No tile provider or no tiles loading, so it's safe to delete!
    this->_pSelf.reset();
  }
}

} // namespace Cesium3DTiles
