#pragma once

#include <Cesium3DTiles/Library.h>

#include <cstddef>
#include <vector>

namespace Cesium3DTiles {
/**
 * @brief Holds {@link Buffer} properties that are specific to the 3D Tiles loader
 * rather than part of the 3D Tiles spec.
 */
struct CESIUM3DTILES_API BufferCesium final {
  /**
   * @brief The buffer's data.
   */
  std::vector<std::byte> data;
};
} // namespace Cesium3DTiles
