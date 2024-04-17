#pragma once

#include "GltfConverterResult.h"

#include <CesiumAsync/Future.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltfReader/GltfReader.h>

#include <gsl/span>

#include <optional>

namespace Cesium3DTilesContent {
struct ConverterSubprocessor;

struct B3dmToGltfConverter {
  static CesiumAsync::Future<GltfConverterResult> convert(
      const gsl::span<const std::byte>& b3dmBinary,
      const CesiumGltfReader::GltfReaderOptions& options,
      ConverterSubprocessor* subprocessor);
};
} // namespace Cesium3DTilesContent
