#pragma once

#include "Library.h"

#include <cstdint>

namespace CesiumGeometry {

/**
 * @brief A structure serving as a unique identifier for a node in an octree.
 *
 * This is one form of a {@link Cesium3DTilesSelection::TileID}.
 *
 * The identifier is composed of the level (with 0 being the level of the root
 * tile), the x-, y-, and z-coordinate of the tile, referring to a grid
 * coordinate system at the respective level.
 */
struct CESIUMGEOMETRY_API OctreeTileID {

  /**
   * @brief Creates a new instance.
   */
  constexpr OctreeTileID() : level(0), x(0), y(0), z(0){};

  /**
   * @brief Creates a new instance.
   *
   * @param level The level of the node, with 0 being the root.
   * @param x The x-coordinate of the tile.
   * @param y The y-coordinate of the tile.
   * @param z The z-coordinate of the tile.
   */
  constexpr OctreeTileID(
      uint32_t level,
      uint32_t x,
      uint32_t y,
      uint32_t z) noexcept
      : level(level), x(x), y(y), z(z) {}

  /**
   * @brief Returns `true` if two identifiers are equal.
   */
  constexpr bool operator==(const OctreeTileID& other) const noexcept {
    return this->level == other.level && this->x == other.x &&
           this->y == other.y && this->z == other.z;
  }

  /**
   * @brief Returns `true` if two identifiers are *not* equal.
   */
  constexpr bool operator!=(const OctreeTileID& other) const noexcept {
    return !(*this == other);
  }

  /**
   * @brief The level of this tile ID, with 0 being the root tile.
   */
  uint32_t level;

  /**
   * @brief The x-coordinate of this tile ID.
   */
  uint32_t x;

  /**
   * @brief The y-coordinate of this tile ID.
   */
  uint32_t y;

  /**
   * @brief The z-coordinate of this tile ID.
   */
  uint32_t z;
};

} // namespace CesiumGeometry
