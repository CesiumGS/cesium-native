#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeFlightPath.h>
#include <CesiumGeospatial/GlobeTransforms.h>
#include <CesiumUtility/Math.h>

#include <glm/gtx/quaternion.hpp>

namespace CesiumGeospatial {

std::optional<GlobeFlightPath>
GlobeFlightPath::fromEarthCenteredEarthFixedCoordinates(
    const glm::dvec3 sourceEcef,
    const glm::dvec3 destinationEcef) {
  std::optional<glm::dvec3> scaledSourceEcef =
      Ellipsoid::WGS84.scaleToGeodeticSurface(sourceEcef);
  std::optional<glm::dvec3> scaledDestinationEcef =
      Ellipsoid::WGS84.scaleToGeodeticSurface(destinationEcef);

  if (!scaledSourceEcef.has_value() || !scaledDestinationEcef.has_value()) {
    // Unable to scale to geodetic surface coordinates - no path we can generate
    return std::nullopt;
  }

  return GlobeFlightPath(
      scaledSourceEcef.value(),
      scaledDestinationEcef.value(),
      sourceEcef,
      destinationEcef);
}

std::optional<GlobeFlightPath> GlobeFlightPath::fromLongitudeLatitudeHeight(
    const Cartographic sourceLlh,
    const Cartographic destinationLlh) {
  return GlobeFlightPath::fromEarthCenteredEarthFixedCoordinates(
      Ellipsoid::WGS84.cartographicToCartesian(sourceLlh),
      Ellipsoid::WGS84.cartographicToCartesian(destinationLlh));
}

glm::dvec3
GlobeFlightPath::getPosition(double percentage, double additionalHeight) const {
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

  std::optional<glm::dvec3> geodeticPosition =
      Ellipsoid::WGS84.scaleToGeodeticSurface(rotatedDirection);

  // Because the start and end points mapped to valid positions on the
  // ellipsoid's surface, the rotated direction should also be a valid point on
  // the ellipsoid's surface. If it's not, something has gone wrong in our math,
  // we shouldn't try to recover from that error.
  assert(geodeticPosition.has_value());

  glm::dvec3 geodeticUp =
      Ellipsoid::WGS84.geodeticSurfaceNormal(geodeticPosition.value());

  double altitudeOffset =
      glm::mix(this->_sourceHeight, this->_destinationHeight, percentage) +
      additionalHeight;

  return geodeticPosition.value() + geodeticUp * altitudeOffset;
}

double GlobeFlightPath::getLength() const { return this->_length; }

GlobeFlightPath::GlobeFlightPath(
    glm::dvec3 scaledSourceEcef,
    glm::dvec3 scaledDestinationEcef,
    glm::dvec3 originalSourceEcef,
    glm::dvec3 originalDestinationEcef) {
  this->_destinationEcef = originalDestinationEcef;

  // Here we find the center of a circle that passes through both the source and
  // destination points, and then calculate the angle that we need to move along
  // that circle to get from point A to B.

  glm::dquat flyQuat = glm::rotation(
      glm::normalize(scaledSourceEcef),
      glm::normalize(scaledDestinationEcef));

  this->_rotationAxis = glm::axis(flyQuat);
  this->_totalAngle = glm::angle(flyQuat);

  // Ellipsoid::cartesianToCartographic will only return std::nullopt if
  // scaleToGeodeticSurface does, so we can safely assume if we're at this point
  // that we don't need to check for that condition.
  Cartographic cartographicSource =
      Ellipsoid::WGS84.cartesianToCartographic(originalSourceEcef)
          .value_or(Cartographic(0, 0, 0));

  this->_sourceHeight = cartographicSource.height;

  cartographicSource.height = 0;
  glm::dvec3 zeroHeightSource =
      Ellipsoid::WGS84.cartographicToCartesian(cartographicSource);

  this->_sourceDirection = glm::normalize(zeroHeightSource);

  Cartographic cartographicDestination =
      Ellipsoid::WGS84.cartesianToCartographic(originalDestinationEcef)
          .value_or(Cartographic(0, 0, 0));

  this->_destinationHeight = cartographicDestination.height;
  this->_length = glm::length(originalDestinationEcef - originalSourceEcef);
}

} // namespace CesiumGeospatial
