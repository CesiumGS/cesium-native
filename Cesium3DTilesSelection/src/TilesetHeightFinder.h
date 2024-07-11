#include <Cesium3DTilesSelection/Tileset.h>
#include <CesiumGeometry/Ray.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGltfContent/GltfUtilities.h>

#include <vector>

namespace Cesium3DTilesSelection {

class TilesetHeightFinder {
  friend class Tileset;

  struct RayIntersect {
    CesiumGeospatial::Cartographic inputCoordinate;
    CesiumGeometry::Ray ray;
    CesiumGltfContent::GltfUtilities::IntersectResult intersectResult;
  };

  struct HeightRequests {
    std::vector<RayIntersect> rayIntersects;
    uint32_t numRaysDone;
    CesiumAsync::Promise<Tileset::HeightResults> promise;
  };

  TilesetHeightFinder(
      Cesium3DTilesSelection::Tileset* pTileset,
      CesiumUtility::IntrusivePointer<TilesetContentManager>
          pTilesetContentManager)
      : _pTileset(pTileset), _pTilesetContentManager(pTilesetContentManager){};

  CesiumAsync::Future<Tileset::HeightResults> _getHeightsAtCoordinates(
      const std::vector<CesiumGeospatial::Cartographic>& coordinates);

  bool _loadTileIfNeeded(Tile* pTile);

  void _intersectVisibleTile(Tile* pTile, RayIntersect& rayInfo);

  void _findCandidateTiles(
      Tile* pTile,
      RayIntersect& rayInfo,
      std::vector<Tile*>& tilesNeedingLoading);

  void _processHeightRequests(std::vector<Tile*>& tilesNeedingLoading);

  std::vector<HeightRequests> _heightRequests;
  Cesium3DTilesSelection::Tileset* _pTileset;
  CesiumUtility::IntrusivePointer<TilesetContentManager>
      _pTilesetContentManager;
};
} // namespace Cesium3DTilesSelection
