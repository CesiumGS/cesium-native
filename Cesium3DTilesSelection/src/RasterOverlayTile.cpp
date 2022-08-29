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
      _state(LoadState::Placeholder),
      _image(),
      _pRendererResources(nullptr),
      _references(0),
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
      _image(),
      _pRendererResources(nullptr),
      _references(0),
      _moreDetailAvailable(MoreDetailAvailable::Unknown) {}

RasterOverlayTile::~RasterOverlayTile() {
  RasterOverlayTileProvider& tileProvider = *this->_pTileProvider;
  const std::shared_ptr<IPrepareRendererResources>& pPrepareRendererResources =
      tileProvider.getPrepareRendererResources();

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

void RasterOverlayTile::addReference() noexcept { ++this->_references; }

void RasterOverlayTile::releaseReference() noexcept {
  assert(this->_references > 0);
  const uint32_t references = --this->_references;
  if (references == 0) {
    this->_pTileProvider->removeTile(this);
  }
}

void RasterOverlayTile::setState(LoadState newState) noexcept {
  this->_state = newState;
}
} // namespace Cesium3DTilesSelection
