#include <CesiumRasterOverlays/IPrepareRasterOverlayRendererResources.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>

#include <memory>

using namespace CesiumAsync;

namespace CesiumRasterOverlays {

RasterOverlayTile::RasterOverlayTile(
    RasterOverlayTileProvider& tileProvider) noexcept
    : _pTileProvider(&tileProvider),
      _targetScreenPixels(0.0),
      _rectangle(CesiumGeometry::Rectangle(0.0, 0.0, 0.0, 0.0)),
      _tileCredits(),
      _state(LoadState::Placeholder),
      _pImage(nullptr),
      _pRendererResources(nullptr),
      _moreDetailAvailable(MoreDetailAvailable::Unknown) {}

RasterOverlayTile::RasterOverlayTile(
    RasterOverlayTileProvider& tileProvider,
    const glm::dvec2& targetScreenPixels,
    const CesiumGeometry::Rectangle& rectangle) noexcept
    : _pTileProvider(&tileProvider),
      _targetScreenPixels(targetScreenPixels),
      _rectangle(rectangle),
      _tileCredits(),
      _state(LoadState::Unloaded),
      _pImage(nullptr),
      _pRendererResources(nullptr),
      _moreDetailAvailable(MoreDetailAvailable::Unknown) {}

RasterOverlayTile::~RasterOverlayTile() {
  RasterOverlayTileProvider& tileProvider = *this->_pTileProvider;

  tileProvider.removeTile(this);

  const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
      pPrepareRendererResources = tileProvider.getPrepareRendererResources();

  if (pPrepareRendererResources) {
    void* pLoadThreadResult =
        this->getState() == RasterOverlayTile::LoadState::Done
            ? nullptr
            : this->_pRendererResources;
    void* pMainThreadResult =
        this->getState() == RasterOverlayTile::LoadState::Done
            ? this->_pRendererResources
            : nullptr;

    pPrepareRendererResources->freeRaster(
        *this,
        pLoadThreadResult,
        pMainThreadResult);
  }
}

RasterOverlay& RasterOverlayTile::getOverlay() noexcept {
  return this->_pTileProvider->getOwner();
}

/**
 * @brief Returns the {@link RasterOverlay} that created this instance.
 */
const RasterOverlay& RasterOverlayTile::getOverlay() const noexcept {
  return this->_pTileProvider->getOwner();
}

void RasterOverlayTile::loadInMainThread() {
  if (this->getState() != RasterOverlayTile::LoadState::Loaded) {
    return;
  }

  // Do the final main thread raster loading
  RasterOverlayTileProvider& tileProvider = *this->_pTileProvider;
  this->_pRendererResources =
      tileProvider.getPrepareRendererResources()->prepareRasterInMainThread(
          *this,
          this->_pRendererResources);

  this->setState(LoadState::Done);
}

void RasterOverlayTile::setState(LoadState newState) noexcept {
  this->_state = newState;
}

} // namespace CesiumRasterOverlays
