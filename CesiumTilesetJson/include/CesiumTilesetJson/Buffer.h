#pragma once

#include <CesiumTilesetJson/BufferCesium.h>
#include <CesiumTilesetJson/BufferSpec.h>
#include <CesiumTilesetJson/Library.h>

namespace CesiumTilesetJson {
/** @copydoc BufferSpec */
struct CESIUMTILESETJSON_API Buffer final : public BufferSpec {
  Buffer() = default;

  /**
   * @brief Holds properties that are specific to the 3D Tiles loader rather
   * than part of the 3D Tiles spec.
   */
  BufferCesium cesium;
};
} // namespace CesiumTilesetJson
