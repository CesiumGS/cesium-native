#include "TerrainQuery.h"

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
      std::optional<double> t =
          IntersectionTests::rayOBBParametric(ray, boundingBox);
      return t && t.value() >= 0;
    }

    bool operator()(const BoundingRegion& boundingRegion) noexcept {
      return boundingRegion.getRectangle().contains(coordinate);
    }

    bool operator()(const BoundingSphere& boundingSphere) noexcept {
      std::optional<double> t =
          IntersectionTests::raySphereParametric(ray, boundingSphere);
      return t && t.value() >= 0;
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

} // namespace

void TerrainQuery::intersectVisibleTile(Tile* pTile) {
  TileRenderContent* pRenderContent = pTile->getContent().getRenderContent();
  if (!pRenderContent)
    return;

  auto gltfIntersectResult =
      CesiumGltfContent::GltfUtilities::intersectRayGltfModel(
          this->ray,
          pRenderContent->getModel(),
          true,
          pTile->getTransform());

  if (!gltfIntersectResult.hit.has_value())
    return;

  // Set ray info to this hit if closer, or the first hit
  if (!this->intersectResult.hit.has_value()) {
    this->intersectResult = std::move(gltfIntersectResult);
  } else {
    double prevDistSq = this->intersectResult.hit->rayToWorldPointDistanceSq;
    double thisDistSq = intersectResult.hit->rayToWorldPointDistanceSq;
    if (thisDistSq < prevDistSq)
      this->intersectResult = std::move(gltfIntersectResult);
  }
}

void TerrainQuery::findCandidateTiles(Tile* pTile) {

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
                                       this->ray,
                                       this->inputCoordinate))
        continue;

      // if bounding volume doesn't intersect this ray, we can skip it
      if (!boundingVolumeContainsCoordinate(
              child.getBoundingVolume(),
              this->ray,
              this->inputCoordinate))
        continue;

      // Child is a candidate, traverse it and its children
      findCandidateTiles(&child);
    }
  }
}
