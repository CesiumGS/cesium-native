#pragma once

#include <Cesium3DTilesContent/B3dmToGltfConverter.h>
#include <Cesium3DTilesContent/PntsToGltfConverter.h>
#include <CesiumNativeTests/readFile.h>

#include <filesystem>

namespace Cesium3DTilesContent {

class ConvertTileToGltf {
public:
  static GltfConverterResult fromB3dm(const std::filesystem::path& filePath) {
    return B3dmToGltfConverter::convert(readFile(filePath), {}, nullptr);
  }

  static GltfConverterResult fromPnts(const std::filesystem::path& filePath) {
    return PntsToGltfConverter::convert(readFile(filePath), {}, nullptr);
  }
};
} // namespace Cesium3DTilesContent
