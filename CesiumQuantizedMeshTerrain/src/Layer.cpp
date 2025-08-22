#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGeospatial/WebMercatorProjection.h>
#include <CesiumQuantizedMeshTerrain/Layer.h>

#include <optional>
#include <string>
#include <variant>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumQuantizedMeshTerrain;

namespace {
const std::string geographicString("EPSG:4326");
const std::string webMercatorString("EPSG:3857");
} // namespace

std::optional<Projection> Layer::getProjection(
    const CesiumGeospatial::Ellipsoid& ellipsoid) const noexcept {
  if (this->projection == geographicString)
    return GeographicProjection(ellipsoid);
  else if (this->projection == webMercatorString)
    return WebMercatorProjection(ellipsoid);
  else
    return std::nullopt;
}

std::optional<CesiumGeometry::QuadtreeTilingScheme> Layer::getTilingScheme(
    const CesiumGeospatial::Ellipsoid& ellipsoid) const noexcept {
  std::optional<Projection> maybeProjection = this->getProjection(ellipsoid);
  if (!maybeProjection)
    return std::nullopt;

  struct Operation {
    QuadtreeTilingScheme operator()(const GeographicProjection& geographic) {
      return QuadtreeTilingScheme(
          geographic.project(GeographicProjection::MAXIMUM_GLOBE_RECTANGLE),
          2,
          1);
    }

    QuadtreeTilingScheme operator()(const WebMercatorProjection& webMercator) {
      return QuadtreeTilingScheme(
          webMercator.project(WebMercatorProjection::MAXIMUM_GLOBE_RECTANGLE),
          1,
          1);
    }
  };

  return std::visit(Operation(), *maybeProjection);
}

std::optional<CesiumGeospatial::BoundingRegion> Layer::getRootBoundingRegion(
    const CesiumGeospatial::Ellipsoid& ellipsoid) const noexcept {
  std::optional<Projection> maybeProjection = this->getProjection(ellipsoid);
  if (!maybeProjection)
    return std::nullopt;

  struct Operation {
    GlobeRectangle operator()(const GeographicProjection&) {
      return GeographicProjection::MAXIMUM_GLOBE_RECTANGLE;
    }

    GlobeRectangle operator()(const WebMercatorProjection&) {
      return WebMercatorProjection::MAXIMUM_GLOBE_RECTANGLE;
    }
  };

  GlobeRectangle rectangle = std::visit(Operation(), *maybeProjection);

  // These heights encompass all Earth terrain, but not all Earth bathymetry.
  const double defaultMinimumHeight = -1000.0;
  const double defaultMaximumHeight = 9000.0;

  return BoundingRegion(
      rectangle,
      defaultMinimumHeight,
      defaultMaximumHeight,
      getProjectionEllipsoid(*maybeProjection));
}
