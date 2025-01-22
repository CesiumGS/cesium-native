#pragma once

#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/Library.h>

#include <glm/vec3.hpp>

#include <optional>

namespace CesiumGeospatial {

/**
 * @brief Produces points on an ellipse that lies on a plane that intersects the
 * center of the earth and each of the input coordinates. The height above the
 * surface at each point along the curve will be a linear interpolation between
 * the source and destination heights.
 */
class CESIUMGEOSPATIAL_API SimplePlanarEllipsoidCurve final {
public:
  /**
   * @brief Creates a new instance of {@link SimplePlanarEllipsoidCurve} from a
   * source and destination specified in Earth-Centered, Earth-Fixed
   * coordinates.
   *
   * @param ellipsoid The ellipsoid that the source and destination positions
   * are relative to.
   * @param sourceEcef The position that the path will begin at in ECEF
   * coordinates.
   * @param destinationEcef The position that the path will end at in ECEF
   * coordinates.
   *
   * @returns An optional type containing a {@link SimplePlanarEllipsoidCurve}
   * object representing the generated path, if possible. If it wasn't possible
   * to scale the input coordinates to geodetic surface coordinates on a WGS84
   * ellipsoid, this will return `std::nullopt` instead.
   */
  static std::optional<SimplePlanarEllipsoidCurve>
  fromEarthCenteredEarthFixedCoordinates(
      const Ellipsoid& ellipsoid,
      const glm::dvec3& sourceEcef,
      const glm::dvec3& destinationEcef);

  /**
   * @brief Creates a new instance of {@link SimplePlanarEllipsoidCurve} from a
   * source and destination specified in cartographic coordinates (Longitude,
   * Latitude, and Height).
   *
   * @param ellipsoid The ellipsoid that these cartographic coordinates are
   * from.
   * @param source The position that the path will begin at in Longitude,
   * Latitude, and Height.
   * @param destination The position that the path will end at in Longitude,
   * Latitude, and Height.
   *
   * @returns An optional type containing a {@link SimplePlanarEllipsoidCurve}
   * object representing the generated path, if possible. If it wasn't possible
   * to scale the input coordinates to geodetic surface coordinates on a WGS84
   * ellipsoid, this will return std::nullopt instead.
   */
  static std::optional<SimplePlanarEllipsoidCurve> fromLongitudeLatitudeHeight(
      const Ellipsoid& ellipsoid,
      const Cartographic& source,
      const Cartographic& destination);

  /**
   * @brief Samples the curve at the given percentage of its length.
   *
   * @param percentage The percentage of the curve's length to sample at,
   * where 0 is the beginning and 1 is the end. This value will be clamped to
   * the range [0..1].
   * @param additionalHeight The height above the earth at this position will be
   * calculated by interpolating between the height at the beginning and end of
   * the curve based on the value of \p percentage. This parameter specifies an
   * additional offset to add to the height.
   *
   * @returns The position of the given point on this curve in Earth-Centered
   * Earth-Fixed coordinates.
   */
  glm::dvec3
  getPosition(double percentage, double additionalHeight = 0.0) const;

private:
  SimplePlanarEllipsoidCurve(
      const Ellipsoid& ellipsoid,
      const glm::dvec3& scaledSourceEcef,
      const glm::dvec3& scaledDestinationEcef,
      const glm::dvec3& originalSourceEcef,
      const glm::dvec3& originalDestinationEcef);

  double _totalAngle;
  double _sourceHeight;
  double _destinationHeight;

  Ellipsoid _ellipsoid;
  glm::dvec3 _sourceDirection;
  glm::dvec3 _rotationAxis;
  glm::dvec3 _sourceEcef;
  glm::dvec3 _destinationEcef;
};

} // namespace CesiumGeospatial
