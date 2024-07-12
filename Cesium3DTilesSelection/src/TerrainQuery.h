#pragma once

#include <Cesium3DTilesSelection/Tileset.h>
#include <CesiumGeometry/Ray.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGltfContent/GltfUtilities.h>

#include <set>
#include <vector>

namespace Cesium3DTilesSelection {

class TerrainQuery {
public:
  CesiumGeospatial::Cartographic inputCoordinate;
  CesiumGeometry::Ray ray;
  CesiumGltfContent::GltfUtilities::IntersectResult intersectResult = {};
  std::vector<Tile*> candidateTiles = {};

  void intersectVisibleTile(Tile* pTile);

  void findCandidateTiles(Tile* pTile);
};

} // namespace Cesium3DTilesSelection
