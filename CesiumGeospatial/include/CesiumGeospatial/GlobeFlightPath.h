#pragma once

#include "Library.h"

#include <CesiumGeospatial/Ellipsoid.h>

#include <glm/vec3.hpp>

#include <optional>

namespace CesiumGeospatial {

/**
 * @brief Describes a flight path along the surface of a WGS84 ellipsoid
 * interpolating between two points. The path is continuous and can be sampled,
 * for example, to move a camera between two points on the ellipsoid.
 */
class CESIUMGEOSPATIAL_API GlobeFlightPath final {
public:
  /**
   * @brief Creates a new instance of \ref GlobeFlightPath from a source and
   * destination specified in Earth-Centered, Earth-Fixed coordinates.
   *
   * @param sourceEcef The position that the path will begin at in ECEF
   * coordinates.
   * @param destinationEcef The position that the path will end at in ECEF
   * coordinates.
   *
   * @returns An optional type containing a \ref GlobeFlightPath object
   * representing the generated path, if possible. If it wasn't possible to
   * scale the input coordinates to geodetic surface coordinates on a WGS84
   * ellipsoid, this will return \ref std::nullopt instead.
   */
  static std::optional<GlobeFlightPath> fromEarthCenteredEarthFixedCoordinates(
      const glm::dvec3 sourceEcef,
      const glm::dvec3 destinationEcef);

  /**
   * @brief Creates a new instance of \ref GlobeFlightPath from a source and
   * destination specified in cartographic coordinates (Longitude, Latitude, and
   * Height).
   *
   * @param sourceLlm The position that the path will begin at in Longitude,
   * Latitude, and Height.
   * @param destinationLlm The position that the path will end at in Longitude,
   * Latitude, and Height.
   *
   * @returns An optional type containing a \ref GlobeFlightPath object
   * representing the generated path, if possible. If it wasn't possible to
   * scale the input coordinates to geodetic surface coordinates on a WGS84
   * ellipsoid, this will return \ref std::nullopt instead.
   */
  static std::optional<GlobeFlightPath> fromLongitudeLatitudeHeight(
      const Cartographic sourceLlm,
      const Cartographic destinationLlm);

  /**
   * @brief Samples the flight path at the given percentage of its length.
   *
   * @param percentage The percentage of the flight path's length to sample at,
   * where 0 is the beginning and 1 is the end. This value will be clamped to
   * the range [0..1].
   * @param additionalHeight The height above the earth at this position will be
   * calculated by interpolating between the height at the beginning and end of
   * the curve based on the value of \p percentage. This parameter specifies an
   * additional offset to add to the height.
   *
   * @returns The position of the given point on this path in Earth-Centered
   * Earth-Fixed coordinates.
   */
  glm::dvec3
  getPosition(double percentage, double additionalHeight = 0.0) const;

  /**
   * @brief Returns the length in meters of this path, measured "as the crow
   * flies" in a straight line.
   *
   * @remarks This is the equivalent of the magnitude of the vector
   * (destinationEcef - sourceEcef).
   */
  double getLength() const;

private:
  GlobeFlightPath(
      glm::dvec3 scaledSourceEcef,
      glm::dvec3 scaledDestinationEcef,
      glm::dvec3 originalSourceEcef,
      glm::dvec3 originalDestinationEcef);

  double _totalAngle;
  double _sourceHeight;
  double _destinationHeight;
  double _length;

  glm::dvec3 _sourceDirection;
  glm::dvec3 _rotationAxis;
  glm::dvec3 _destinationEcef;
};

} // namespace CesiumGeospatial
