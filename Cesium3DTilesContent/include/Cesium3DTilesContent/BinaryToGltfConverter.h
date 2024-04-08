#pragma once

#include <Cesium3DTilesContent/GltfConverterResult.h>
#include <CesiumGltfReader/GltfReader.h>

#include <gsl/span>

#include <cstddef>

namespace Cesium3DTilesContent {
struct ConverterSubprocessor;

struct BinaryToGltfConverter {
public:
  static GltfConverterResult convert(
      const gsl::span<const std::byte>& gltfBinary,
      const CesiumGltfReader::GltfReaderOptions& options,
      ConverterSubprocessor* subProcessor);

private:
  static CesiumGltfReader::GltfReader _gltfReader;
};
} // namespace Cesium3DTilesContent
