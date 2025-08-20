#include <Cesium3DTilesSelection/GltfModifierVersionExtension.h>
#include <CesiumGltf/Model.h>

namespace Cesium3DTilesSelection {

/* static */ std::optional<int32_t> GltfModifierVersionExtension::getVersion(
    const CesiumGltf::Model& model) noexcept {
  const auto* pExtension = model.getExtension<GltfModifierVersionExtension>();
  return pExtension ? std::make_optional(pExtension->version) : std::nullopt;
}

/* static */ void GltfModifierVersionExtension::setVersion(
    CesiumGltf::Model& model,
    int32_t version) noexcept {
  auto* pExtension = model.getExtension<GltfModifierVersionExtension>();
  if (!pExtension) {
    pExtension = &model.addExtension<GltfModifierVersionExtension>();
  }
  pExtension->version = version;
}

} // namespace Cesium3DTilesSelection
