#include "ConvertTileToGltf.h"

#include <CesiumNativeTests/FileAccessor.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/readFile.h>

namespace Cesium3DTilesContent {

CesiumAsync::AsyncSystem ConvertTileToGltf::asyncSystem(
    std::make_shared<CesiumNativeTests::SimpleTaskProcessor>());

ConverterSubprocessor
ConvertTileToGltf::makeSubprocessor(const std::string& baseUrl) {
  auto fileAccessor = std::make_shared<CesiumNativeTests::FileAccessor>();
  std::vector<CesiumAsync::IAssetAccessor::THeader> requestHeaders;
  return ConverterSubprocessor(
      asyncSystem,
      fileAccessor,
      baseUrl,
      glm::dmat4(1.0),
      requestHeaders);
}

GltfConverterResult ConvertTileToGltf::fromB3dm(
    const std::filesystem::path& filePath,
    const CesiumGltfReader::GltfReaderOptions& options) {
  ConverterSubprocessor subprocessor = makeSubprocessor("");
  auto bytes = readFile(filePath);
  auto future = B3dmToGltfConverter::convert(bytes, options, subprocessor);
  return future.wait();
}

GltfConverterResult ConvertTileToGltf::fromPnts(
    const std::filesystem::path& filePath,
    const CesiumGltfReader::GltfReaderOptions& options) {
  ConverterSubprocessor subprocessor = makeSubprocessor("");
  auto bytes = readFile(filePath);
  auto future = PntsToGltfConverter::convert(bytes, options, subprocessor);
  return future.wait();
}
} // namespace Cesium3DTilesContent
