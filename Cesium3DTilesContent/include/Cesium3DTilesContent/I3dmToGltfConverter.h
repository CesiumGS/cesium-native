#pragma once

#include <Cesium3DTilesContent/GltfConverterResult.h>
#include <CesiumAsync/Future.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltfReader/GltfReader.h>

#include <glm/mat4x4.hpp>
#include <gsl/span>

#include <optional>

namespace Cesium3DTilesContent {
struct AssetFetcher;

struct I3dmToGltfConverter {
  static CesiumAsync::Future<GltfConverterResult> convert(
      const gsl::span<const std::byte>& instancesBinary,
      const CesiumGltfReader::GltfReaderOptions& options,
      const AssetFetcher& assetFetcher);
};
} // namespace Cesium3DTilesContent
