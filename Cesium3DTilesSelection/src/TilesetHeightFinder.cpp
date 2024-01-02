#include "TilesetHeightFinder.h"

#include "TileUtilities.h"
#include "TilesetContentManager.h"

#include <CesiumGeometry/IntersectionTests.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGltfContent/GltfUtilities.h>

using namespace Cesium3DTilesSelection;
using namespace CesiumGeospatial;
using namespace CesiumGeometry;
using namespace CesiumUtility;
using namespace CesiumAsync;

namespace {
bool boundingVolumeContainsCoordinate(
    const BoundingVolume& boundingVolume,
    const Ray& ray,
    const Cartographic& coordinate) {
  struct Operation {
    const Ray& ray;
    const Cartographic& coordinate;

    bool operator()(const OrientedBoundingBox& boundingBox) noexcept {
      double t;
      return IntersectionTests::rayOBBParametric(ray, boundingBox, t);
    }

    bool operator()(const BoundingRegion& boundingRegion) noexcept {
      return boundingRegion.getRectangle().contains(coordinate);
    }

    bool operator()(const BoundingSphere& boundingSphere) noexcept {
      double t;
      return IntersectionTests::raySphereParametric(ray, boundingSphere, t);
    }

    bool operator()(
        const BoundingRegionWithLooseFittingHeights& boundingRegion) noexcept {
      return boundingRegion.getBoundingRegion().getRectangle().contains(
          coordinate);
    }

    bool operator()(const S2CellBoundingVolume& s2Cell) noexcept {
      return s2Cell.computeBoundingRegion().getRectangle().contains(coordinate);
    }
  };

  return std::visit(Operation{ray, coordinate}, boundingVolume);
}

Ray createRay(Cartographic cartographic) {
  cartographic.height = 100000.0;
  return Ray(
      Ellipsoid::WGS84.cartographicToCartesian(cartographic),
      -Ellipsoid::WGS84.geodeticSurfaceNormal(cartographic));
}
} // namespace

bool TilesetHeightFinder::_loadTileIfNeeded(Tile* pTile) {
  if (pTile->getChildren().size() != 0) {
    return false;
  }
  const TilesetOptions& options = _pTileset->getOptions();
  switch (pTile->getState()) {
  case TileLoadState::Unloaded:
  case TileLoadState::FailedTemporarily:
    if (_pTilesetContentManager->getNumberOfTilesLoading() <
        static_cast<int32_t>(options.maximumSimultaneousTileLoads))
      _pTilesetContentManager->loadTileContent(*pTile, options);
    return true;
  case TileLoadState::ContentLoading:
  case TileLoadState::Unloading:
    return true;
  case TileLoadState::ContentLoaded:
  case TileLoadState::Done:
  case TileLoadState::Failed:
    return false;
  }
  return false;
}

void TilesetHeightFinder::_intersectLeafTile(Tile* pTile, RayInfo& rayInfo) {
  const Ray& ray = rayInfo.ray;
  double& tMin = rayInfo.tMin;

  TileRenderContent* pRenderContent = pTile->getContent().getRenderContent();
  if (pRenderContent) {
    CesiumGltf::Model& gltf = pRenderContent->getModel();
    double t;
    if (CesiumGltfContent::GltfUtilities::intersectRayGltfModelParametric(
            ray,
            gltf,
            t,
            true,
            pTile->getTransform()) &&
        t >= 0) {
      tMin = tMin < 0 ? t : std::min(tMin, t);
    }
  }
}

void TilesetHeightFinder::_findAndIntersectLeafTiles(
    Tile* pTile,
    RayInfo& rayInfo,
    std::vector<Tile*>& newTilesToLoad) {
  if (pTile->getState() == TileLoadState::Failed) {
    return;
  }
  if (pTile->getChildren().empty()) {
    _intersectLeafTile(pTile, rayInfo);
  } else {
    for (Tile& child : pTile->getChildren()) {
      if (!boundingVolumeContainsCoordinate(
              child.getBoundingVolume(),
              rayInfo.ray,
              rayInfo.coordinate))
        continue;
      if (_loadTileIfNeeded(&child)) {
        newTilesToLoad.push_back(&child);
      } else {
        _findAndIntersectLeafTiles(&child, rayInfo, newTilesToLoad);
      }
    }
  }
}

void TilesetHeightFinder::_processTilesLoadingQueue(RayInfo& rayInfo) {
  std::vector<Tile*> newTilesToLoad;
  for (auto it = rayInfo.tilesLoading.begin();
       it != rayInfo.tilesLoading.end();) {
    Tile* pTile = *it;
    if (_loadTileIfNeeded(pTile)) {
      ++it;
    } else {
      it = rayInfo.tilesLoading.erase(it);
      _findAndIntersectLeafTiles(pTile, rayInfo, newTilesToLoad);
    }
  }
  rayInfo.tilesLoading.insert(
      rayInfo.tilesLoading.end(),
      newTilesToLoad.begin(),
      newTilesToLoad.end());
}

void TilesetHeightFinder::_processHeightRequests() {
  HeightRequests& requests = _heightRequests.front();
  RayInfo& current = requests.current;
  _processTilesLoadingQueue(current);
  while (current.tilesLoading.empty()) {
    requests.heights[requests.numRaysDone++] =
        current.tMin >= 0 ? 100000.0 - current.tMin : -9999.0;
    if (requests.numRaysDone < requests.coordinates.size()) {
      current.coordinate = requests.coordinates[requests.numRaysDone];
      current.ray = createRay(current.coordinate);
      current.tMin = -1.0;
      Tile* pRoot = _pTilesetContentManager->getRootTile();
      _findAndIntersectLeafTiles(pRoot, current, current.tilesLoading);
    } else {
      requests.promise.resolve(std::move(requests.heights));
      _heightRequests.erase(_heightRequests.begin());
      return;
    }
  }
}

Future<std::vector<double>> TilesetHeightFinder::_getHeightsAtCoordinates(
    std::vector<CesiumGeospatial::Cartographic> coordinates) {
  Tile* pRoot = _pTilesetContentManager->getRootTile();
  if (pRoot == nullptr || coordinates.empty()) {
    return _pTileset->getAsyncSystem()
        .createResolvedFuture<std::vector<double>>(
            std::vector<double>(coordinates.size(), -9999.0));
  }
  Promise promise =
      _pTileset->getAsyncSystem().createPromise<std::vector<double>>();
  _heightRequests.emplace_back(HeightRequests{
      RayInfo{createRay(coordinates[0]), coordinates[0], -1.0, {pRoot}},
      0,
      coordinates,
      std::vector<double>(coordinates.size()),
      promise});
  return promise.getFuture();
}
