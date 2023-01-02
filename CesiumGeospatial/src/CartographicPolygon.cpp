#include "CesiumGeospatial/CartographicPolygon.h"

#include <mapbox/earcut.hpp>

#include <array>

namespace CesiumGeospatial {

static std::vector<uint32_t>
triangulatePolygon(const std::vector<glm::dvec2>& polygon) {
  std::vector<uint32_t> indices;
  const size_t vertexCount = polygon.size();

  if (vertexCount < 3) {
    return indices;
  }

  const glm::dvec2& point0 = polygon[0];

  using Point = std::array<double, 2>;

  std::vector<std::vector<Point>> localPolygons;

  // normalize the longitude relative to the first point before triangulating
  std::vector<Point> localPolygon(vertexCount);
  localPolygon[0] = {0.0, point0.y};
  for (size_t i = 1; i < vertexCount; ++i) {
    const glm::dvec2& cartographic = polygon[i];
    Point& point = localPolygon[i];
    point[0] = cartographic.x - point0.x;
    point[1] = cartographic.y;

    // check if the difference crosses the antipole
    if (glm::abs(point[0]) > CesiumUtility::Math::OnePi) {
      if (point[0] > 0.0) {
        point[0] -= CesiumUtility::Math::TwoPi;
      } else {
        point[0] += CesiumUtility::Math::TwoPi;
      }
    }
  }

  localPolygons.emplace_back(std::move(localPolygon));

  indices = mapbox::earcut<uint32_t>(localPolygons);
  return indices;
}

static std::optional<GlobeRectangle>
computeBoundingRectangle(const std::vector<glm::dvec2>& polygon) {
  const size_t vertexCount = polygon.size();

  if (vertexCount < 3) {
    return std::nullopt;
  }

  const glm::dvec2& point0 = polygon[0];

  // the bounding globe rectangle
  double west = point0.x;
  double east = point0.x;
  double south = point0.y;
  double north = point0.y;

  for (size_t i = 1; i < vertexCount; ++i) {
    const glm::dvec2& point1 = polygon[i];

    if (point1.y > north) {
      north = point1.y;
    } else if (point1.y < south) {
      south = point1.y;
    }

    const double dif_west = point1.x - west;
    // check if the difference crosses the antipole
    if (glm::abs(dif_west) > CesiumUtility::Math::OnePi) {
      // east wrapping past the antipole to the west
      if (dif_west > 0.0) {
        west = point1.x;
      }
    } else {
      if (dif_west < 0.0) {
        west = point1.x;
      }
    }

    const double dif_east = point1.x - east;
    // check if the difference crosses the antipole
    if (glm::abs(dif_east) > CesiumUtility::Math::OnePi) {
      // west wrapping past the antipole to the east
      if (dif_east < 0.0) {
        east = point1.x;
      }
    } else {
      if (dif_east > 0.0) {
        east = point1.x;
      }
    }
  }

  return CesiumGeospatial::GlobeRectangle(west, south, east, north);
}

CartographicPolygon::CartographicPolygon(const std::vector<glm::dvec2>& polygon)
    : _vertices(polygon),
      _indices(triangulatePolygon(polygon)),
      _boundingRectangle(computeBoundingRectangle(polygon)) {}

} // namespace CesiumGeospatial
