#pragma once

#include <CesiumGeospatial/Library.h>
#include <CesiumUtility/Math.h>

namespace CesiumGeospatial {

/**
 * @brief A position defined by longitude, latitude, and height.
 */
class CESIUMGEOSPATIAL_API Cartographic final {
public:
  /**
   * @brief Creates a new instance.
   *
   * @param longitudeRadians The longitude, in radians.
   * @param latitudeRadians The latitude, in radians.
   * @param heightMeters The height, in meters. Default value: 0.0.
   */
  constexpr Cartographic(
      double longitudeRadians,
      double latitudeRadians,
      double heightMeters = 0.0) noexcept
      : longitude(longitudeRadians),
        latitude(latitudeRadians),
        height(heightMeters) {}

  /**
   * @brief Creates a new instance from a longitude and latitude specified in
   * degrees, and a height given in meters.
   *
   * The values in the resulting object will be in radians.
   *
   * @param longitudeDegrees The longitude, in degrees.
   * @param latitudeDegrees The latitude, in degrees.
   * @param heightMeters The height, in meters. Default value: 0.0.
   */
  static constexpr Cartographic fromDegrees(
      double longitudeDegrees,
      double latitudeDegrees,
      double heightMeters = 0.0) noexcept {
    return Cartographic(
        CesiumUtility::Math::degreesToRadians(longitudeDegrees),
        CesiumUtility::Math::degreesToRadians(latitudeDegrees),
        heightMeters);
  }

  /**
   * @brief Returns `true` if two cartographics are equal.
   */
  constexpr bool operator==(const Cartographic& rhs) const noexcept {
    return this->longitude == rhs.longitude && this->latitude == rhs.latitude &&
           this->height == rhs.height;
  };

  /**
   * @brief The longitude, in radians.
   */
  double longitude;

  /**
   * @brief The latitude, in radians.
   */
  double latitude;

  /**
   * @brief The height, in meters.
   */
  double height;
};
} // namespace CesiumGeospatial
