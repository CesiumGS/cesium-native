#pragma once

#include <Cesium3DTiles/BufferCesium.h>
#include <Cesium3DTiles/BufferSpec.h>
#include <Cesium3DTiles/Library.h>

namespace Cesium3DTiles {
/** @copydoc BufferSpec */
struct CESIUM3DTILES_API Buffer final : public BufferSpec {
  Buffer() = default;

  /**
   * @brief Holds properties that are specific to the 3D Tiles loader rather
   * than part of the 3D Tiles spec.
   */
  BufferCesium cesium;
};
} // namespace Cesium3DTiles
