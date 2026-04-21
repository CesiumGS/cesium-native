#pragma once

#include <CesiumGltf/ExtensionExtPrimitiveVoxelsSpec.h>

namespace CesiumGltf {

/** @copydoc ExtensionExtPrimitiveVoxelsSpec */
struct CESIUMGLTF_API ExtensionExtPrimitiveVoxels final
    : public ExtensionExtPrimitiveVoxelsSpec {
  /**
   * @brief The constant used to indicate a voxel primitive in \ref
   * CesiumGltf::MeshPrimitive::mode.
   */
  static constexpr int32_t MODE = 2147483647;

  ExtensionExtPrimitiveVoxels() = default;
};

} // namespace CesiumGltf
