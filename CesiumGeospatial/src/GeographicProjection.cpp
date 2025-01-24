#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumGeospatial/GlobeRectangle.h>

#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_double3.hpp>

namespace CesiumGeospatial {

GeographicProjection::GeographicProjection(const Ellipsoid& ellipsoid) noexcept
    : _ellipsoid(ellipsoid),
      _semimajorAxis(ellipsoid.getMaximumRadius()),
      _oneOverSemimajorAxis(1.0 / ellipsoid.getMaximumRadius()) {}

glm::dvec3
GeographicProjection::project(const Cartographic& cartographic) const noexcept {
  const double semimajorAxis = this->_semimajorAxis;
  return glm::dvec3(
      cartographic.longitude * semimajorAxis,
      cartographic.latitude * semimajorAxis,
      cartographic.height);
}

CesiumGeometry::Rectangle GeographicProjection::project(
    const CesiumGeospatial::GlobeRectangle& rectangle) const noexcept {
  const glm::dvec3 sw = this->project(rectangle.getSouthwest());
  const glm::dvec3 ne = this->project(rectangle.getNortheast());
  return CesiumGeometry::Rectangle(sw.x, sw.y, ne.x, ne.y);
}

Cartographic GeographicProjection::unproject(
    const glm::dvec2& projectedCoordinates) const noexcept {
  const double oneOverEarthSemimajorAxis = this->_oneOverSemimajorAxis;

  return Cartographic(
      projectedCoordinates.x * oneOverEarthSemimajorAxis,
      projectedCoordinates.y * oneOverEarthSemimajorAxis,
      0.0);
}

Cartographic GeographicProjection::unproject(
    const glm::dvec3& projectedCoordinates) const noexcept {
  Cartographic result = this->unproject(glm::dvec2(projectedCoordinates));
  result.height = projectedCoordinates.z;
  return result;
}

CesiumGeospatial::GlobeRectangle GeographicProjection::unproject(
    const CesiumGeometry::Rectangle& rectangle) const noexcept {
  const Cartographic sw = this->unproject(rectangle.getLowerLeft());
  const Cartographic ne = this->unproject(rectangle.getUpperRight());
  return GlobeRectangle(sw.longitude, sw.latitude, ne.longitude, ne.latitude);
}

} // namespace CesiumGeospatial
