#pragma once

#include "Library.h"

namespace Cesium3DTiles {

/**
 * @brief Options for configuring the parsing of a {@link Tileset}'s content
 * and construction of Gltf models.
 */
struct CESIUM3DTILES_API TilesetContentOptions {

  /**
   * @brief Whether to include a water mask within the Gltf extras.
   *
   * Currently only applicable for quantized-mesh tilesets that support the
   * water mask extension.
   */
  bool enableWaterMask = false;
  
};

} // namespace Cesium3DTiles
