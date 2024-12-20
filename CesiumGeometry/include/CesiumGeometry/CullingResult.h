#pragma once

#include <CesiumGeometry/Library.h>

namespace CesiumGeometry {

/**
 * @brief The result of culling an object.
 */
enum class CESIUMGEOMETRY_API CullingResult {
  /**
   * @brief Indicates that an object lies completely outside the culling volume.
   */
  Outside = -1,

  /**
   * @brief Indicates that an object intersects with the boundary of the culling
   * volume.
   *
   * This means that the object is partially inside and partially outside the
   * culling volume.
   */
  Intersecting = 0,

  /**
   * @brief Indicates that an object lies completely inside the culling volume.
   */
  Inside = 1
};

} // namespace CesiumGeometry
