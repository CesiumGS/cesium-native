#pragma once

#include <array>
#include <string>

namespace CesiumGltf {

/**
 * @brief The standard glTF instance attribute semantics from
 * `EXT_mesh_gpu_instancing` plus additional instance attribute semantics from
 * extensions.
 */
struct InstanceAttributeSemantics {
  /**
   * @brief XYZ translation vector.
   */
  static const std::string TRANSLATION;

  /**
   * @brief XYZW rotation quaternion.
   */
  static const std::string ROTATION;

  /**
   * @brief XYZ scale vector.
   */
  static const std::string SCALE;

  /**
   * @brief Feature IDs used by `EXT_instance_features`.
   */
  static const std::array<std::string, 8> FEATURE_ID_n;
};

} // namespace CesiumGltf
