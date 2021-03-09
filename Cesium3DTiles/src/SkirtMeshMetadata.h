#include "CesiumGltf/JsonValue.h"
#include <glm/vec3.hpp>
#include <optional>

namespace Cesium3DTiles {
struct SkirtMeshMetadata {
  SkirtMeshMetadata()
      : noSkirtIndicesBegin{0},
        noSkirtIndicesCount{0},
        meshCenter{0.0, 0.0, 0.0},
        skirtWestHeight{0.0},
        skirtSouthHeight{0.0},
        skirtEastHeight{0.0},
        skirtNorthHeight{0.0} {}

  static std::optional<SkirtMeshMetadata>
  parseFromGltfExtras(const CesiumGltf::JsonValue::Object& extras);

  static CesiumGltf::JsonValue::Object
  createGltfExtras(const SkirtMeshMetadata& skirt);

  uint32_t noSkirtIndicesBegin;
  uint32_t noSkirtIndicesCount;
  glm::dvec3 meshCenter;
  double skirtWestHeight;
  double skirtSouthHeight;
  double skirtEastHeight;
  double skirtNorthHeight;
};
} // namespace Cesium3DTiles