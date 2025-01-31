#include <CesiumGltf/VertexAttributeSemantics.h>

#include <array>
#include <string>

namespace CesiumGltf {

/**
 * @brief Unitless XYZ vertex positions.
 */
const std::string VertexAttributeSemantics::POSITION = "POSITION";

/**
 * @brief Normalized XYZ vertex normals.
 */
const std::string VertexAttributeSemantics::NORMAL = "NORMAL";

/**
 * @brief XYZW vertex tangents where the XYZ portion is normalized, and the W
 * component is a sign value (-1 or +1) indicating handedness of the tangent
 * basis.
 */
const std::string VertexAttributeSemantics::TANGENT = "TANGENT";

/**
 * @brief ST texture coordinates
 */
const std::array<std::string, 8> VertexAttributeSemantics::TEXCOORD_n = {
    "TEXCOORD_0",
    "TEXCOORD_1",
    "TEXCOORD_2",
    "TEXCOORD_3",
    "TEXCOORD_4",
    "TEXCOORD_5",
    "TEXCOORD_6",
    "TEXCOORD_7"};

/**
 * @brief RGB or RGBA vertex color linear multiplier.
 */
const std::array<std::string, 8> VertexAttributeSemantics::COLOR_n = {
    "COLOR_0",
    "COLOR_1",
    "COLOR_2",
    "COLOR_3",
    "COLOR_4",
    "COLOR_5",
    "COLOR_6",
    "COLOR_7"};

/**
 * @brief The indices of the joints from the corresponding skin.joints array
 * that affect the vertex.
 */
const std::array<std::string, 8> VertexAttributeSemantics::JOINTS_n = {
    "JOINTS_0",
    "JOINTS_1",
    "JOINTS_2",
    "JOINTS_3",
    "JOINTS_4",
    "JOINTS_5",
    "JOINTS_6",
    "JOINTS_7"};

/**
 * @brief The weights indicating how strongly the joint influences the vertex.
 */
const std::array<std::string, 8> VertexAttributeSemantics::WEIGHTS_n = {
    "WEIGHTS_0",
    "WEIGHTS_1",
    "WEIGHTS_2",
    "WEIGHTS_3",
    "WEIGHTS_4",
    "WEIGHTS_5",
    "WEIGHTS_6",
    "WEIGHTS_7"};

/**
 * @brief Feature IDs used in `EXT_mesh_features`.
 */
const std::array<std::string, 8> VertexAttributeSemantics::FEATURE_ID_n = {
    "_FEATURE_ID_0",
    "_FEATURE_ID_1",
    "_FEATURE_ID_2",
    "_FEATURE_ID_3",
    "_FEATURE_ID_4",
    "_FEATURE_ID_5",
    "_FEATURE_ID_6",
    "_FEATURE_ID_7"};

} // namespace CesiumGltf
