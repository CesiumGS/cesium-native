#include "TriangulatePolygon.h"

#include <CesiumUtility/Math.h>

#include <glm/common.hpp>
#include <glm/ext/vector_double2.hpp>
#include <mapbox/earcut.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace CesiumGeospatial {
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

    std::vector<Point> localPolyline(vertexCount);
    for (size_t i = 1; i < vertexCount; ++i) {
      localPolyline[i] = Point{ring[i].x, ring[i].y};
    }

    localPolylines.emplace_back(std::move(localPolyline));
  }

  indices = mapbox::earcut<uint32_t>(localPolylines);
  return indices;
}
} // namespace CesiumGeospatial