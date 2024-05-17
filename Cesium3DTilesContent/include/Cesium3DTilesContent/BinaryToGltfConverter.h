#pragma once

#include <Cesium3DTilesContent/GltfConverterResult.h>
#include <CesiumAsync/Future.h>
#include <CesiumGltfReader/GltfReader.h>

#include <gsl/span>

#include <cstddef>

namespace Cesium3DTilesContent {
struct AssetFetcher;

struct BinaryToGltfConverter {
public:
  static CesiumAsync::Future<GltfConverterResult> convert(
      const gsl::span<const std::byte>& gltfBinary,
      const CesiumGltfReader::GltfReaderOptions& options,
      const AssetFetcher& assetFetcher);

private:
  static GltfConverterResult convertImmediate(
      const gsl::span<const std::byte>& gltfBinary,
      const CesiumGltfReader::GltfReaderOptions& options);
  static CesiumGltfReader::GltfReader _gltfReader;
};
} // namespace Cesium3DTilesContent
