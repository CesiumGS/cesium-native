#pragma once

#include <CesiumGeometry/Ray.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGltfContent/GltfUtilities.h>

#include <string>
#include <vector>

namespace Cesium3DTilesSelection {

class Tile;

class TilesetHeightQuery {
public:
  CesiumGeospatial::Cartographic inputCoordinate;
  CesiumGeometry::Ray ray;
  CesiumGltfContent::GltfUtilities::IntersectResult intersectResult = {};

  // The query ray might intersect these non-leaf tiles that are still relevant
  // because of additive refinement.
  std::vector<Tile*> additiveCandidateTiles = {};

  // The current set of leaf tiles whose bounding volume the query ray passes
  // through.
  std::vector<Tile*> candidateTiles = {};

  // The previous set of leaf files. Swapping `candidateTiles` and
  // `previousCandidateTiles` each frame allows us to avoid a heap allocation
  // for a new vector each frame.
  std::vector<Tile*> previousCandidateTiles = {};

  void intersectVisibleTile(Tile* pTile);

  void findCandidateTiles(Tile* pTile, std::vector<std::string>& warnings);
};

} // namespace Cesium3DTilesSelection
