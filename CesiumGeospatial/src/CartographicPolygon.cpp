#include <CesiumGeometry/IntersectionTests.h>
#include <CesiumGeospatial/BoundingRegionBuilder.h>
#include <CesiumGeospatial/Cartographic.h>
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
std::vector<glm::dvec2>
cartographicToVec(const std::vector<Cartographic>& polygon) {
  std::vector<glm::dvec2> vertices;
  vertices.reserve(polygon.size());
  for (const Cartographic& point : polygon) {
    vertices.emplace_back(point.longitude, point.latitude);
  }
  return vertices;
}

GlobeRectangle
computeBoundingRectangle(const std::vector<glm::dvec2>& polygon) {
  BoundingRegionBuilder builder;
  for (const glm::dvec2& point : polygon) {
    builder.expandToIncludePosition(Cartographic(point.x, point.y, 0));
  }
  return builder.toGlobeRectangle();
}

std::vector<uint32_t>
triangulatePolygon(const std::vector<std::vector<glm::dvec2>>& rings) {
  std::vector<uint32_t> indices;
  using Point = std::array<double, 2>;
  std::vector<std::vector<Point>> localPolylines;
  for (const std::vector<glm::dvec2>& ring : rings) {
    const size_t vertexCount = ring.size();

    if (vertexCount < 3) {
      return indices;
    }

    const glm::dvec2& point0 = ring[0];

    // normalize the longitude relative to the first point before triangulating
    std::vector<Point> localPolyline(vertexCount);
    localPolyline[0] = {0.0, point0.y};
    for (size_t i = 1; i < vertexCount; ++i) {
      const glm::dvec2& cartographic = ring[i];
      Point& point = localPolyline[i];
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

    localPolylines.emplace_back(std::move(localPolyline));
  }

  indices = mapbox::earcut<uint32_t>(localPolylines);
  return indices;
}
} // namespace

CartographicPolygon::CartographicPolygon(const std::vector<glm::dvec2>& polygon)
    : _vertices(polygon),
      _indices(triangulatePolygon({polygon})),
      _holes(),
      _boundingRectangle(computeBoundingRectangle(polygon)) {}

CartographicPolygon::CartographicPolygon(
    const std::vector<Cartographic>& polygon)
    : _vertices(cartographicToVec(polygon)),
      _indices(triangulatePolygon({this->_vertices})),
      _holes(),
      _boundingRectangle(computeBoundingRectangle(this->_vertices)) {}

CartographicPolygon::CartographicPolygon(
    const std::vector<Cartographic>& vertices,
    std::vector<CartographicPolygon>&& holes)
    : _vertices(cartographicToVec(vertices)),
      _indices(),
      _holes(std::move(holes)),
      _boundingRectangle(computeBoundingRectangle(this->_vertices)) {
  std::vector<std::vector<glm::dvec2>> rings{this->_vertices};
  for (const CartographicPolygon& hole : this->_holes) {
    rings.emplace_back(hole.getVertices());
  }
  this->_indices = triangulatePolygon(rings);
}

bool CartographicPolygon::contains(const Cartographic& point) const {
  // Simple bounding rectangle check to catch points without a chance of
  // intersecting.
  if (!this->_boundingRectangle.contains(point)) {
    return false;
  }

  // We don't even have a single triangle, no chance that it's within our
  // non-polygon.
  if (this->_indices.size() < 3) {
    return false;
  }

  const glm::dvec2 pointVec(point.longitude, point.latitude);

  // Check all of our triangles to see if it contains our point.
  for (size_t j = 2; j < this->_indices.size(); j += 3) {
    if (IntersectionTests::pointInTriangle(
            pointVec,
            this->_vertices[this->_indices[j - 2]],
            this->_vertices[this->_indices[j - 1]],
            this->_vertices[this->_indices[j]])) {
      return true;
    }
  }

  // Not in any of our triangles.
  return false;
}

bool CartographicPolygon::operator==(const CartographicPolygon& rhs) const {
  // The other two fields are derived from the vertices, so if the vertices are
  // equal the polygons are equal.
  if (this->_vertices.size() != rhs._vertices.size()) {
    return false;
  }

  for (size_t i = 0; i < this->_vertices.size(); i++) {
    if (this->_vertices[i] != rhs._vertices[i]) {
      return false;
    }
  }

  return true;
}

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
