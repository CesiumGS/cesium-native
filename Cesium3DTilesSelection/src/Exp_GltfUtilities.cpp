#include <Cesium3DTilesSelection/Exp_GltfUtilities.h>
#include <CesiumGltf/ExtensionCesiumRTC.h>

namespace Cesium3DTilesSelection {
/*static*/ glm::dmat4x4 GltfUtilities::applyRtcCenter(
    const CesiumGltf::Model& gltf,
    const glm::dmat4x4& rootTransform) {
  const CesiumGltf::ExtensionCesiumRTC* cesiumRTC = gltf.getExtension<CesiumGltf::ExtensionCesiumRTC>();
  if (cesiumRTC == nullptr) {
    return rootTransform;
  }
  const std::vector<double>& rtcCenter = cesiumRTC->center;
  if (rtcCenter.size() != 3) {
    return rootTransform;
  }
  const double x = rtcCenter[0];
  const double y = rtcCenter[1];
  const double z = rtcCenter[2];
  const glm::dmat4x4 rtcTransform(
      glm::dvec4(1.0, 0.0, 0.0, 0.0),
      glm::dvec4(0.0, 1.0, 0.0, 0.0),
      glm::dvec4(0.0, 0.0, 1.0, 0.0),
      glm::dvec4(x, y, z, 1.0));
  return rootTransform * rtcTransform;
}
} // namespace Cesium3DTilesSelection
