#pragma once

#include <Cesium3DTilesContent/GltfConverters.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumNativeTests/readFile.h>

#include <filesystem>

namespace Cesium3DTilesContent {

class ConvertTileToGltf {
public:
  static GltfConverterResult fromB3dm(
      const std::filesystem::path& filePath,
      const CesiumGltfReader::GltfReaderOptions& options = {});
  static GltfConverterResult fromPnts(
      const std::filesystem::path& filePath,
      const CesiumGltfReader::GltfReaderOptions& options = {});
  static GltfConverterResult fromI3dm(
      const std::filesystem::path& filePath,
      const CesiumGltfReader::GltfReaderOptions& options = {});

private:
  static CesiumAsync::AsyncSystem asyncSystem;
  static AssetFetcher makeAssetFetcher(const std::string& baseUrl);
};
} // namespace Cesium3DTilesContent
