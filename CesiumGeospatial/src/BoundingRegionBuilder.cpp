#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/BoundingRegionBuilder.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumUtility/Math.h>

#include <glm/common.hpp>

#include <limits>

using namespace CesiumUtility;

namespace {

bool isCloseToPole(double latitude, double tolerance) {
  return Math::PiOverTwo - glm::abs(latitude) < tolerance;
}

} // namespace

namespace CesiumGeospatial {

BoundingRegionBuilder::BoundingRegionBuilder() noexcept
    : _poleTolerance(Math::Epsilon10),
      _rectangle(GlobeRectangle::EMPTY),
      _minimumHeight(std::numeric_limits<double>::max()),
      _maximumHeight(std::numeric_limits<double>::lowest()),
      _longitudeRangeIsEmpty(true) {}

BoundingRegion
BoundingRegionBuilder::toRegion(const Ellipsoid& ellipsoid) const {
  if (this->_longitudeRangeIsEmpty) {
    return BoundingRegion(GlobeRectangle::EMPTY, 1.0, -1.0, ellipsoid);
  } else {
    return BoundingRegion(
        this->_rectangle,
        this->_minimumHeight,
        this->_maximumHeight,
        ellipsoid);
  }
}

GlobeRectangle BoundingRegionBuilder::toGlobeRectangle() const {
  if (this->_longitudeRangeIsEmpty) {
    return GlobeRectangle::EMPTY;
  } else {
    return this->_rectangle;
  }
}

void BoundingRegionBuilder::setPoleTolerance(double tolerance) noexcept {
  this->_poleTolerance = tolerance;
}

double BoundingRegionBuilder::getPoleTolerance() const noexcept {
  return this->_poleTolerance;
}

bool BoundingRegionBuilder::expandToIncludePosition(
    const Cartographic& position) {

  // Always update the latitude and height ranges
  bool modified = false;

  if (position.latitude < this->_rectangle.getSouth()) {
    this->_rectangle.setSouth(position.latitude);
    modified = true;
  }

  if (position.latitude > this->_rectangle.getNorth()) {
    this->_rectangle.setNorth(position.latitude);
    modified = true;
  }

  if (position.height < this->_minimumHeight) {
    this->_minimumHeight = position.height;
    modified = true;
  }

  if (position.height > this->_maximumHeight) {
    this->_maximumHeight = position.height;
    modified = true;
  }

  // Only update the longitude range if this position isn't too close to the
  // North or South pole.
  if (!isCloseToPole(position.latitude, this->_poleTolerance)) {
    if (this->_longitudeRangeIsEmpty) {
      this->_rectangle.setWest(position.longitude);
      this->_rectangle.setEast(position.longitude);
      this->_longitudeRangeIsEmpty = false;
      modified = true;
    } else if (!this->_rectangle.contains(position)) {
      double positionToWestDistance =
          this->_rectangle.getWest() - position.longitude;
      if (positionToWestDistance < 0.0) {
        const double antiMeridianToWest =
            this->_rectangle.getWest() - (-Math::OnePi);
        const double positionToAntiMeridian = Math::OnePi - position.longitude;
        positionToWestDistance = antiMeridianToWest + positionToAntiMeridian;
      }

      double eastToPositionDistance =
          position.longitude - this->_rectangle.getEast();
      if (eastToPositionDistance < 0.0) {
        const double antiMeridianToPosition =
            position.longitude - (-Math::OnePi);
        const double eastToAntiMeridian =
            Math::OnePi - this->_rectangle.getEast();
        eastToPositionDistance = antiMeridianToPosition + eastToAntiMeridian;
      }

      if (positionToWestDistance < eastToPositionDistance) {
        this->_rectangle.setWest(position.longitude);
      } else {
        this->_rectangle.setEast(position.longitude);
      }

      modified = true;
    }
  }

  return modified;
}

} // namespace CesiumGeospatial
