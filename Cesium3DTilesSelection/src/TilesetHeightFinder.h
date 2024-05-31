#include <Cesium3DTilesSelection/Tileset.h>
#include <CesiumGeometry/Ray.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGltfContent/GltfUtilities.h>

#include <vector>

namespace Cesium3DTilesSelection {

class TilesetHeightFinder {
  friend class Tileset;

  struct RayIntersect {
    CesiumGeometry::Ray ray;
    CesiumGeospatial::Cartographic coordinate;
    CesiumGltfContent::GltfUtilities::HitResult hitResult;
    std::vector<Tile*> tilesLoading;
  };

  struct HeightRequests {
    std::vector<RayIntersect> rayIntersects;
    uint32_t numRaysDone;
    CesiumAsync::Promise<std::vector<Tileset::HeightResult>> promise;
  };

  TilesetHeightFinder(
      Cesium3DTilesSelection::Tileset* pTileset,
      CesiumUtility::IntrusivePointer<TilesetContentManager>
          pTilesetContentManager)
      : _pTileset(pTileset), _pTilesetContentManager(pTilesetContentManager){};

  CesiumAsync::Future<std::vector<Tileset::HeightResult>>
  _getHeightsAtCoordinates(
      const std::vector<CesiumGeospatial::Cartographic>& coordinates);

  bool _loadTileIfNeeded(Tile* pTile);

  void _intersectVisibleTile(Tile* pTile, RayIntersect& rayInfo);

  void _findAndIntersectVisibleTiles(
      Tile* pTile,
      RayIntersect& rayInfo,
      std::vector<Tile*>& newTilesToLoad);

  void _processTilesLoadingQueue(RayIntersect& rayInfo);

  void _processHeightRequests();

  std::vector<HeightRequests> _heightRequests;
  Cesium3DTilesSelection::Tileset* _pTileset;
  CesiumUtility::IntrusivePointer<TilesetContentManager>
      _pTilesetContentManager;
};
} // namespace Cesium3DTilesSelection
