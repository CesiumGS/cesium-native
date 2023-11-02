#pragma once

#include "GltfConverterResult.h"

#include <CesiumGltf/Model.h>
#include <CesiumGltfReader/GltfReader.h>

#include <gsl/span>

#include <optional>

namespace Cesium3DTilesContent {
struct PntsToGltfConverter {
  static GltfConverterResult convert(
      const gsl::span<const std::byte>& pntsBinary,
      const CesiumGltfReader::GltfReaderOptions& options);
};
} // namespace Cesium3DTilesContent
