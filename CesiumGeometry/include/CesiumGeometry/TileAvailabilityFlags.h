#pragma once

#include "Library.h"

#include <cstdint>

namespace CesiumGeometry {

class CESIUMGEOMETRY_API TileAvailabilityFlags final {
public:
  /**
   * @brief The tile is known to be available.
   */
  static const uint8_t TILE_AVAILABLE = 1;

  /**
   * @brief The tile's content is known to be available.
   */
  static const uint8_t CONTENT_AVAILABLE = 2;

  /**
   * @brief This tile has a subtree that is known to be available.
   */
  static const uint8_t SUBTREE_AVAILABLE = 4;

  /**
   * @brief This tile has a subtree that is loaded.
   */
  static const uint8_t SUBTREE_LOADED = 8;

  // TODO: is REACHABLE needed? Reevaluate after implementation
  /**
   * @brief The tile is reachable through the tileset availability tree.
   *
   * If a tile is not reachable, the above flags being false may simply
   * indicate that a subtree needed to reach this tile has not yet been loaded.
   */
  static const uint8_t REACHABLE = 16;
};

} // namespace CesiumGeometry