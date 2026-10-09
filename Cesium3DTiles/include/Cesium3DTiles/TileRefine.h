#pragma once

#include <CesiumUtility/Result.h>

namespace Cesium3DTiles {

/**
 * @brief Refinement strategies for a @ref Cesium3DTiles::Tile.
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

/**
 * @brief Attempts to parse a @ref TileRefine from the given string.
 */
CesiumUtility::Result<TileRefine> parseTileRefine(const std::string& input);

} // namespace Cesium3DTiles
