#pragma once

#include <Cesium3DTilesSelection/GltfConverterResult.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltfReader/GltfReader.h>

#include <gsl/span>

#include <optional>

namespace Cesium3DTilesSelection {
struct PntsToGltfConverter {
  static GltfConverterResult convert(
      const gsl::span<const std::byte>& pntsBinary,
      const CesiumGltfReader::GltfReaderOptions& options);
};
} // namespace Cesium3DTilesSelection
