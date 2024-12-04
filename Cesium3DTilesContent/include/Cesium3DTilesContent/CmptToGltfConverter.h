#pragma once

#include <Cesium3DTilesContent/GltfConverterResult.h>
#include <CesiumAsync/Future.h>
#include <CesiumGltfReader/GltfReader.h>

#include <cstddef>
#include <span>

namespace Cesium3DTilesContent {
struct AssetFetcher;

struct CmptToGltfConverter {
  static CesiumAsync::Future<GltfConverterResult> convert(
      const std::span<const std::byte>& cmptBinary,
      const CesiumGltfReader::GltfReaderOptions& options,
      const AssetFetcher& assetFetcher);
};
} // namespace Cesium3DTilesContent
