#include <CesiumUtility/JsonValue.h>

#include <glm/vec3.hpp>

#include <optional>

namespace CesiumGltfContent {
struct SkirtMeshMetadata {
  SkirtMeshMetadata() noexcept
      : noSkirtIndicesBegin{0},
        noSkirtIndicesCount{0},
        noSkirtVerticesBegin{0},
        noSkirtVerticesCount{0},
        meshCenter{0.0, 0.0, 0.0},
        skirtWestHeight{0.0},
        skirtSouthHeight{0.0},
        skirtEastHeight{0.0},
        skirtNorthHeight{0.0} {}

  static std::optional<SkirtMeshMetadata>
  parseFromGltfExtras(const CesiumUtility::JsonValue::Object& extras);

  static CesiumUtility::JsonValue::Object
  createGltfExtras(const SkirtMeshMetadata& skirt);

  uint32_t noSkirtIndicesBegin;
  uint32_t noSkirtIndicesCount;
  uint32_t noSkirtVerticesBegin;
  uint32_t noSkirtVerticesCount;
  glm::dvec3 meshCenter;
  double skirtWestHeight;
  double skirtSouthHeight;
  double skirtEastHeight;
  double skirtNorthHeight;
};
} // namespace CesiumGltfContent
