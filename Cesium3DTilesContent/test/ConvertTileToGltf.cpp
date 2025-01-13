#include "ConvertTileToGltf.h"

#include <Cesium3DTilesContent/B3dmToGltfConverter.h>
#include <Cesium3DTilesContent/GltfConverterResult.h>
#include <Cesium3DTilesContent/I3dmToGltfConverter.h>
#include <Cesium3DTilesContent/PntsToGltfConverter.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeometry/Axis.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumNativeTests/FileAccessor.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/readFile.h>

#include <glm/ext/matrix_double4x4.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace Cesium3DTilesContent {

CesiumAsync::AsyncSystem ConvertTileToGltf::asyncSystem(
    std::make_shared<CesiumNativeTests::SimpleTaskProcessor>());

AssetFetcher ConvertTileToGltf::makeAssetFetcher(const std::string& baseUrl) {
  auto fileAccessor = std::make_shared<CesiumNativeTests::FileAccessor>();
  std::vector<CesiumAsync::IAssetAccessor::THeader> requestHeaders;
  return AssetFetcher(
      asyncSystem,
      fileAccessor,
      baseUrl,
      glm::dmat4(1.0),
      requestHeaders,
      CesiumGeometry::Axis::Y);
}

GltfConverterResult ConvertTileToGltf::fromB3dm(
    const std::filesystem::path& filePath,
    const CesiumGltfReader::GltfReaderOptions& options) {
  AssetFetcher assetFetcher = makeAssetFetcher("");
  auto bytes = readFile(filePath);
  auto future = B3dmToGltfConverter::convert(bytes, options, assetFetcher);
  return future.wait();
}

GltfConverterResult ConvertTileToGltf::fromPnts(
    const std::filesystem::path& filePath,
    const CesiumGltfReader::GltfReaderOptions& options) {
  AssetFetcher assetFetcher = makeAssetFetcher("");
  auto bytes = readFile(filePath);
  auto future = PntsToGltfConverter::convert(bytes, options, assetFetcher);
  return future.wait();
}

GltfConverterResult ConvertTileToGltf::fromI3dm(
    const std::filesystem::path& filePath,
    const CesiumGltfReader::GltfReaderOptions& options) {
  AssetFetcher assetFetcher = makeAssetFetcher("");
  auto bytes = readFile(filePath);
  auto future = I3dmToGltfConverter::convert(bytes, options, assetFetcher);
  return future.wait();
}

} // namespace Cesium3DTilesContent
