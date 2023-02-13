#pragma once

#include "Library.h"

#include <glm/fwd.hpp>
#include <glm/mat4x4.hpp>

namespace CesiumGeometry {

/**
 * @brief Coordinate system matrix constructions helpers.
 */
struct CESIUMGEOMETRY_API Transforms final {

  /**
   * @brief A matrix to convert from y-up to z-up orientation,
   * by rotating about PI/2 around the x-axis
   */
  static const glm::dmat4 Y_UP_TO_Z_UP;

  /**
   * @brief A matrix to convert from z-up to y-up orientation,
   * by rotating about -PI/2 around the x-axis
   */
  static const glm::dmat4 Z_UP_TO_Y_UP;

  /**
   * @brief A matrix to convert from x-up to z-up orientation,
   * by rotating about -PI/2 around the y-axis
   */
  static const glm::dmat4 X_UP_TO_Z_UP;

  /**
   * @brief A matrix to convert from z-up to x-up orientation,
   * by rotating about PI/2 around the y-axis
   */
  static const glm::dmat4 Z_UP_TO_X_UP;

  /**
   * @brief A matrix to convert from x-up to y-up orientation,
   * by rotating about PI/2 around the z-axis
   */
  static const glm::dmat4 X_UP_TO_Y_UP;

  /**
   * @brief A matrix to convert from y-up to x-up orientation,
   * by rotating about -PI/2 around the z-axis
   */
  static const glm::dmat4 Y_UP_TO_X_UP;

  /**
   * @brief Constructs a translation-rotation-scale matrix, equivalent to
   * `translation * rotation * scale`. So if a vector is multiplied with the
   * resulting matrix, it will be first scaled, then rotated, then translated.
   *
   * @param translation The translation.
   * @param rotation The rotation.
   * @param scale The scale.
   */
  static glm::dmat4 translationRotationScale(
      const glm::dvec3& translation,
      const glm::dquat& rotation,
      const glm::dvec3& scale);
};

} // namespace CesiumGeometry
