#pragma once

#include <Cesium3DTilesSelection/GltfConverterResult.h>
#include <CesiumGltfReader/GltfReader.h>

#include <gsl/span>

#include <cstddef>

namespace Cesium3DTilesSelection {
struct CmptToGltfConverter {
  static GltfConverterResult convert(
      const gsl::span<const std::byte>& cmptBinary,
      const CesiumGltfReader::GltfReaderOptions& options);
};
} // namespace Cesium3DTilesSelection
