#pragma once

#include <Cesium3DTilesSelection/Exp_GltfConverterResult.h>

#include <gsl/span>

#include <cstddef>

namespace Cesium3DTilesSelection {
struct CmptToGltfConverter {
  static GltfConverterResult
  convert(const gsl::span<const std::byte>& cmptBinary);
};
} // namespace Cesium3DTilesSelection
