#include "TileUtilities.h"

#include <Cesium3DTilesSelection/RasterizedPolygonsTileExcluder.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <CesiumRasterOverlays/RasterizedPolygonsOverlay.h>
#include <CesiumUtility/IntrusivePointer.h>

using namespace Cesium3DTilesSelection;

RasterizedPolygonsTileExcluder::RasterizedPolygonsTileExcluder(
    const CesiumUtility::IntrusivePointer<
        const CesiumRasterOverlays::RasterizedPolygonsOverlay>&
        pOverlay) noexcept
    : _pOverlay(pOverlay) {}

bool RasterizedPolygonsTileExcluder::shouldExclude(
    const Tile& tile) const noexcept {
  if (this->_pOverlay->getInvertSelection()) {
    return Cesium3DTilesSelection::CesiumImpl::outsidePolygons(
        tile.getBoundingVolume(),
        this->_pOverlay->getPolygons(),
        this->_pOverlay->getEllipsoid());
  } else {
    return Cesium3DTilesSelection::CesiumImpl::withinPolygons(
        tile.getBoundingVolume(),
        this->_pOverlay->getPolygons(),
        this->_pOverlay->getEllipsoid());
  }
}
