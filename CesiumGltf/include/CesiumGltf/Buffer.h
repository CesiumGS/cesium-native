#pragma once

#include <CesiumGltf/BufferCesium.h>
#include <CesiumGltf/BufferSpec.h>
#include <CesiumGltf/Library.h>

namespace CesiumGltf {
/** @copydoc BufferSpec */
struct CESIUMGLTF_API Buffer final : public BufferSpec {
  Buffer() = default;

  /**
   * @brief Holds properties that are specific to the glTF loader rather than
   * part of the glTF spec.
   */
  BufferCesium cesium;
};
} // namespace CesiumGltf
