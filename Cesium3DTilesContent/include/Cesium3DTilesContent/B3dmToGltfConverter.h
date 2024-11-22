#pragma once

#include "GltfConverterResult.h"

#include <CesiumAsync/Future.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltfReader/GltfReader.h>

#include <optional>
#include <span>

namespace Cesium3DTilesContent {
struct AssetFetcher;

struct B3dmToGltfConverter {
  static CesiumAsync::Future<GltfConverterResult> convert(
      const std::span<const std::byte>& b3dmBinary,
      const CesiumGltfReader::GltfReaderOptions& options,
      const AssetFetcher& assetFetcher);
};
} // namespace Cesium3DTilesContent
