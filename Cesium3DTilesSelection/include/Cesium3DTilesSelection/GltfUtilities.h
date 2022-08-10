#pragma once

#include <Cesium3DTilesSelection/CreditSystem.h>
#include <Cesium3DTilesSelection/RasterOverlayDetails.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGltf/Model.h>

#include <glm/glm.hpp>

#include <vector>

namespace Cesium3DTilesSelection {
struct GltfUtilities {
  static glm::dmat4x4 applyRtcCenter(
      const CesiumGltf::Model& gltf,
      const glm::dmat4x4& rootTransform);

  static glm::dmat4x4 applyGltfUpAxisTransform(
      const CesiumGltf::Model& model,
      const glm::dmat4x4& rootTransform);

  static std::optional<RasterOverlayDetails>
  createRasterOverlayTextureCoordinates(
      CesiumGltf::Model& gltf,
      const glm::dmat4& modelToEcefTransform,
      int32_t firstTextureCoordinateID,
      const std::optional<CesiumGeospatial::GlobeRectangle>& globeRectangle,
      std::vector<CesiumGeospatial::Projection>&& projections);

  static CesiumGeospatial::BoundingRegion computeBoundingRegion(
      const CesiumGltf::Model& gltf,
      const glm::dmat4& transform);

  static std::vector<Credit> parseGltfCopyright(
      CreditSystem& creditSystems,
      const CesiumGltf::Model& gltf,
      bool showOnScreen);
};
} // namespace Cesium3DTilesSelection
