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

// 10,000 meters above ellisoid
// Highest point on ellipsoid is Mount Everest at 8,848 m
// Nothing intersectable should be above this
#define RAY_ORIGIN_HEIGHT 10000.0

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
  cartographic.height = RAY_ORIGIN_HEIGHT;
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
  if (!rayInfo.intersectResult.hit.has_value()) {
    rayInfo.intersectResult = std::move(intersectResult);
  } else {
    double prevDistSq = rayInfo.intersectResult.hit->rayToWorldPointDistanceSq;
    double thisDistSq = intersectResult.hit->rayToWorldPointDistanceSq;
    if (thisDistSq < prevDistSq)
      rayInfo.intersectResult = std::move(intersectResult);
  }
}

void TilesetHeightFinder::_findCandidateTiles(
    Tile* pTile,
    RayIntersect& rayInfo,
    std::vector<Tile*>& candidateTiles) {

  // If tile failed to load, this means we can't complete the intersection
  // TODO, need to return warning, or abort the intersect
  if (pTile->getState() == TileLoadState::Failed)
    return;

  // If tile not done loading, add it to the list
  if (pTile->getState() != TileLoadState::Done) {
    candidateTiles.push_back(pTile);
    return;
  }

  if (pTile->getChildren().empty()) {
    // This is a leaf node, add it to the list
    candidateTiles.push_back(pTile);
  } else {
    // We have children

    // If additive refinement, add parent to the list with children
    if (pTile->getRefine() == TileRefine::Add)
      candidateTiles.push_back(pTile);

    // Traverse children
    for (Tile& child : pTile->getChildren()) {
      auto& contentBoundingVolume = child.getContentBoundingVolume();

      // If content bounding volume exists and no intersection, we can skip it
      if (contentBoundingVolume && !boundingVolumeContainsCoordinate(
                                       *contentBoundingVolume,
                                       rayInfo.ray,
                                       rayInfo.inputCoordinate))
        continue;

      // if bounding volume doesn't intersect this ray, we can skip it
      if (!boundingVolumeContainsCoordinate(
              child.getBoundingVolume(),
              rayInfo.ray,
              rayInfo.inputCoordinate))
        continue;

      // Child is a candidate, traverse it and its children
      _findCandidateTiles(&child, rayInfo, candidateTiles);
    }
  }
}

void TilesetHeightFinder::_processHeightRequests(
    std::set<Tile*>& tilesNeedingLoading) {
  HeightRequests& requests = _heightRequests.front();
  Tile* pRoot = _pTilesetContentManager->getRootTile();

  for (RayIntersect& intersect : requests.rayIntersects) {
    intersect.candidateTiles.clear();

    _findCandidateTiles(pRoot, intersect, intersect.candidateTiles);

    // If any candidates need loading, add to return set
    for (Tile* pTile : intersect.candidateTiles) {
      if (pTile->getState() != TileLoadState::Done)
        tilesNeedingLoading.insert(pTile);
    }
  }

  // Bail if we're waiting on tiles to load
  if (!tilesNeedingLoading.empty())
    return;

  // Do the intersect tests
  for (RayIntersect& intersect : requests.rayIntersects) {
    for (Tile* pTile : intersect.candidateTiles)
      _intersectVisibleTile(pTile, intersect);
  }

  // All rays are done, create results
  Tileset::HeightResults results;
  for (RayIntersect& ray : requests.rayIntersects) {
    Tileset::HeightResults::CoordinateResult coordinateResult = {
        ray.intersectResult.hit.has_value(),
        std::move(ray.inputCoordinate),
        std::move(ray.intersectResult.warnings)};

    if (coordinateResult.heightAvailable)
      coordinateResult.coordinate.height =
          RAY_ORIGIN_HEIGHT -
          glm::sqrt(ray.intersectResult.hit->rayToWorldPointDistanceSq);

    results.coordinateResults.push_back(coordinateResult);
  }

  requests.promise.resolve(std::move(results));
  _heightRequests.erase(_heightRequests.begin());
}

Future<Tileset::HeightResults> TilesetHeightFinder::_getHeightsAtCoordinates(
    const std::vector<CesiumGeospatial::Cartographic>& coordinates) {
  Tile* pRoot = _pTilesetContentManager->getRootTile();
  if (pRoot == nullptr || coordinates.empty()) {
    return _pTileset->getAsyncSystem()
        .createResolvedFuture<Tileset::HeightResults>({});
  }
  Promise promise =
      _pTileset->getAsyncSystem().createPromise<Tileset::HeightResults>();

  std::vector<RayIntersect> rayIntersects;
  for (const CesiumGeospatial::Cartographic& coordinate : coordinates)
    rayIntersects.push_back(
        RayIntersect{coordinate, createRay(coordinate), {}});

  _heightRequests.emplace_back(
      HeightRequests{std::move(rayIntersects), promise});

  return promise.getFuture();
}
