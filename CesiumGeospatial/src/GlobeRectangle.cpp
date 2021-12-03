#include "CesiumGeospatial/GlobeRectangle.h"

#include <CesiumUtility/Math.h>

using namespace CesiumUtility;

namespace CesiumGeospatial {

/*static*/ const GlobeRectangle GlobeRectangle::EMPTY{
    Math::ONE_PI,
    Math::PI_OVER_TWO,
    -Math::ONE_PI,
    -Math::PI_OVER_TWO};

Cartographic GlobeRectangle::computeCenter() const noexcept {
  double latitudeCenter = (this->_south + this->_north) * 0.5;

  if (this->_west <= this->_east) {
    // Simple rectangle not crossing the anti-meridian.
    return Cartographic((this->_west + this->_east) * 0.5, latitudeCenter, 0.0);
  } else {
    // Rectangle crosses the anti-meridian.
    double westToAntiMeridian = Math::ONE_PI - this->_west;
    double antiMeridianToEast = this->_east - -Math::ONE_PI;
    double total = westToAntiMeridian + antiMeridianToEast;
    if (westToAntiMeridian >= antiMeridianToEast) {
      // Center is in the Eastern hemisphere.
      return Cartographic(
          glm::min(Math::ONE_PI, this->_west + total * 0.5),
          latitudeCenter,
          0.0);
    } else {
      // Center is in the Western hemisphere.
      return Cartographic(
          glm::max(-Math::ONE_PI, this->_east - total * 0.5),
          latitudeCenter,
          0.0);
    }
  }
}

bool GlobeRectangle::contains(const Cartographic& cartographic) const noexcept {
  const double latitude = cartographic.latitude;
  if (latitude < this->_south || latitude > this->_north) {
    return false;
  }

  const double longitude = cartographic.longitude;
  if (this->_west <= this->_east) {
    // Simple rectangle not crossing the anti-meridian.
    return longitude >= this->_west && longitude <= this->_east;
  } else {
    // Rectangle crosses the anti-meridian.
    return longitude >= this->_west || longitude <= this->_east;
  }
}

bool GlobeRectangle::isEmpty() const noexcept {
  return this->_south > this->_north;
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

  const double west = CesiumUtility::Math::negativePiToPi(
      glm::max(rectangleWest, otherRectangleWest));
  const double east = CesiumUtility::Math::negativePiToPi(
      glm::min(rectangleEast, otherRectangleEast));

  if ((this->_west < this->_east || other._west < other._east) &&
      east <= west) {
    return std::nullopt;
  }

  const double south = glm::max(this->_south, other._south);
  const double north = glm::min(this->_north, other._north);

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

  const double west = CesiumUtility::Math::convertLongitudeRange(
      glm::min(rectangleWest, otherRectangleWest));
  const double east = CesiumUtility::Math::convertLongitudeRange(
      glm::max(rectangleEast, otherRectangleEast));

  return GlobeRectangle(
      west,
      glm::min(this->_south, other._south),
      east,
      glm::max(this->_north, other._north));
}

} // namespace CesiumGeospatial
