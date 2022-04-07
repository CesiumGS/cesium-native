#pragma once

#include <Cesium3DTilesSelection/Exp_ErrorList.h>
#include <CesiumGltf/Model.h>
#include <rapidjson/document.h>
#include <GSL/span>
#include <cstddef>

namespace Cesium3DTilesSelection {
struct BatchTableToGltfFeatureMetadata {
  static ErrorList convert(
      const rapidjson::Document& featureTableJson,
      const rapidjson::Document& batchTableJson,
      const gsl::span<const std::byte>& batchTableBinaryData,
      CesiumGltf::Model& gltf);
};
} // namespace Cesium3DTilesSelection
