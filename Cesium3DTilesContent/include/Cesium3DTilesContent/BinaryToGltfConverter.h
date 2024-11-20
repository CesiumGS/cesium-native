#pragma once

#include <Cesium3DTilesContent/GltfConverterResult.h>
#include <CesiumAsync/Future.h>
#include <CesiumGltfReader/GltfReader.h>

#include <cstddef>
#include <span>

namespace Cesium3DTilesContent {
struct AssetFetcher;

struct BinaryToGltfConverter {
public:
  static CesiumAsync::Future<GltfConverterResult> convert(
      const std::span<const std::byte>& gltfBinary,
      const CesiumGltfReader::GltfReaderOptions& options,
      const AssetFetcher& assetFetcher);

private:
  static GltfConverterResult convertImmediate(
      const std::span<const std::byte>& gltfBinary,
      const CesiumGltfReader::GltfReaderOptions& options,
      const AssetFetcher& assetFetcher);
  static CesiumGltfReader::GltfReader _gltfReader;
};
} // namespace Cesium3DTilesContent
