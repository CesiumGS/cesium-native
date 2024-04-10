#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumQuantizedMeshTerrain/Layer.h>
#include <CesiumQuantizedMeshTerrain/LayerJsonUtilities.h>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumQuantizedMeshTerrain;

namespace {
const std::string geographicString("EPSG:4326");
const std::string webMercatorString("EPSG:3857");
} // namespace

std::optional<Projection>
LayerJsonUtilities::getProjection(const Layer& layer) {
  if (layer.projection == geographicString)
    return GeographicProjection();
  else if (layer.projection == webMercatorString)
    return WebMercatorProjection();
  else
    return std::nullopt;
}

std::optional<CesiumGeometry::QuadtreeTilingScheme>
LayerJsonUtilities::getTilingScheme(const Layer& layer) {
  std::optional<Projection> maybeProjection = getProjection(layer);
  if (!maybeProjection)
    return std::nullopt;

  struct Operation {
    QuadtreeTilingScheme operator()(const GeographicProjection& projection) {
      return QuadtreeTilingScheme(
          projection.project(GeographicProjection::MAXIMUM_GLOBE_RECTANGLE),
          2,
          1);
    }

    QuadtreeTilingScheme operator()(const WebMercatorProjection& projection) {
      return QuadtreeTilingScheme(
          projection.project(WebMercatorProjection::MAXIMUM_GLOBE_RECTANGLE),
          1,
          1);
    }
  };

  return std::visit(Operation(), *maybeProjection);
}

std::optional<CesiumGeospatial::BoundingRegion>
LayerJsonUtilities::getRootBoundingRegion(const Layer& layer) {
  std::optional<Projection> maybeProjection = getProjection(layer);
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

  return BoundingRegion(rectangle, defaultMinimumHeight, defaultMaximumHeight);
}
