#pragma once

#include <Cesium3DTilesSelection/GltfConverterResult.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltfReader/GltfReader.h>

#include <gsl/span>

#include <optional>

namespace Cesium3DTilesSelection {
struct B3dmToGltfConverter {
  static GltfConverterResult convert(
      const gsl::span<const std::byte>& b3dmBinary,
      const CesiumGltfReader::GltfReaderOptions& options);
};
} // namespace Cesium3DTilesSelection
