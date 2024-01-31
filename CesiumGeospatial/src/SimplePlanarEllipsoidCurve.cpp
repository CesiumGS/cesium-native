#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeTransforms.h>
#include <CesiumGeospatial/SimplePlanarEllipsoidCurve.h>
#include <CesiumUtility/Math.h>

#include <glm/gtx/quaternion.hpp>

namespace CesiumGeospatial {

std::optional<SimplePlanarEllipsoidCurve>
SimplePlanarEllipsoidCurve::fromEarthCenteredEarthFixedCoordinates(
    const glm::dvec3& sourceEcef,
    const glm::dvec3& destinationEcef) {
  std::optional<glm::dvec3> scaledSourceEcef =
      Ellipsoid::WGS84.scaleToGeocentricSurface(sourceEcef);
  std::optional<glm::dvec3> scaledDestinationEcef =
      Ellipsoid::WGS84.scaleToGeocentricSurface(destinationEcef);

  if (!scaledSourceEcef.has_value() || !scaledDestinationEcef.has_value()) {
    // Unable to scale to geodetic surface coordinates - no path we can generate
    return std::nullopt;
  }

  return SimplePlanarEllipsoidCurve(
      scaledSourceEcef.value(),
      scaledDestinationEcef.value(),
      sourceEcef,
      destinationEcef);
}

std::optional<SimplePlanarEllipsoidCurve>
SimplePlanarEllipsoidCurve::fromLongitudeLatitudeHeight(
    const Cartographic& sourceLlh,
    const Cartographic& destinationLlh) {
  return SimplePlanarEllipsoidCurve::fromEarthCenteredEarthFixedCoordinates(
      Ellipsoid::WGS84.cartographicToCartesian(sourceLlh),
      Ellipsoid::WGS84.cartographicToCartesian(destinationLlh));
}

glm::dvec3 SimplePlanarEllipsoidCurve::getPosition(
    double percentage,
    double additionalHeight) const {
  if (percentage >= 1.0) {
    // We can shortcut our math here and just return the destination.
    return this->_destinationEcef;
  }

  percentage = glm::clamp(percentage, 0.0, 1.0);

  // Rotate us around the circle between points A and B by the given percentage
  // of the total angle we're rotating by.
  glm::dvec3 rotatedDirection =
      glm::angleAxis(percentage * this->_totalAngle, this->_rotationAxis) *
      this->_sourceDirection;

  glm::dvec3 geocentricPosition =
      Ellipsoid::WGS84.scaleToGeocentricSurface(rotatedDirection);

  glm::dvec3 geocentricUp = glm::normalize(geocentricPosition);

  double altitudeOffset =
      glm::mix(this->_sourceHeight, this->_destinationHeight, percentage) +
      additionalHeight;

  return geocentricPosition + geocentricUp * altitudeOffset;
}

SimplePlanarEllipsoidCurve::SimplePlanarEllipsoidCurve(
    const glm::dvec3& scaledSourceEcef,
    const glm::dvec3& scaledDestinationEcef,
    const glm::dvec3& originalSourceEcef,
    const glm::dvec3& originalDestinationEcef) {
  this->_destinationEcef = originalDestinationEcef;

  // Here we find the center of a circle that passes through both the source and
  // destination points, and then calculate the angle that we need to move along
  // that circle to get from point A to B.

  glm::dquat flyQuat = glm::rotation(
      glm::normalize(scaledSourceEcef),
      glm::normalize(scaledDestinationEcef));

  this->_rotationAxis = glm::axis(flyQuat);
  this->_totalAngle = glm::angle(flyQuat);

  this->_sourceHeight = glm::length(originalSourceEcef - scaledSourceEcef);
  this->_destinationHeight =
      glm::length(originalDestinationEcef - scaledDestinationEcef);

  this->_sourceDirection =
      glm::normalize(originalSourceEcef - scaledSourceEcef);
}

} // namespace CesiumGeospatial
