#pragma once

namespace Cesium3DTilesSelection {

/**
 * @brief Refinement strategies for a {@link Cesium3DTilesSelection::Tile}.
 */
enum class TileRefine {

  /**
   * @brief The content of the child tiles will be added to the content of the
   * parent tile.
   */
  Add = 0,

  /**
   * @brief The content of the child tiles will replace the content of the
   * parent tile.
   */
  Replace = 1
};

} // namespace Cesium3DTilesSelection
