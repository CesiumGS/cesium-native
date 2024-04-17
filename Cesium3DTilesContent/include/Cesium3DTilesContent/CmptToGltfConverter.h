#pragma once

#include <Cesium3DTilesContent/GltfConverterResult.h>
#include <CesiumAsync/Future.h>
#include <CesiumGltfReader/GltfReader.h>

#include <gsl/span>

#include <cstddef>

namespace Cesium3DTilesContent {
struct ConverterSubprocessor;

struct CmptToGltfConverter {
  static CesiumAsync::Future<GltfConverterResult> convert(
      const gsl::span<const std::byte>& cmptBinary,
      const CesiumGltfReader::GltfReaderOptions& options,
      ConverterSubprocessor*);
};
} // namespace Cesium3DTilesContent
