#include "CesiumGeospatial/GlobeRectangle.h"

#include <CesiumUtility/Math.h>

using namespace CesiumUtility;

namespace CesiumGeospatial {

Cartographic GlobeRectangle::computeCenter() const noexcept {
  double east = this->_east;
  double west = this->_west;

  if (east < west) {
    east += Math::TWO_PI;
  }

  double longitude = Math::negativePiToPi((west + east) * 0.5);
  double latitude = (this->_south + this->_north) * 0.5;

  return Cartographic(longitude, latitude, 0.0);
}

bool GlobeRectangle::contains(const Cartographic& cartographic) const noexcept {
  double longitude = cartographic.longitude;
  double latitude = cartographic.latitude;

  double west = this->_west;
  double east = this->_east;

  if (east < west) {
    east += Math::TWO_PI;
    if (longitude < 0.0) {
      longitude += Math::TWO_PI;
    }
  }
  return (
      (longitude > west ||
       Math::equalsEpsilon(longitude, west, Math::EPSILON14)) &&
      (longitude < east ||
       Math::equalsEpsilon(longitude, east, Math::EPSILON14)) &&
      latitude >= this->_south && latitude <= this->_north);
}

std::optional<GlobeRectangle> GlobeRectangle::computeIntersection(
    const GlobeRectangle& other) const noexcept {
  double rectangleEast = this->_east;
  double rectangleWest = this->_west;

  double otherRectangleEast = other._east;
  double otherRectangleWest = other._west;

  if (rectangleEast < rectangleWest && otherRectangleEast > 0.0) {
    rectangleEast += CesiumUtility::Math::TWO_PI;
  } else if (otherRectangleEast < otherRectangleWest && rectangleEast > 0.0) {
    otherRectangleEast += CesiumUtility::Math::TWO_PI;
  }

  if (rectangleEast < rectangleWest && otherRectangleWest < 0.0) {
    otherRectangleWest += CesiumUtility::Math::TWO_PI;
  } else if (otherRectangleEast < otherRectangleWest && rectangleWest < 0.0) {
    rectangleWest += CesiumUtility::Math::TWO_PI;
  }

  double west = CesiumUtility::Math::negativePiToPi(
      glm::max(rectangleWest, otherRectangleWest));
  double east = CesiumUtility::Math::negativePiToPi(
      glm::min(rectangleEast, otherRectangleEast));

  if ((this->_west < this->_east || other._west < other._east) &&
      east <= west) {
    return std::nullopt;
  }

  double south = glm::max(this->_south, other._south);
  double north = glm::min(this->_north, other._north);

  if (south >= north) {
    return std::nullopt;
  }

  return GlobeRectangle(west, south, east, north);
}

GlobeRectangle
GlobeRectangle::computeUnion(const GlobeRectangle& other) const noexcept {
  double rectangleEast = this->_east;
  double rectangleWest = this->_west;

  double otherRectangleEast = other._east;
  double otherRectangleWest = other._west;

  if (rectangleEast < rectangleWest && otherRectangleEast > 0.0) {
    rectangleEast += CesiumUtility::Math::TWO_PI;
  } else if (otherRectangleEast < otherRectangleWest && rectangleEast > 0.0) {
    otherRectangleEast += CesiumUtility::Math::TWO_PI;
  }

  if (rectangleEast < rectangleWest && otherRectangleWest < 0.0) {
    otherRectangleWest += CesiumUtility::Math::TWO_PI;
  } else if (otherRectangleEast < otherRectangleWest && rectangleWest < 0.0) {
    rectangleWest += CesiumUtility::Math::TWO_PI;
  }

  double west = CesiumUtility::Math::convertLongitudeRange(
      glm::min(rectangleWest, otherRectangleWest));
  double east = CesiumUtility::Math::convertLongitudeRange(
      glm::max(rectangleEast, otherRectangleEast));

  return GlobeRectangle(
      west,
      glm::min(this->_south, other._south),
      east,
      glm::max(this->_north, other._north));
}

} // namespace CesiumGeospatial
