#pragma once

#include <array>
#include <string>

namespace CesiumGltf {

/**
 * @brief The standard glTF vertex attribute semantics from the specification.
 */
struct VertexAttributeSemantics {
  /**
   * @brief Unitless XYZ vertex positions.
   */
  static const std::string POSITION;

  /**
   * @brief Normalized XYZ vertex normals.
   */
  static const std::string NORMAL;

  /**
   * @brief XYZW vertex tangents where the XYZ portion is normalized, and the W
   * component is a sign value (-1 or +1) indicating handedness of the tangent
   * basis.
   */
  static const std::string TANGENT;

  /**
   * @brief ST texture coordinates
   */
  static const std::array<std::string, 8> TEXCOORD_n;

  /**
   * @brief RGB or RGBA vertex color linear multiplier.
   */
  static const std::array<std::string, 8> COLOR_n;

  /**
   * @brief The indices of the joints from the corresponding skin.joints array
   * that affect the vertex.
   */
  static const std::array<std::string, 8> JOINTS_n;

  /**
   * @brief The weights indicating how strongly the joint influences the vertex.
   */
  static const std::array<std::string, 8> WEIGHTS_n;
};

} // namespace CesiumGltf
