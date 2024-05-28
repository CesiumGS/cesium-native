#include <Cesium3DTilesSelection/Tileset.h>
#include <CesiumGeometry/Ray.h>
#include <CesiumGeospatial/Cartographic.h>

#include <vector>

namespace Cesium3DTilesSelection {

class TilesetHeightFinder {
  friend class Tileset;

  struct RayInfo {
    CesiumGeometry::Ray ray;
    CesiumGeospatial::Cartographic coordinate;
    double tMin;
    std::vector<Tile*> tilesLoading;
  };

  struct HeightRequests {
    RayInfo current;
    uint32_t numRaysDone;
    std::vector<CesiumGeospatial::Cartographic> coordinates = {
        CesiumGeospatial::Cartographic(0.0, 0.0)};
    CesiumAsync::Promise<std::vector<CesiumGeospatial::Cartographic>> promise;
  };

  TilesetHeightFinder(
      Cesium3DTilesSelection::Tileset* pTileset,
      CesiumUtility::IntrusivePointer<TilesetContentManager>
          pTilesetContentManager)
      : _pTileset(pTileset), _pTilesetContentManager(pTilesetContentManager){};

  CesiumAsync::Future<std::vector<CesiumGeospatial::Cartographic>>
  _getHeightsAtCoordinates(
      const std::vector<CesiumGeospatial::Cartographic>& coordinates);

  bool _loadTileIfNeeded(Tile* pTile);

  void _intersectVisibleTile(Tile* pTile, RayInfo& rayInfo);

  void _findAndIntersectVisibleTiles(
      Tile* pTile,
      RayInfo& rayInfo,
      std::vector<Tile*>& newTilesToLoad);

  void _processTilesLoadingQueue(RayInfo& rayInfo);

  void _processHeightRequests();

  std::vector<HeightRequests> _heightRequests;
  Cesium3DTilesSelection::Tileset* _pTileset;
  CesiumUtility::IntrusivePointer<TilesetContentManager>
      _pTilesetContentManager;
};
} // namespace Cesium3DTilesSelection
