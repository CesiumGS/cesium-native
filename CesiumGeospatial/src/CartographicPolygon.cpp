#include <CesiumGeometry/IntersectionTests.h>
#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumUtility/Math.h>

#include <glm/common.hpp>
#include <glm/ext/matrix_double2x2.hpp>
#include <glm/ext/vector_double2.hpp>
#include <glm/matrix.hpp>
#include <mapbox/earcut.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

using namespace CesiumGeometry;

namespace CesiumGeospatial {

namespace {
std::vector<uint32_t>
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

std::optional<GlobeRectangle>
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
} // namespace

CartographicPolygon::CartographicPolygon(const std::vector<glm::dvec2>& polygon)
    : _vertices(polygon),
      _indices(triangulatePolygon(polygon)),
      _boundingRectangle(computeBoundingRectangle(polygon)) {}

/*static*/ bool CartographicPolygon::rectangleIsWithinPolygons(
    const CesiumGeospatial::GlobeRectangle& rectangle,
    const std::vector<CartographicPolygon>& cartographicPolygons) noexcept {

  glm::dvec2 rectangleCorners[] = {
      glm::dvec2(rectangle.getWest(), rectangle.getSouth()),
      glm::dvec2(rectangle.getWest(), rectangle.getNorth()),
      glm::dvec2(rectangle.getEast(), rectangle.getNorth()),
      glm::dvec2(rectangle.getEast(), rectangle.getSouth())};

  glm::dvec2 rectangleEdges[] = {
      rectangleCorners[1] - rectangleCorners[0],
      rectangleCorners[2] - rectangleCorners[1],
      rectangleCorners[3] - rectangleCorners[2],
      rectangleCorners[0] - rectangleCorners[3]};

  // Iterate through all polygons.
  for (const auto& selection : cartographicPolygons) {
    const std::optional<CesiumGeospatial::GlobeRectangle>&
        polygonBoundingRectangle = selection.getBoundingRectangle();
    if (!polygonBoundingRectangle ||
        !rectangle.computeIntersection(*polygonBoundingRectangle)) {
      continue;
    }

    const std::vector<glm::dvec2>& vertices = selection.getVertices();
    const std::vector<uint32_t>& indices = selection.getIndices();

    // First check if an arbitrary point on the bounding globe rectangle is
    // inside the polygon.
    bool inside = false;
    for (size_t j = 2; j < indices.size(); j += 3) {
      if (IntersectionTests::pointInTriangle(
              rectangleCorners[0],
              vertices[indices[j - 2]],
              vertices[indices[j - 1]],
              vertices[indices[j]])) {
        inside = true;
        break;
      }
    }

    // If the arbitrary point was outside, then this polygon does not entirely
    // cull the tile.
    if (!inside) {
      continue;
    }

    // Check if the polygon perimeter intersects the bounding globe rectangle
    // edges.
    bool intersectionFound = false;
    for (size_t j = 0; j < vertices.size(); ++j) {
      const glm::dvec2& a = vertices[j];
      const glm::dvec2& b = vertices[(j + 1) % vertices.size()];

      const glm::dvec2 ba = a - b;

      // Check each rectangle edge.
      for (size_t k = 0; k < 4; ++k) {
        const glm::dvec2& cd = rectangleEdges[k];
        const glm::dmat2 lineSegmentMatrix(cd, ba);
        const glm::dvec2 ca = a - rectangleCorners[k];

        // s and t are calculated such that:
        // line_intersection = a + t * ab = c + s * cd
        const glm::dvec2 st = glm::inverse(lineSegmentMatrix) * ca;

        // check that the intersection is within the line segments
        if (st.x <= 1.0 && st.x >= 0.0 && st.y <= 1.0 && st.y >= 0.0) {
          intersectionFound = true;
          break;
        }
      }

      if (intersectionFound) {
        break;
      }
    }

    // There is no intersection with the perimeter and at least one point is
    // inside the polygon so the tile is completely inside this polygon.
    if (!intersectionFound) {
      return true;
    }
  }

  return false;
}

/*static*/ bool CartographicPolygon::rectangleIsOutsidePolygons(
    const CesiumGeospatial::GlobeRectangle& rectangle,
    const std::vector<CesiumGeospatial::CartographicPolygon>&
        cartographicPolygons) noexcept {

  glm::dvec2 rectangleCorners[] = {
      glm::dvec2(rectangle.getWest(), rectangle.getSouth()),
      glm::dvec2(rectangle.getWest(), rectangle.getNorth()),
      glm::dvec2(rectangle.getEast(), rectangle.getNorth()),
      glm::dvec2(rectangle.getEast(), rectangle.getSouth())};

  glm::dvec2 rectangleEdges[] = {
      rectangleCorners[1] - rectangleCorners[0],
      rectangleCorners[2] - rectangleCorners[1],
      rectangleCorners[3] - rectangleCorners[2],
      rectangleCorners[0] - rectangleCorners[3]};

  // Iterate through all polygons.
  for (const auto& selection : cartographicPolygons) {
    const std::optional<CesiumGeospatial::GlobeRectangle>&
        polygonBoundingRectangle = selection.getBoundingRectangle();
    if (!polygonBoundingRectangle ||
        !rectangle.computeIntersection(*polygonBoundingRectangle)) {
      continue;
    }

    const std::vector<glm::dvec2>& vertices = selection.getVertices();
    const std::vector<uint32_t>& indices = selection.getIndices();

    // Check if an arbitrary point on the polygon is in the globe rectangle.
    if (IntersectionTests::pointInTriangle(
            vertices[0],
            rectangleCorners[0],
            rectangleCorners[1],
            rectangleCorners[2]) ||
        IntersectionTests::pointInTriangle(
            vertices[0],
            rectangleCorners[0],
            rectangleCorners[2],
            rectangleCorners[3])) {
      return false;
    }

    // Check if an arbitrary point on the bounding globe rectangle is
    // inside the polygon.
    for (size_t j = 2; j < indices.size(); j += 3) {
      if (IntersectionTests::pointInTriangle(
              rectangleCorners[0],
              vertices[indices[j - 2]],
              vertices[indices[j - 1]],
              vertices[indices[j]])) {
        return false;
      }
    }

    // Now we know the rectangle does not fully contain the polygon and the
    // polygon does not fully contain the rectangle. Now check if the polygon
    // perimeter intersects the bounding globe rectangle edges.
    for (size_t j = 0; j < vertices.size(); ++j) {
      const glm::dvec2& a = vertices[j];
      const glm::dvec2& b = vertices[(j + 1) % vertices.size()];

      const glm::dvec2 ba = a - b;

      // Check each rectangle edge.
      for (size_t k = 0; k < 4; ++k) {
        const glm::dvec2& cd = rectangleEdges[k];
        const glm::dmat2 lineSegmentMatrix(cd, ba);
        const glm::dvec2 ca = a - rectangleCorners[k];

        // s and t are calculated such that:
        // line_intersection = a + t * ab = c + s * cd
        const glm::dvec2 st = glm::inverse(lineSegmentMatrix) * ca;

        // check that the intersection is within the line segments
        if (st.x <= 1.0 && st.x >= 0.0 && st.y <= 1.0 && st.y >= 0.0) {
          return false;
        }
      }
    }
  }

  return true;
}

} // namespace CesiumGeospatial
