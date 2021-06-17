#include "Cesium3DTiles/RasterMappedTo3DTile.h"
#include "Cesium3DTiles/IPrepareRendererResources.h"
#include "Cesium3DTiles/RasterOverlayCollection.h"
#include "Cesium3DTiles/RasterOverlayTileProvider.h"
#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/TileID.h"
#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTiles/TilesetExternals.h"
#include "TileUtilities.h"
#include <optional>
#include <variant>

namespace Cesium3DTiles {

RasterMappedTo3DTile::RasterMappedTo3DTile(
    const CesiumUtility::IntrusivePointer<RasterOverlayTile>& pRasterTile,
    const CesiumGeometry::Rectangle& textureCoordinateRectangle)
    : _pLoadingTile(pRasterTile),
      _pReadyTile(nullptr),
      _textureCoordinateID(0),
      _textureCoordinateRectangle(textureCoordinateRectangle),
      _translation(0.0, 0.0),
      _scale(1.0, 1.0),
      _state(AttachmentState::Unattached),
      _originalFailed(false) {}

void RasterMappedTo3DTile::detachFromTile(Tile& tile) noexcept {
  if (this->getState() == AttachmentState::Unattached) {
    return;
  }

  if (!this->_pReadyTile) {
    return;
  }

  TilesetExternals& externals = tile.getTileset()->getExternals();
  externals.pPrepareRendererResources->detachRasterInMainThread(
      tile,
      this->getTextureCoordinateID(),
      *this->_pReadyTile,
      this->_pReadyTile->getRendererResources(),
      this->getTextureCoordinateRectangle());

  this->_state = AttachmentState::Unattached;
}

void RasterMappedTo3DTile::computeTranslationAndScale(Tile& tile) {
  if (!this->_pReadyTile) {
    return;
  }

  const CesiumGeospatial::GlobeRectangle* pRectangle =
      Cesium3DTiles::Impl::obtainGlobeRectangle(&tile.getBoundingVolume());
  if (!pRectangle) {
    return;
  }

  RasterOverlayTileProvider& tileProvider =
      *this->_pReadyTile->getOverlay().getTileProvider();
  CesiumGeometry::Rectangle geometryRectangle =
      projectRectangleSimple(tileProvider.getProjection(), *pRectangle);
  CesiumGeometry::Rectangle imageryRectangle =
      this->_pReadyTile->getImageryRectangle();

  double terrainWidth = geometryRectangle.computeWidth();
  double terrainHeight = geometryRectangle.computeHeight();

  double scaleX = terrainWidth / imageryRectangle.computeWidth();
  double scaleY = terrainHeight / imageryRectangle.computeHeight();
  this->_translation = glm::dvec2(
      (scaleX * (geometryRectangle.minimumX - imageryRectangle.minimumX)) /
          terrainWidth,
      (scaleY * (geometryRectangle.minimumY - imageryRectangle.minimumY)) /
          terrainHeight);
  this->_scale = glm::dvec2(scaleX, scaleY);
}

} // namespace Cesium3DTiles
