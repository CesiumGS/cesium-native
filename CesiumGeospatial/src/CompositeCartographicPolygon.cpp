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

glm::dvec2
fetchIndex(const std::vector<CartographicPolygon>& polygons, uint32_t index) {
  for (size_t i = 0; i < polygons.size(); i++) {
    if (index >= polygons[i].getVertices().size()) {
      index -= polygons[i].getVertices().size();
      continue;
    }

    return polygons[i].getVertices()[index];
  }

  return glm::dvec2(0, 0);
}
} // namespace

CompositeCartographicPolygon::CompositeCartographicPolygon(
    std::vector<CartographicPolygon>&& polygons)
    : _polygons(std::move(polygons)),
      _boundingRectangle(calculateBoundingRectangle(this->_polygons)) {
  const std::vector<uint32_t> indices = this->triangulate();
  this->_unindexedVertices.reserve(indices.size());
  for (uint32_t index : indices) {
    this->_unindexedVertices.emplace_back(fetchIndex(this->_polygons, index));
  }
}

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
  std::vector<std::vector<glm::dvec2>> rings;
  rings.reserve(this->_polygons.size());
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

const std::vector<glm::dvec2>&
CompositeCartographicPolygon::getUnindexedVertices() const {
  return this->_unindexedVertices;
}

const std::vector<CartographicPolygon>&
CompositeCartographicPolygon::getLinearRings() const {
  return this->_polygons;
}
} // namespace CesiumGeospatial