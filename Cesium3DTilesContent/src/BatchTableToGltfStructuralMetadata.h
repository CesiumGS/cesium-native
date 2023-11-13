#pragma once

#include <CesiumGltf/Model.h>
#include <CesiumUtility/ErrorList.h>

#include <gsl/span>
#include <rapidjson/document.h>

#include <cstddef>

namespace Cesium3DTilesContent {
struct BatchTableToGltfStructuralMetadata {
  static CesiumUtility::ErrorList convertFromB3dm(
      const rapidjson::Document& featureTableJson,
      const rapidjson::Document& batchTableJson,
      const gsl::span<const std::byte>& batchTableBinaryData,
      CesiumGltf::Model& gltf);

  static CesiumUtility::ErrorList convertFromPnts(
      const rapidjson::Document& featureTableJson,
      const rapidjson::Document& batchTableJson,
      const gsl::span<const std::byte>& batchTableBinaryData,
      CesiumGltf::Model& gltf);
};
} // namespace Cesium3DTilesContent
