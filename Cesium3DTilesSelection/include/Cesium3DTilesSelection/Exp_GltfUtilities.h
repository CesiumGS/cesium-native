#pragma once

#include <CesiumGltf/Model.h>
#include <glm/glm.hpp>

namespace Cesium3DTilesSelection {
struct GltfUtilities {
  static glm::dmat4x4 applyRtcCenter(
      const CesiumGltf::Model& gltf,
      const glm::dmat4x4& rootTransform);
};
} // namespace Cesium3DTilesSelection
