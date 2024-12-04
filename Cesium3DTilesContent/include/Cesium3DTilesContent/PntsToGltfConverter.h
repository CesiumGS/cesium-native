#pragma once

#include "GltfConverterResult.h"

#include <CesiumAsync/Future.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltfReader/GltfReader.h>

#include <optional>
#include <span>

namespace Cesium3DTilesContent {
struct AssetFetcher;

struct PntsToGltfConverter {
  static CesiumAsync::Future<GltfConverterResult> convert(
      const std::span<const std::byte>& pntsBinary,
      const CesiumGltfReader::GltfReaderOptions& options,
      const AssetFetcher& assetFetcher);
};
} // namespace Cesium3DTilesContent
