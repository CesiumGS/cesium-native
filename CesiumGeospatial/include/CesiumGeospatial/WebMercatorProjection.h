#pragma once

#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Library.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace CesiumGeospatial {

class Cartographic;

/**
 * @brief The map projection used by Google Maps, Bing Maps, and most of ArcGIS
 * Online, EPSG:3857.
 *
 * This projection uses geodetic longitude and latitude expressed with WGS84 and
 * transforms them to Mercator using the spherical (rather than ellipsoidal)
 * equations.
 *
 * @see GeographicProjection
 */
class CESIUMGEOSPATIAL_API WebMercatorProjection final {
public:
  /**
   * @brief The maximum latitude (both North and South) supported by a Web
   * Mercator (EPSG:3857) projection.
   *
   * Technically, the Mercator projection is defined for any latitude
   * up to (but not including) 90 degrees, but it makes sense
   * to cut it off sooner because it grows exponentially with increasing
   * latitude. The logic behind this particular cutoff value, which is the one
   * used by Google Maps, Bing Maps, and Esri, is that it makes the projection
   * square.  That is, the rectangle is equal in the X and Y directions.
   *
   * The constant value is computed by calling:
   *    `CesiumGeospatial::WebMercatorProjection::mercatorAngleToGeodeticLatitude(CesiumUtility::Math::OnePi)`
   */
  static const double MAXIMUM_LATITUDE;

  /**
   * @brief The maximum bounding rectangle of the Web Mercator projection,
   * ranging from -PI to PI radians longitude and
   * from -MAXIMUM_LATITUDE to +MAXIMUM_LATITUDE.
   */
  static const GlobeRectangle MAXIMUM_GLOBE_RECTANGLE;

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
    const double value =
        ellipsoid.getMaximumRadius() * CesiumUtility::Math::OnePi;
    return CesiumGeometry::Rectangle(-value, -value, value, value);
  }

  /**
   * @brief Constructs a new instance.
   *
   * @param ellipsoid The {@link Ellipsoid}.
   */
  WebMercatorProjection(
      const Ellipsoid& ellipsoid CESIUM_DEFAULT_ELLIPSOID) noexcept;

  /**
   * @brief Gets the {@link Ellipsoid}.
   */
  const Ellipsoid& getEllipsoid() const noexcept { return this->_ellipsoid; }

  /**
   * @brief Converts geodedic ellipsoid coordinates to Web Mercator coordinates.
   *
   * Converts geodetic ellipsoid coordinates, in radians, to the equivalent Web
   * Mercator X, Y, Z coordinates expressed in meters.  The height is copied
   * unmodified to the `z` coordinate.
   *
   * @param cartographic The geodetic coordinates in radians.
   * @returns The equivalent web mercator X, Y, Z coordinates, in meters.
   */
  glm::dvec3 project(const Cartographic& cartographic) const noexcept;

  /**
   * @brief Projects a globe rectangle to Web Mercator coordinates.
   *
   * This is done by projecting the southwest and northeast corners.
   *
   * @param rectangle The globe rectangle to project.
   * @return The projected rectangle.
   */
  CesiumGeometry::Rectangle
  project(const CesiumGeospatial::GlobeRectangle& rectangle) const noexcept;

  /**
   * @brief Converts Web Mercator coordinates to geodetic ellipsoid coordinates.
   *
   * Converts Web Mercator X and Y coordinates, expressed in meters, to a
   * {@link Cartographic} containing geodetic ellipsoid coordinates.
   * The height is set to 0.0.
   *
   * @param projectedCoordinates The web mercator projected coordinates to
   * unproject.
   * @returns The equivalent cartographic coordinates.
   */
  Cartographic unproject(const glm::dvec2& projectedCoordinates) const noexcept;

  /**
   * @brief Converts Web Mercator coordinates to geodetic ellipsoid coordinates.
   *
   * Converts Web Mercator X, Y coordinates, expressed in meters, to a
   * {@link Cartographic} containing geodetic ellipsoid coordinates.
   * The Z coordinate is copied unmodified to the height.
   *
   * @param projectedCoordinates The web mercator projected coordinates to
   * unproject, with height (z) in meters.
   * @returns The equivalent cartographic coordinates.
   */
  Cartographic unproject(const glm::dvec3& projectedCoordinates) const noexcept;

  /**
   * @brief Unprojects a Web Mercator rectangle to the globe.
   *
   * This is done by unprojecting the southwest and northeast corners.
   *
   * @param rectangle The rectangle to unproject.
   * @returns The unprojected rectangle.
   */
  CesiumGeospatial::GlobeRectangle
  unproject(const CesiumGeometry::Rectangle& rectangle) const noexcept;

  /**
   * @brief Converts a Mercator angle, in the range -PI to PI, to a geodetic
   * latitude in the range -PI/2 to PI/2.
   *
   * @param mercatorAngle The angle to convert.
   * @returns The geodetic latitude in radians.
   */
  static double mercatorAngleToGeodeticLatitude(double mercatorAngle) noexcept;

  /**
   * @brief Converts a geodetic latitude in radians, in the range -PI/2 to PI/2,
   * to a Mercator angle in the range -PI to PI.
   *
   * @param latitude The geodetic latitude in radians.
   * @returns The Mercator angle.
   */
  static double geodeticLatitudeToMercatorAngle(double latitude) noexcept;

  /**
   * @brief Returns `true` if two projections (i.e. their ellipsoids) are equal.
   */
  bool operator==(const WebMercatorProjection& rhs) const noexcept {
    return this->_ellipsoid == rhs._ellipsoid;
  };

  /**
   * @brief Returns `true` if two projections (i.e. their ellipsoids) are *not*
   * equal.
   */
  bool operator!=(const WebMercatorProjection& rhs) const noexcept {
    return !(*this == rhs);
  };

private:
  Ellipsoid _ellipsoid;
  double _semimajorAxis;
  double _oneOverSemimajorAxis;
};

} // namespace CesiumGeospatial
