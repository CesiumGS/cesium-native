#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/SimplePlanarEllipsoidCurve.h>

#include <glm/common.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/gtx/quaternion.hpp>

#include <optional>

namespace CesiumGeospatial {

std::optional<SimplePlanarEllipsoidCurve>
SimplePlanarEllipsoidCurve::fromEarthCenteredEarthFixedCoordinates(
    const Ellipsoid& ellipsoid,
    const glm::dvec3& sourceEcef,
    const glm::dvec3& destinationEcef) {
  std::optional<glm::dvec3> scaledSourceEcef =
      ellipsoid.scaleToGeocentricSurface(sourceEcef);
  std::optional<glm::dvec3> scaledDestinationEcef =
      ellipsoid.scaleToGeocentricSurface(destinationEcef);

  if (!scaledSourceEcef.has_value() || !scaledDestinationEcef.has_value()) {
    // Unable to scale to geocentric surface coordinates - no curve we can
    // generate
    return std::optional<SimplePlanarEllipsoidCurve>();
  }

  return SimplePlanarEllipsoidCurve(
      ellipsoid,
      scaledSourceEcef.value(),
      scaledDestinationEcef.value(),
      sourceEcef,
      destinationEcef);
}

std::optional<SimplePlanarEllipsoidCurve>
SimplePlanarEllipsoidCurve::fromLongitudeLatitudeHeight(
    const Ellipsoid& ellipsoid,
    const Cartographic& source,
    const Cartographic& destination) {
  return SimplePlanarEllipsoidCurve::fromEarthCenteredEarthFixedCoordinates(
      ellipsoid,
      ellipsoid.cartographicToCartesian(source),
      ellipsoid.cartographicToCartesian(destination));
}

glm::dvec3 SimplePlanarEllipsoidCurve::getPosition(
    double percentage,
    double additionalHeight) const {
  if (percentage <= 0.0) {
    return this->_sourceEcef;
  } else if (percentage >= 1.0) {
    // We can shortcut our math here and just return the destination.
    return this->_destinationEcef;
  }

  percentage = glm::clamp(percentage, 0.0, 1.0);

  // Rotate us around the circle between points A and B by the given percentage
  // of the total angle we're rotating by.
  glm::dvec3 rotatedDirection =
      glm::angleAxis(percentage * this->_totalAngle, this->_rotationAxis) *
      this->_sourceDirection;

  // It's safe for us to assume here that scaleToGeocentricSurface will return a
  // value, since rotatedDirection should never be (0, 0, 0)
  glm::dvec3 geocentricPosition =
      this->_ellipsoid.scaleToGeocentricSurface(rotatedDirection)
          .value_or(glm::dvec3(0, 0, 0));

  glm::dvec3 geocentricUp = glm::normalize(geocentricPosition);

  double altitudeOffset =
      glm::mix(this->_sourceHeight, this->_destinationHeight, percentage) +
      additionalHeight;

  return geocentricPosition + geocentricUp * altitudeOffset;
}

SimplePlanarEllipsoidCurve::SimplePlanarEllipsoidCurve(
    const Ellipsoid& ellipsoid,
    const glm::dvec3& scaledSourceEcef,
    const glm::dvec3& scaledDestinationEcef,
    const glm::dvec3& originalSourceEcef,
    const glm::dvec3& originalDestinationEcef)
    : _sourceHeight(
          glm::length(originalSourceEcef) - glm::length(scaledSourceEcef)),
      _destinationHeight(
          glm::length(originalDestinationEcef) -
          glm::length(scaledDestinationEcef)),
      _ellipsoid(ellipsoid),
      _sourceDirection(glm::normalize(originalSourceEcef)),
      _sourceEcef(originalSourceEcef),
      _destinationEcef(originalDestinationEcef) {
  // Here we find the center of a circle that passes through both the source and
  // destination points, and then calculate the angle that we need to move along
  // that circle to get from point A to B.

  glm::dquat flyQuat = glm::rotation(
      glm::normalize(scaledSourceEcef),
      glm::normalize(scaledDestinationEcef));

  this->_rotationAxis = glm::axis(flyQuat);
  this->_totalAngle = glm::angle(flyQuat);
}

} // namespace CesiumGeospatial
