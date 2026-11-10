#pragma once

#include <Cesium3DTiles/Library.h>
#include <CesiumUtility/Result.h>

namespace Cesium3DTiles {

/**
 * @brief Refinement strategies for a tile in 3D Tiles.
 */
enum class CESIUM3DTILES_API TileRefine {

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
 *
 * @returns The resulting @ref TileRefine, or a list of errors if parsing was
 * unsuccessful.
 */
CESIUM3DTILES_API CesiumUtility::Result<TileRefine>
getTileRefine(const std::string& input);

} // namespace Cesium3DTiles
