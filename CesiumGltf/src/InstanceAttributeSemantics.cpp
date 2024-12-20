#include <CesiumGltf/InstanceAttributeSemantics.h>

#include <array>
#include <string>

namespace CesiumGltf {

/**
 * @brief XYZ translation vector.
 */
const std::string InstanceAttributeSemantics::TRANSLATION = "TRANSLATION";

/**
 * @brief XYZW rotation quaternion.
 */
const std::string InstanceAttributeSemantics::ROTATION = "ROTATION";

/**
 * @brief XYZ scale vector.
 */
const std::string InstanceAttributeSemantics::SCALE = "SCALE";

/**
 * @brief Feature IDs used in `EXT_mesh_features`.
 */
const std::array<std::string, 8> InstanceAttributeSemantics::FEATURE_ID_n = {
    "_FEATURE_ID_0",
    "_FEATURE_ID_1",
    "_FEATURE_ID_2",
    "_FEATURE_ID_3",
    "_FEATURE_ID_4",
    "_FEATURE_ID_5",
    "_FEATURE_ID_6",
    "_FEATURE_ID_7"};

} // namespace CesiumGltf
