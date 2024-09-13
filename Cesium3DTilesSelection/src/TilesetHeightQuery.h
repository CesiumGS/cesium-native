#pragma once

#include <CesiumAsync/Future.h>
#include <CesiumAsync/Promise.h>
#include <CesiumGeometry/Ray.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGltfContent/GltfUtilities.h>

#include <list>
#include <set>
#include <string>
#include <vector>

namespace Cesium3DTilesSelection {

class Tile;
class TilesetContentManager;
struct TilesetOptions;
struct SampleHeightResult;

class TilesetHeightQuery {
public:
  TilesetHeightQuery(
      const CesiumGeospatial::Cartographic& position,
      const CesiumGeospatial::Ellipsoid& ellipsoid);

  CesiumGeospatial::Cartographic inputCoordinate;
  CesiumGeometry::Ray ray;
  CesiumGltfContent::GltfUtilities::IntersectResult intersectResult;

  // The query ray might intersect these non-leaf tiles that are still relevant
  // because of additive refinement.
  std::vector<Tile*> additiveCandidateTiles;

  // The current set of leaf tiles whose bounding volume the query ray passes
  // through.
  std::vector<Tile*> candidateTiles;

  // The previous set of leaf files. Swapping `candidateTiles` and
  // `previousCandidateTiles` each frame allows us to avoid a heap allocation
  // for a new vector each frame.
  std::vector<Tile*> previousCandidateTiles;

  void intersectVisibleTile(Tile* pTile);

  void findCandidateTiles(Tile* pTile, std::vector<std::string>& warnings);
};

struct TilesetHeightRequest {
  static void processHeightRequests(
      TilesetContentManager& contentManager,
      const TilesetOptions& options,
      std::list<TilesetHeightRequest>& heightRequests,
      std::vector<Tile*>& heightQueryLoadQueue);

  std::vector<TilesetHeightQuery> queries;
  CesiumAsync::Promise<SampleHeightResult> promise;

  bool tryCompleteHeightRequest(
      TilesetContentManager& contentManager,
      const TilesetOptions& options,
      std::set<Tile*>& tilesNeedingLoading);
};

} // namespace Cesium3DTilesSelection
