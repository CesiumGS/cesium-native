#pragma once

#include "Ellipsoid.h"
#include "Library.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace CesiumGeospatial {
class Cartographic;
}

namespace CesiumGeospatial {

enum class LocalDirection { East, North, West, South, Up, Down };

/**
 * @brief A coordinate system created from a local horizontal plane at a
 * particular origin point on the globe.
 *
 * Each principal axis in the new coordinate system can be specified to point in
 * a particular compass direction, or it can point up or down. In this way, a
 * right-handed or left-handed coordinate system can be created.
 */
class CESIUMGEOSPATIAL_API LocalHorizontalCoordinateSystem {
public:
  /**
   * @brief Create a new coordinate system centered at a given longitude,
   * latitude, and ellipsoidal height.
   *
   * @param origin The origin of the coordinate system.
   * @param xAxisDirection The local direction in which the X axis points at the
   * origin.
   * @param yAxisDirection The local direction in which the Y axis points at the
   * origin.
   * @param zAxisDirection The local direction in which the Z axis points at the
   * origin.
   * @param scaleToMeters Local units are converted to meters by multiplying
   * them by this factor. For example, if the local units are centimeters, this
   * parameter should be 1.0 / 100.0.
   * @param ellipsoid The ellipsoid on which the coordinate system is based.
   */
  LocalHorizontalCoordinateSystem(
      const Cartographic& origin,
      LocalDirection xAxisDirection = LocalDirection::East,
      LocalDirection yAxisDirection = LocalDirection::North,
      LocalDirection zAxisDirection = LocalDirection::Up,
      double scaleToMeters = 1.0,
      const Ellipsoid& ellipsoid = Ellipsoid::WGS84);

  /**
   * @brief Create a new coordinate system centered at a Earth-Centered,
   * Earth-Fixed position.
   *
   * @param origin The origin of the coordinate system.
   * @param xAxisDirection The local direction in which the X axis points at the
   * origin.
   * @param yAxisDirection The local direction in which the Y axis points at the
   * origin.
   * @param zAxisDirection The local direction in which the Z axis points at the
   * origin.
   * @param scaleToMeters Local units are converted to meters by multiplying
   * them by this factor. For example, if the local units are centimeters, this
   * parameter should be 1.0 / 100.0.
   * @param ellipsoid The ellipsoid on which the coordinate system is based.
   */
  LocalHorizontalCoordinateSystem(
      const glm::dvec3& originEcef,
      LocalDirection xAxisDirection = LocalDirection::East,
      LocalDirection yAxisDirection = LocalDirection::North,
      LocalDirection zAxisDirection = LocalDirection::Up,
      double scaleToMeters = 1.0,
      const Ellipsoid& ellipsoid = Ellipsoid::WGS84);

  /**
   * @brief Gets the transformation matrix from the local horizontal coordinate
   * system managed by this instance to the Earth-Centered, Earth-fixed
   * coordinate system.
   *
   * @return The transformation.
   */
  const glm::dmat4& getLocalToEcefTransformation() const noexcept {
    return this->_localToEcef;
  }

  /**
   * @brief Gets the transformation matrix from the Earth-Centered, Earth-Fixed
   * (ECEF) coordinate system to the local horizontal coordinate system managed
   * by this instance.
   *
   * @return The transformation.
   */
  const glm::dmat4& getEcefToLocalTransformation() const noexcept {
    return this->_ecefToLocal;
  };

  /**
   * @brief Converts a position in the local horizontal coordinate system
   * managed by this instance to Earth-Centered, Earth-Fixed (ECEF).
   *
   * @param localPosition The position in the local coordinate system.
   * @return The equivalent position in the ECEF coordinate system.
   */
  glm::dvec3
  localPositionToEcef(const glm::dvec3& localPosition) const noexcept;

  /**
   * @brief Converts a position in the Earth-Centered, Earth-Fixed (ECEF)
   * coordinate system to the local horizontal coordinate system managed by this
   * instance.
   *
   * @param ecefPosition The position in the ECEF coordinate system.
   * @return The equivalent position in the local coordinate system.
   */
  glm::dvec3 ecefPositionToLocal(const glm::dvec3& ecefPosition) const noexcept;

  /**
   * @brief Converts a direction in the local horizontal coordinate system
   * managed by this instance to Earth-Centered, Earth-Fixed (ECEF).
   *
   * Because the vector is treated as a direction only, the translation portion
   * of the transformation is ignored.
   *
   * @param localPosition The direction in the local coordinate system.
   * @return The equivalent direction in the ECEF coordinate system.
   */
  glm::dvec3
  localDirectionToEcef(const glm::dvec3& localDirection) const noexcept;

  /**
   * @brief Converts a direction in the Earth-Centered, Earth-Fixed (ECEF)
   * coordinate system to the local horizontal coordinate system managed by this
   * instance.
   *
   * Because the vector is treated as a direction only, the translation portion
   * of the transformation is ignored.
   *
   * @param ecefPosition The direction in the ECEF coordinate system.
   * @return The equivalent direction in the local coordinate system.
   */
  glm::dvec3
  ecefDirectionToLocal(const glm::dvec3& ecefDirection) const noexcept;

  /**
   * @brief Computes the transformation matrix from this local horizontal
   * coordinate system to another one. For example, if the returned matrix is
   * multiplied with a vector representing a position in this coordinate system,
   * the result will be a new vector representing the position in the target
   * coordinate system.
   *
   * @param target The other local horizontal coordinate system.
   * @return The transformation.
   */
  glm::dmat4 computeTransformationToAnotherLocal(
      const LocalHorizontalCoordinateSystem& target) const noexcept;

private:
  glm::dmat4 _ecefToLocal;
  glm::dmat4 _localToEcef;
};

} // namespace CesiumGeospatial
