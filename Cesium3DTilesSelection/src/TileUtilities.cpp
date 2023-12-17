#include "TileUtilities.h"

#include <CesiumGeometry/IntersectionTests.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/BoundingRegionWithLooseFittingHeights.h>
#include <CesiumGeospatial/GlobeRectangle.h>

#include <variant>

using namespace CesiumGeospatial;
using namespace CesiumGeometry;

namespace Cesium3DTilesSelection {
namespace CesiumImpl {

bool withinPolygons(
    const BoundingVolume& boundingVolume,
    const std::vector<CartographicPolygon>& cartographicPolygons) noexcept {

  std::optional<GlobeRectangle> maybeRectangle =
      estimateGlobeRectangle(boundingVolume);
  if (!maybeRectangle) {
    return false;
  }

  return CartographicPolygon::rectangleIsWithinPolygons(
      *maybeRectangle,
      cartographicPolygons);
}

bool outsidePolygons(
    const BoundingVolume& boundingVolume,
    const std::vector<CesiumGeospatial::CartographicPolygon>&
        cartographicPolygons) noexcept {

  std::optional<GlobeRectangle> maybeRectangle =
      estimateGlobeRectangle(boundingVolume);
  if (!maybeRectangle) {
    return false;
  }

  return CartographicPolygon::rectangleIsOutsidePolygons(
      *maybeRectangle,
      cartographicPolygons);
}

bool rayIntersectsBoundingVolume(
    const BoundingVolume& boundingVolume,
    const Ray& ray) {
  struct Operation {
    const Ray& ray;

    bool operator()(const OrientedBoundingBox& boundingBox) noexcept {
      return IntersectionTests::rayOBBParametric(ray, boundingBox) >= 0;
    }

    bool operator()(const BoundingRegion& boundingRegion) noexcept {
      return IntersectionTests::rayOBBParametric(
                 ray,
                 boundingRegion.getBoundingBox()) >= 0;
    }

    bool operator()(const BoundingSphere& boundingSphere) noexcept {
      return IntersectionTests::raySphereParametric(ray, boundingSphere) >= 0;
    }

    bool operator()(
        const BoundingRegionWithLooseFittingHeights& boundingRegion) noexcept {
      return IntersectionTests::rayOBBParametric(
                 ray,
                 boundingRegion.getBoundingRegion().getBoundingBox()) >= 0;
    }

    bool operator()(const S2CellBoundingVolume& s2Cell) noexcept {
      return IntersectionTests::rayOBBParametric(
                 ray,
                 s2Cell.computeBoundingRegion().getBoundingBox()) >= 0;
    }
  };

  return std::visit(Operation{ray}, boundingVolume);
}

} // namespace CesiumImpl

} // namespace Cesium3DTilesSelection
