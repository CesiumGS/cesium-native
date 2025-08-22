#pragma once

#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Library.h>
#include <CesiumUtility/Math.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace CesiumGeospatial {

class Cartographic;

/**
 * @brief A map projection where longitude and latitude are mapped using an
 * {@link Ellipsoid}.
 *
 * The longitude and latitude are linearly mapped to X and Y by multiplying them
 * (in radians) by the {@link Ellipsoid::getMaximumRadius()}. This projection is
 * commonly known as geographic, equirectangular, equidistant cylindrical, or
 * plate carrée. It is also known as EPSG:4326.
 *
 * @see WebMercatorProjection
 */
class CESIUMGEOSPATIAL_API GeographicProjection final {
public:
  /**
   * @brief The maximum bounding rectangle of the geographic projection,
   * ranging from -PI to PI radians longitude and
   * from -PI/2 to +PI/2 radians latitude.
   */
  static constexpr GlobeRectangle MAXIMUM_GLOBE_RECTANGLE = GlobeRectangle(
      -CesiumUtility::Math::OnePi,
      -CesiumUtility::Math::PiOverTwo,
      CesiumUtility::Math::OnePi,
      CesiumUtility::Math::PiOverTwo);

  /**
   * @brief Computes the maximum rectangle that can be covered with this
   * projection
   *
   * @param ellipsoid The {@link Ellipsoid}. Default value:
   * {@link Ellipsoid::WGS84}.
   * @return The rectangle
   */
  static constexpr CesiumGeometry::Rectangle computeMaximumProjectedRectangle(
      const Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID) noexcept {
    const double longitudeValue =
        ellipsoid.getMaximumRadius() * CesiumUtility::Math::OnePi;
    const double latitudeValue =
        ellipsoid.getMaximumRadius() * CesiumUtility::Math::PiOverTwo;
    return CesiumGeometry::Rectangle(
        -longitudeValue,
        -latitudeValue,
        longitudeValue,
        latitudeValue);
  }

  /**
   * @brief Constructs a new instance.
   *
   * @param ellipsoid The {@link Ellipsoid}.
   */
  GeographicProjection(
      const Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID) noexcept;

  /**
   * @brief Gets the {@link Ellipsoid}.
   */
  const Ellipsoid& getEllipsoid() const noexcept { return this->_ellipsoid; }

  /**
   * @brief Converts geodedic ellipsoid coordinates to geographic coordinates.
   *
   * Converts geodetic ellipsoid coordinates, in radians, to the equivalent
   * geographic X, Y, Z coordinates expressed in meters.  The height is copied
   * unmodified to the `z` coordinate.
   *
   * @param cartographic The geodetic coordinates in radians.
   * @returns The equivalent geographic X, Y, Z coordinates, in meters.
   */
  glm::dvec3 project(const Cartographic& cartographic) const noexcept;

  /**
   * @brief Projects a globe rectangle to geographic coordinates.
   *
   * This is done by projecting the southwest and northeast corners.
   *
   * @param rectangle The globe rectangle to project.
   * @return The projected rectangle.
   */
  CesiumGeometry::Rectangle
  project(const CesiumGeospatial::GlobeRectangle& rectangle) const noexcept;

  /**
   * @brief Converts geographic coordinates to geodetic ellipsoid coordinates.
   *
   * Converts geographic X and Y coordinates, expressed in meters, to a
   * {@link Cartographic} containing geodetic ellipsoid coordinates.
   * The height is set to 0.0.
   *
   * @param projectedCoordinates The geographic projected coordinates to
   * unproject.
   * @returns The equivalent cartographic coordinates.
   */
  Cartographic unproject(const glm::dvec2& projectedCoordinates) const noexcept;

  /**
   * @brief Converts geographic coordinates to geodetic ellipsoid coordinates.
   *
   * Converts geographic X, Y coordinates, expressed in meters, to a
   * {@link Cartographic} containing geodetic ellipsoid coordinates.
   * The Z coordinate is copied unmodified to the height.
   *
   * @param projectedCoordinates The geographic projected coordinates to
   * unproject, with height (z) in meters.
   * @returns The equivalent cartographic coordinates.
   */
  Cartographic unproject(const glm::dvec3& projectedCoordinates) const noexcept;

  /**
   * @brief Unprojects a geographic rectangle to the globe.
   *
   * This is done by unprojecting the southwest and northeast corners.
   *
   * @param rectangle The rectangle to unproject.
   * @returns The unprojected rectangle.
   */
  CesiumGeospatial::GlobeRectangle
  unproject(const CesiumGeometry::Rectangle& rectangle) const noexcept;

  /**
   * @brief Returns `true` if two projections (i.e. their ellipsoids) are equal.
   */
  bool operator==(const GeographicProjection& rhs) const noexcept {
    return this->_ellipsoid == rhs._ellipsoid;
  };

  /**
   * @brief Returns `true` if two projections (i.e. their ellipsoids) are *not*
   * equal.
   */
  bool operator!=(const GeographicProjection& rhs) const noexcept {
    return !(*this == rhs);
  };

private:
  Ellipsoid _ellipsoid;
  double _semimajorAxis;
  double _oneOverSemimajorAxis;
};

} // namespace CesiumGeospatial
