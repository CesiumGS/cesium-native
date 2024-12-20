#include "TileUtilities.h"

#include <Cesium3DTilesSelection/BoundingVolume.h>
#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>

#include <optional>
#include <vector>

using namespace CesiumGeospatial;

namespace Cesium3DTilesSelection {
namespace CesiumImpl {

bool withinPolygons(
    const BoundingVolume& boundingVolume,
    const std::vector<CartographicPolygon>& cartographicPolygons,
    const Ellipsoid& ellipsoid) noexcept {

  std::optional<GlobeRectangle> maybeRectangle =
      estimateGlobeRectangle(boundingVolume, ellipsoid);
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
        cartographicPolygons,
    const Ellipsoid& ellipsoid) noexcept {

  std::optional<GlobeRectangle> maybeRectangle =
      estimateGlobeRectangle(boundingVolume, ellipsoid);
  if (!maybeRectangle) {
    return false;
  }

  return CartographicPolygon::rectangleIsOutsidePolygons(
      *maybeRectangle,
      cartographicPolygons);
}

} // namespace CesiumImpl

} // namespace Cesium3DTilesSelection
