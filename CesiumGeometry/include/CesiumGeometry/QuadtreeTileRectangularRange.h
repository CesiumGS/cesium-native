#pragma once

#include <cstdint>

namespace CesiumGeometry {

/**
 * @brief A rectangular range of tiles at a particular level of a quadtree.
 */
struct QuadtreeTileRectangularRange {

  /**
   * @brief The level in the tree at which this rectangle is located, with 0
   * being the root.
   */
  uint32_t level;

  /**
   * @brief The minimum x-coordinate of the range, *inclusive*.
   */
  uint32_t minimumX;

  /**
   * @brief The minimum y-coordinate of the range, *inclusive*.
   */
  uint32_t minimumY;

  /**
   * @brief The maximum x-coordinate of the range, *inclusive*.
   */
  uint32_t maximumX;

  /**
   * @brief The maximum y-coordinate of the range, *inclusive*.
   */
  uint32_t maximumY;
};

} // namespace CesiumGeometry
