#include "ConvertToGltf.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeometry/Axis.h>
#include <CesiumGltfConverters/B3dmToGltfConverter.h>
#include <CesiumGltfConverters/GltfConverterResult.h>
#include <CesiumGltfConverters/I3dmToGltfConverter.h>
#include <CesiumGltfConverters/PntsToGltfConverter.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumNativeTests/FileAccessor.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/readFile.h>

#include <glm/ext/matrix_double4x4.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace CesiumGltfConverters {

CesiumAsync::AsyncSystem ConvertToGltf::asyncSystem(
    std::make_shared<CesiumNativeTests::SimpleTaskProcessor>());

AssetFetcher ConvertToGltf::makeAssetFetcher(const std::string& baseUrl) {
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

GltfConverterResult ConvertToGltf::fromB3dm(
    const std::filesystem::path& filePath,
    const CesiumGltfReader::GltfReaderOptions& options) {
  AssetFetcher assetFetcher = makeAssetFetcher("");
  auto bytes = readFile(filePath);
  auto future = B3dmToGltfConverter::convert(bytes, options, assetFetcher);
  return future.wait();
}

GltfConverterResult ConvertToGltf::fromPnts(
    const std::filesystem::path& filePath,
    const CesiumGltfReader::GltfReaderOptions& options) {
  AssetFetcher assetFetcher = makeAssetFetcher("");
  auto bytes = readFile(filePath);
  auto future = PntsToGltfConverter::convert(bytes, options, assetFetcher);
  return future.wait();
}

GltfConverterResult ConvertToGltf::fromI3dm(
    const std::filesystem::path& filePath,
    const CesiumGltfReader::GltfReaderOptions& options) {
  AssetFetcher assetFetcher = makeAssetFetcher("");
  auto bytes = readFile(filePath);
  auto future = I3dmToGltfConverter::convert(bytes, options, assetFetcher);
  return future.wait();
}

} // namespace CesiumGltfConverters
