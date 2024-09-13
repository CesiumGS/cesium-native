#include "TilesetHeightQuery.h"

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

void TilesetHeightQuery::intersectVisibleTile(Tile* pTile) {
  TileRenderContent* pRenderContent = pTile->getContent().getRenderContent();
  if (!pRenderContent)
    return;

  auto gltfIntersectResult =
      CesiumGltfContent::GltfUtilities::intersectRayGltfModel(
          this->ray,
          pRenderContent->getModel(),
          true,
          pTile->getTransform());

  if (!gltfIntersectResult.warnings.empty()) {
    this->intersectResult.warnings.insert(
        this->intersectResult.warnings.end(),
        gltfIntersectResult.warnings.begin(),
        gltfIntersectResult.warnings.end());
  }

  // Set ray info to this hit if closer, or the first hit
  if (!this->intersectResult.hit.has_value()) {
    this->intersectResult.hit = std::move(gltfIntersectResult.hit);
  } else {
    double prevDistSq = this->intersectResult.hit->rayToWorldPointDistanceSq;
    double thisDistSq = intersectResult.hit->rayToWorldPointDistanceSq;
    if (thisDistSq < prevDistSq)
      this->intersectResult.hit = std::move(gltfIntersectResult.hit);
  }
}

void TilesetHeightQuery::findCandidateTiles(
    Tile* pTile,
    std::vector<std::string>& warnings) {

  // If tile failed to load, this means we can't complete the intersection
  if (pTile->getState() == TileLoadState::Failed) {
    warnings.push_back("Tile load failed during query. Ignoring.");
    return;
  }

  const std::optional<BoundingVolume>& contentBoundingVolume =
      pTile->getContentBoundingVolume();

  if (pTile->getChildren().empty()) {
    // This is a leaf node, it's a candidate

    // If optional content bounding volume exists, test against it
    if (contentBoundingVolume) {
      if (boundingVolumeContainsCoordinate(
              *contentBoundingVolume,
              this->ray,
              this->inputCoordinate))
        this->candidateTiles.push_back(pTile);
    } else {
      this->candidateTiles.push_back(pTile);
    }
  } else {
    // We have children

    // If additive refinement, add parent to the list with children
    if (pTile->getRefine() == TileRefine::Add) {
      // If optional content bounding volume exists, test against it
      if (contentBoundingVolume) {
        if (boundingVolumeContainsCoordinate(
                *contentBoundingVolume,
                this->ray,
                this->inputCoordinate))
          this->additiveCandidateTiles.push_back(pTile);
      } else {
        this->additiveCandidateTiles.push_back(pTile);
      }
    }

    // Traverse children
    for (Tile& child : pTile->getChildren()) {
      // if bounding volume doesn't intersect this ray, we can skip it
      if (!boundingVolumeContainsCoordinate(
              child.getBoundingVolume(),
              this->ray,
              this->inputCoordinate))
        continue;

      // Child is a candidate, traverse it and its children
      findCandidateTiles(&child, warnings);
    }
  }
}
