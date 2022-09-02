#include "Cesium3DTilesSelection/RasterizedPolygonsTileExcluder.h"

#include "Cesium3DTilesSelection/RasterizedPolygonsOverlay.h"
#include "Cesium3DTilesSelection/Tile.h"
#include "TileUtilities.h"

using namespace Cesium3DTilesSelection;

RasterizedPolygonsTileExcluder::RasterizedPolygonsTileExcluder(
    const CesiumUtility::IntrusivePointer<const RasterizedPolygonsOverlay>&
        pOverlay) noexcept
    : _pOverlay(pOverlay) {}

bool RasterizedPolygonsTileExcluder::shouldExclude(
    const Tile& tile) const noexcept {
  if (this->_pOverlay->getInvertSelection()) {
    return Cesium3DTilesSelection::CesiumImpl::outsidePolygons(
        tile.getBoundingVolume(),
        this->_pOverlay->getPolygons());
  } else {
    return Cesium3DTilesSelection::CesiumImpl::withinPolygons(
        tile.getBoundingVolume(),
        this->_pOverlay->getPolygons());
  }
}
