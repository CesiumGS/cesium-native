#pragma once

#include <Cesium3DTilesSelection/ErrorList.h>
#include <CesiumGltf/Model.h>

#include <gsl/span>
#include <rapidjson/document.h>

#include <cstddef>

namespace Cesium3DTilesSelection {
struct BatchTableToGltfFeatureMetadata {
  static ErrorList convertFromB3dm(
      const rapidjson::Document& featureTableJson,
      const rapidjson::Document& batchTableJson,
      const gsl::span<const std::byte>& batchTableBinaryData,
      CesiumGltf::Model& gltf);

  static ErrorList convertFromPnts(
      const rapidjson::Document& featureTableJson,
      const rapidjson::Document& batchTableJson,
      const gsl::span<const std::byte>& batchTableBinaryData,
      CesiumGltf::Model& gltf);
};
} // namespace Cesium3DTilesSelection
