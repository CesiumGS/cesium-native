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
  if (pTile->getChildren().size() != 0 &&
      pTile->getRefine() != TileRefine::Add) {
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
    if (!_pTilesetContentManager->getRasterOverlayCollection()
             .getOverlays()
             .empty()) {
      _pTilesetContentManager->updateTileContent(*pTile, options);
    }
    return false;
  case TileLoadState::Done:
  case TileLoadState::Failed:
    return false;
  }
  return false;
}

void TilesetHeightFinder::_intersectVisibleTile(
    Tile* pTile,
    RayIntersect& rayInfo) {
  TileRenderContent* pRenderContent = pTile->getContent().getRenderContent();
  if (!pRenderContent)
    return;

  auto intersectResult =
      CesiumGltfContent::GltfUtilities::intersectRayGltfModel(
          rayInfo.ray,
          pRenderContent->getModel(),
          true,
          pTile->getTransform());

  if (!intersectResult.hit.has_value())
    return;

  // Set ray info to this hit if closer, or the first hit
  double prevDistSq = rayInfo.intersectResult.hit->rayToWorldPointDistanceSq;
  double thisDistSq = intersectResult.hit->rayToWorldPointDistanceSq;

  bool setClosest = prevDistSq == -1 || thisDistSq < prevDistSq;
  if (setClosest)
    rayInfo.intersectResult = std::move(intersectResult);
}

void TilesetHeightFinder::_findAndIntersectVisibleTiles(
    Tile* pTile,
    RayIntersect& rayInfo,
    std::vector<Tile*>& newTilesToLoad) {
  if (pTile->getState() == TileLoadState::Failed) {
    return;
  }

  if (pTile->getChildren().empty()) {
    _intersectVisibleTile(pTile, rayInfo);
  } else {
    if (pTile->getRefine() == TileRefine::Add) {
      _intersectVisibleTile(pTile, rayInfo);
    }
    for (Tile& child : pTile->getChildren()) {
      if (child.getContentBoundingVolume()) {
        if (!boundingVolumeContainsCoordinate(
                *child.getContentBoundingVolume(),
                rayInfo.ray,
                rayInfo.coordinate))
          continue;
      } else if (!boundingVolumeContainsCoordinate(
                     child.getBoundingVolume(),
                     rayInfo.ray,
                     rayInfo.coordinate))
        continue;
      if (_loadTileIfNeeded(&child)) {
        newTilesToLoad.push_back(&child);
      } else {
        _findAndIntersectVisibleTiles(&child, rayInfo, newTilesToLoad);
      }
    }
  }
}

void TilesetHeightFinder::_processTilesLoadingQueue(RayIntersect& rayInfo) {
  std::vector<Tile*> newTilesToLoad;
  for (auto it = rayInfo.tilesLoading.begin();
       it != rayInfo.tilesLoading.end();) {
    Tile* pTile = *it;
    if (_loadTileIfNeeded(pTile)) {
      ++it;
    } else {
      it = rayInfo.tilesLoading.erase(it);
      _findAndIntersectVisibleTiles(pTile, rayInfo, newTilesToLoad);
    }
  }
  rayInfo.tilesLoading.insert(
      rayInfo.tilesLoading.end(),
      newTilesToLoad.begin(),
      newTilesToLoad.end());
}

void TilesetHeightFinder::_processHeightRequests() {
  HeightRequests& requests = _heightRequests.front();
  RayIntersect* currentIntersect =
      &requests.rayIntersects[requests.numRaysDone];
  _processTilesLoadingQueue(*currentIntersect);

  while (currentIntersect->tilesLoading.empty()) {
    // Our ray is done loading and should have found a hit or not
    // Go to next ray in this request batch
    requests.numRaysDone++;

    // If there are more rays to process, set up the next one
    if (requests.numRaysDone < requests.rayIntersects.size()) {
      currentIntersect = &requests.rayIntersects[requests.numRaysDone];
      Tile* pRoot = _pTilesetContentManager->getRootTile();
      _findAndIntersectVisibleTiles(
          pRoot,
          *currentIntersect,
          currentIntersect->tilesLoading);
      continue;
    }

    // All rays are done, create results
    std::vector<Tileset::HeightResult> results;
    for (RayIntersect& ray : requests.rayIntersects) {
      bool heightAvailable =
          ray.intersectResult.hit->rayToWorldPointDistanceSq != -1;
      if (heightAvailable) {
        ray.coordinate.height =
            100000.0 -
            glm::sqrt(ray.intersectResult.hit->rayToWorldPointDistanceSq);
      }

      results.push_back(Tileset::HeightResult{heightAvailable, ray.coordinate});
    }
    requests.promise.resolve(std::move(results));
    _heightRequests.erase(_heightRequests.begin());
    return;
  }
}

Future<std::vector<Tileset::HeightResult>>
TilesetHeightFinder::_getHeightsAtCoordinates(
    const std::vector<CesiumGeospatial::Cartographic>& coordinates) {
  Tile* pRoot = _pTilesetContentManager->getRootTile();
  if (pRoot == nullptr || coordinates.empty()) {
    return _pTileset->getAsyncSystem()
        .createResolvedFuture<std::vector<Tileset::HeightResult>>(
            std::vector<Tileset::HeightResult>(
                coordinates.size(),
                {false, {-1, -1, -1}}));
  }
  Promise promise = _pTileset->getAsyncSystem()
                        .createPromise<std::vector<Tileset::HeightResult>>();

  std::vector<RayIntersect> rayIntersects;
  for (const CesiumGeospatial::Cartographic& coordinate : coordinates)
    rayIntersects.push_back(
        RayIntersect{createRay(coordinate), coordinate, {}, {pRoot}});

  _heightRequests.emplace_back(
      HeightRequests{std::move(rayIntersects), 0, promise});

  return promise.getFuture();
}
