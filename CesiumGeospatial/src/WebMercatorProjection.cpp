#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/WebMercatorProjection.h>
#include <CesiumUtility/Math.h>

#include <glm/exponential.hpp>
#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/trigonometric.hpp>

namespace CesiumGeospatial {

/*static*/ const double WebMercatorProjection::MAXIMUM_LATITUDE =
    WebMercatorProjection::mercatorAngleToGeodeticLatitude(
        CesiumUtility::Math::OnePi);

/*static*/ const GlobeRectangle WebMercatorProjection::MAXIMUM_GLOBE_RECTANGLE =
    GlobeRectangle(
        -CesiumUtility::Math::OnePi,
        -MAXIMUM_LATITUDE,
        CesiumUtility::Math::OnePi,
        MAXIMUM_LATITUDE);

WebMercatorProjection::WebMercatorProjection(
    const Ellipsoid& ellipsoid) noexcept
    : _ellipsoid(ellipsoid),
      _semimajorAxis(ellipsoid.getMaximumRadius()),
      _oneOverSemimajorAxis(1.0 / ellipsoid.getMaximumRadius()) {}

glm::dvec3 WebMercatorProjection::project(
    const Cartographic& cartographic) const noexcept {
  const double semimajorAxis = this->_semimajorAxis;
  return glm::dvec3(
      cartographic.longitude * semimajorAxis,
      WebMercatorProjection::geodeticLatitudeToMercatorAngle(
          cartographic.latitude) *
          semimajorAxis,
      cartographic.height);
}

CesiumGeometry::Rectangle WebMercatorProjection::project(
    const CesiumGeospatial::GlobeRectangle& rectangle) const noexcept {
  const glm::dvec3 sw = this->project(rectangle.getSouthwest());
  const glm::dvec3 ne = this->project(rectangle.getNortheast());
  return CesiumGeometry::Rectangle(sw.x, sw.y, ne.x, ne.y);
}

Cartographic WebMercatorProjection::unproject(
    const glm::dvec2& projectedCoordinates) const noexcept {
  const double oneOverEarthSemimajorAxis = this->_oneOverSemimajorAxis;

  return Cartographic(
      projectedCoordinates.x * oneOverEarthSemimajorAxis,
      WebMercatorProjection::mercatorAngleToGeodeticLatitude(
          projectedCoordinates.y * oneOverEarthSemimajorAxis),
      0.0);
}

Cartographic WebMercatorProjection::unproject(
    const glm::dvec3& projectedCoordinates) const noexcept {
  Cartographic result = this->unproject(glm::dvec2(projectedCoordinates));
  result.height = projectedCoordinates.z;
  return result;
}

CesiumGeospatial::GlobeRectangle WebMercatorProjection::unproject(
    const CesiumGeometry::Rectangle& rectangle) const noexcept {
  const Cartographic sw = this->unproject(rectangle.getLowerLeft());
  const Cartographic ne = this->unproject(rectangle.getUpperRight());
  return GlobeRectangle(sw.longitude, sw.latitude, ne.longitude, ne.latitude);
}

/*static*/ double WebMercatorProjection::mercatorAngleToGeodeticLatitude(
    double mercatorAngle) noexcept {
  return CesiumUtility::Math::PiOverTwo -
         2.0 * glm::atan(glm::exp(-mercatorAngle));
}

/*static*/ double WebMercatorProjection::geodeticLatitudeToMercatorAngle(
    double latitude) noexcept {
  // Clamp the latitude coordinate to the valid Mercator bounds.
  latitude = CesiumUtility::Math::clamp(
      latitude,
      -WebMercatorProjection::MAXIMUM_LATITUDE,
      WebMercatorProjection::MAXIMUM_LATITUDE);

  const double sinLatitude = glm::sin(latitude);
  return 0.5 * glm::log((1.0 + sinLatitude) / (1.0 - sinLatitude));
}

} // namespace CesiumGeospatial
