#include "CesiumGeospatial/BoundingRegionBuilder.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "TriangulatePolygon.h"

#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/CartographicPolygon.h>
#include <CesiumGeospatial/CompositeCartographicPolygon.h>

#include <glm/ext/vector_double2.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace CesiumGeospatial {
namespace {
CesiumGeospatial::GlobeRectangle
calculateBoundingRectangle(const std::vector<CartographicPolygon>& polygons) {
  CesiumGeospatial::BoundingRegionBuilder builder;
  for (const CartographicPolygon& polygon : polygons) {
    const GlobeRectangle& rect = polygon.getBoundingRectangle();
    builder.expandToIncludePosition(rect.getSouthwest());
    builder.expandToIncludePosition(rect.getNortheast());
  }

  return builder.toGlobeRectangle();
}
} // namespace

CompositeCartographicPolygon::CompositeCartographicPolygon(
    std::vector<CartographicPolygon>&& polygons)
    : _polygons(std::move(polygons)),
      _boundingRectangle(calculateBoundingRectangle(this->_polygons)) {}

bool CompositeCartographicPolygon::contains(const Cartographic& point) const {
  // If there's no polygons, so we're definitely not inside any of them.
  if (this->_polygons.size() < 1) {
    return false;
  }

  // The first polygon specifies the outer bounds, so if we're not in that,
  // we're not in any of them.
  if (!this->_polygons[0].contains(point)) {
    return false;
  }

  // By this point, we're definitely in the outer ring, so the only way we could
  // not contain this point is if we're inside any of the inner rings.
  for (size_t i = 1; i < this->_polygons.size(); i++) {
    if (this->_polygons[i].contains(point)) {
      return false;
    }
  }

  // In the outer ring, but not in any of the inner rings, so we're inside the
  // composite polygon.
  return true;
}

std::vector<uint32_t> CompositeCartographicPolygon::triangulate() const {
  std::vector<std::vector<glm::dvec2>> rings(this->_polygons.size());
  for (const CartographicPolygon& polygon : this->_polygons) {
    rings.emplace_back(polygon.getVertices());
  }

  return triangulatePolygon(rings);
}

bool CompositeCartographicPolygon::operator==(
    const CompositeCartographicPolygon& rhs) const {
  if (this->_polygons.size() != rhs._polygons.size()) {
    return false;
  }

  for (size_t i = 0; i < this->_polygons.size(); i++) {
    if (this->_polygons[i] != rhs._polygons[i]) {
      return false;
    }
  }

  return true;
}

const std::vector<CartographicPolygon>&
CompositeCartographicPolygon::getLinearRings() const {
  return this->_polygons;
}
} // namespace CesiumGeospatial