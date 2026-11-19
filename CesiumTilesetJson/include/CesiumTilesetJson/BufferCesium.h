#pragma once

#include <CesiumTilesetJson/Library.h>

#include <cstddef>
#include <vector>

namespace CesiumTilesetJson {
/**
 * @brief Holds @ref Buffer properties that are specific to the 3D Tiles loader
 * rather than part of the 3D Tiles spec.
 */
struct CESIUMTILESETJSON_API BufferCesium final {
  /**
   * @brief The buffer's data.
   */
  std::vector<std::byte> data;
};
} // namespace CesiumTilesetJson
