#include "Cesium3DTilesSelection/RasterizedPolygonsTileExcluder.h"
#include "Cesium3DTilesSelection/RasterizedPolygonsOverlay.h"
#include "Cesium3DTilesSelection/Tile.h"
#include "TileUtilities.h"

using namespace Cesium3DTilesSelection;

RasterizedPolygonsTileExcluder::RasterizedPolygonsTileExcluder(
    const RasterizedPolygonsOverlay& overlay)
    : _pOverlay(&overlay) {}

bool RasterizedPolygonsTileExcluder::shouldExclude(const Tile& tile) const {
  return Cesium3DTilesSelection::Impl::withinPolygons(
      tile.getBoundingVolume(),
      this->_pOverlay->getPolygons());
}
