#pragma once

#include <CesiumGeometry/Library.h>

#include <cstdint>

namespace CesiumGeometry {

/**
 * @brief A bitmask representing the availability state of a tile.
 */
enum CESIUMGEOMETRY_API TileAvailabilityFlags {
  /**
   * @brief The tile is known to be available.
   */
  TILE_AVAILABLE = 1U,

  /**
   * @brief The tile's content is known to be available.
   */
  CONTENT_AVAILABLE = 2U,

  /**
   * @brief This tile has a subtree that is known to be available.
   */
  SUBTREE_AVAILABLE = 4U,

  /**
   * @brief This tile has a subtree that is loaded.
   */
  SUBTREE_LOADED = 8U,

  // TODO: is REACHABLE needed? Reevaluate after implementation
  /**
   * @brief The tile is reachable through the tileset availability tree.
   *
   * If a tile is not reachable, the above flags being false may simply
   * indicate that a subtree needed to reach this tile has not yet been loaded.
   */
  REACHABLE = 16U
};

} // namespace CesiumGeometry
