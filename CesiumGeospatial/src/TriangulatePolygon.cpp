#include "TriangulatePolygon.h"

#include <CesiumUtility/Math.h>

#include <glm/vec2.hpp>
#include <mapbox/earcut.hpp>

#include <array>
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
} // namespace CesiumGeospatial