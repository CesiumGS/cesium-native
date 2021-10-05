#pragma once

#include "BufferCesium.h"
#include "BufferSpec.h"
#include "Library.h"

namespace CesiumGltf {
/** @copydoc BufferSpec */
struct CESIUMGLTF_API Buffer final : public BufferSpec {
  /**
   * @brief Holds properties that are specific to the glTF loader rather than
   * part of the glTF spec.
   */
  BufferCesium cesium;
};
} // namespace CesiumGltf
