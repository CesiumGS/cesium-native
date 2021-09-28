#pragma once

#include "Library.h"

#include <glm/mat3x3.hpp>

namespace CesiumGeometry {

/**
 * @brief Coordinate system conversion matrices
 */
struct CESIUMGEOMETRY_API AxisTransforms final {

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
};

} // namespace CesiumGeometry
