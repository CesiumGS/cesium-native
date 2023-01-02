#pragma once

#include "Library.h"

#include <glm/vec3.hpp>

namespace CesiumGeometry {

struct CESIUMGEOMETRY_API AxisAlignedBox final {

  AxisAlignedBox() noexcept
      : minimumX(0.0),
        minimumY(0.0),
        minimumZ(0.0),
        maximumX(0.0),
        maximumY(0.0),
        maximumZ(0.0),
        lengthX(0.0),
        lengthY(0.0),
        lengthZ(0.0),
        center(0.0) {}

  AxisAlignedBox(
      double minimumX_,
      double minimumY_,
      double minimumZ_,
      double maximumX_,
      double maximumY_,
      double maximumZ_) noexcept
      : minimumX(minimumX_),
        minimumY(minimumY_),
        minimumZ(minimumZ_),
        maximumX(maximumX_),
        maximumY(maximumY_),
        maximumZ(maximumZ_),
        lengthX(maximumX - minimumX),
        lengthY(maximumY - minimumY),
        lengthZ(maximumZ - minimumZ),
        center(
            0.5 * (maximumX - minimumX),
            0.5 * (maximumY - minimumY),
            0.5 * (maximumZ - minimumZ)) {}

  /**
   * @brief The minimum x-coordinate.
   */
  double minimumX;

  /**
   * @brief The minimum y-coordinate.
   */
  double minimumY;

  /**
   * @brief The minimum z-coordinate.
   */
  double minimumZ;

  /**
   * @brief The maximum x-coordinate.
   */
  double maximumX;

  /**
   * @brief The maximum y-coordinate.
   */
  double maximumY;

  /**
   * @brief The maximum z-coordinate.
   */
  double maximumZ;

  /**
   * @brief The length of the box on the x-axis.
   */
  double lengthX;

  /**
   * @brief The length of the box on the y-axis.
   */
  double lengthY;

  /**
   * @brief The length of the box on the z-axis.
   */
  double lengthZ;

  /**
   * @brief The center of the box.
   */
  glm::dvec3 center;

  constexpr bool contains(const glm::dvec3& position) const noexcept {
    return position.x >= this->minimumX && position.x <= this->maximumX &&
           position.y >= this->minimumY && position.y <= this->maximumY &&
           position.z >= this->minimumZ && position.z <= this->maximumZ;
  }
};

} // namespace CesiumGeometry
