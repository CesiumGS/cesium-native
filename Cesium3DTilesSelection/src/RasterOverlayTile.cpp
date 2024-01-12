#include "Cesium3DTilesSelection/RasterOverlayTile.h"

#include "Cesium3DTilesSelection/IPrepareRendererResources.h"
#include "Cesium3DTilesSelection/RasterOverlay.h"
#include "Cesium3DTilesSelection/RasterOverlayTileProvider.h"
#include "Cesium3DTilesSelection/TilesetExternals.h"

#include <CesiumAsync/IAssetResponse.h>
#include <CesiumAsync/ITaskProcessor.h>
#include <CesiumUtility/joinToString.h>

using namespace CesiumAsync;

namespace Cesium3DTilesSelection {

RasterOverlayTile::RasterOverlayTile(
    RasterOverlayTileProvider& tileProvider) noexcept
    : _pTileProvider(&tileProvider),
      _targetScreenPixels(0.0),
      _rectangle(CesiumGeometry::Rectangle(0.0, 0.0, 0.0, 0.0)),
      _tileCredits(),
      _state(RasterLoadState::Placeholder),
      _image(),
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
      _state(RasterLoadState::Unloaded),
      _image(),
      _pRendererResources(nullptr),
      _moreDetailAvailable(MoreDetailAvailable::Unknown) {}

RasterOverlayTile::~RasterOverlayTile() {
  RasterOverlayTileProvider& tileProvider = *this->_pTileProvider;

  tileProvider.removeTile(this);

  const std::shared_ptr<IPrepareRendererResources>& pPrepareRendererResources =
      tileProvider.getPrepareRendererResources();

  if (pPrepareRendererResources) {
    void* pLoadThreadResult = this->getState() == RasterLoadState::Done
                                  ? nullptr
                                  : this->_pRendererResources;
    void* pMainThreadResult = this->getState() == RasterLoadState::Done
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
  if (this->getState() != RasterLoadState::Loaded) {
    return;
  }

  // Do the final main thread raster loading
  RasterOverlayTileProvider& tileProvider = *this->_pTileProvider;
  this->_pRendererResources =
      tileProvider.getPrepareRendererResources()->prepareRasterInMainThread(
          *this,
          this->_pRendererResources);

  this->setState(RasterLoadState::Done);
}

void RasterOverlayTile::setState(RasterLoadState newState) noexcept {
  this->_state = newState;
}

} // namespace Cesium3DTilesSelection
