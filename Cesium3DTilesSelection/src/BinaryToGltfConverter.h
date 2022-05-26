#pragma once

#include <Cesium3DTilesSelection/GltfConverterResult.h>
#include <CesiumGltfReader/GltfReader.h>

#include <gsl/span>

#include <cstddef>

namespace Cesium3DTilesSelection {
struct BinaryToGltfConverter {
public:
  static GltfConverterResult convert(
      const gsl::span<const std::byte>& gltfBinary,
      const CesiumGltfReader::GltfReaderOptions& options);

private:
  static CesiumGltfReader::GltfReader _gltfReader;
};
} // namespace Cesium3DTilesSelection
