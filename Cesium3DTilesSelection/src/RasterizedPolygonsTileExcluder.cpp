#include "Cesium3DTilesSelection/RasterizedPolygonsTileExcluder.h"

#include "Cesium3DTilesSelection/RasterizedPolygonsOverlay.h"
#include "Cesium3DTilesSelection/Tile.h"
#include "TileUtilities.h"

using namespace Cesium3DTilesSelection;

RasterizedPolygonsTileExcluder::RasterizedPolygonsTileExcluder(
    const RasterizedPolygonsOverlay& overlay) noexcept
    : _pOverlay(&overlay) {}

bool RasterizedPolygonsTileExcluder::shouldExclude(
    const Tile& tile) const noexcept {
  if (this->_pOverlay->getFlipSelection()) {
    return Cesium3DTilesSelection::CesiumImpl::outsidePolygons(
        tile.getBoundingVolume(),
        this->_pOverlay->getPolygons());
  } else {
    return Cesium3DTilesSelection::CesiumImpl::withinPolygons(
        tile.getBoundingVolume(),
        this->_pOverlay->getPolygons());
  }
}
