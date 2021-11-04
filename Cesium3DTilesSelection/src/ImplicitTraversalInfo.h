#pragma once

#include "Cesium3DTilesSelection/Tile.h"
#include "CesiumGeometry/Availability.h"

namespace Cesium3DTilesSelection {

/**
 * @brief Useful implicit tiling information for traversal of the current
 * tile.
 */
struct ImplicitTraversalInfo {

  /**
   * @brief The parent subtree node to the current one.
   *
   * This is only useful when we have not yet loaded the current subtree.
   * Knowing the parent will let us easily add the new subtree once its
   * loaded.
   */
  CesiumGeometry::AvailabilityNode* pParentNode;

  /**
   * @brief The subtree that contains the current tile's availability.
   *
   * If the current tile is the root of a subtree that has not been loaded
   * yet, this will be nullptr.
   */
  CesiumGeometry::AvailabilityNode* pCurrentNode;

  /**
   * @brief The {@link CesiumGeometry::TileAvailabilityFlags} of the current
   * tile.
   */
  uint8_t availability;

  /**
   * @brief Creates empty instance.
   */
  ImplicitTraversalInfo() noexcept;

  /**
   * @brief Attempt to initialize an instance with the given {@link Tile}.
   *
   * @param pTile The tile that may be the implicit root.
   */
  ImplicitTraversalInfo(Tile* pTile) noexcept;
};

} // namespace Cesium3DTilesSelection