#pragma once

#include "B3dmToGltfConverter.h"
#include "PntsToGltfConverter.h"
#include "readFile.h"

#include <filesystem>

namespace Cesium3DTilesSelection {

class ConvertTileToGltf {
public:
  static GltfConverterResult fromB3dm(const std::filesystem::path& filePath) {
    return B3dmToGltfConverter::convert(readFile(filePath), {});
  }

  static GltfConverterResult fromPnts(const std::filesystem::path& filePath) {
    return PntsToGltfConverter::convert(readFile(filePath), {});
  }
};
} // namespace Cesium3DTilesSelection
