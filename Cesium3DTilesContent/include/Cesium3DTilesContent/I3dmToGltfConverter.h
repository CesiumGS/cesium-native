#pragma once

#include <Cesium3DTilesContent/GltfConverterResult.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltfReader/GltfReader.h>

#include <glm/mat4x4.hpp>
#include <gsl/span>

#include <optional>

namespace Cesium3DTilesContent {
struct ConverterSubprocessor;

struct I3dmToGltfConverter {
  static GltfConverterResult convert(
      const gsl::span<const std::byte>& instancesBinary,
      const CesiumGltfReader::GltfReaderOptions& options,
      ConverterSubprocessor* subprocessor);
};
} // namespace Cesium3DTilesContent
