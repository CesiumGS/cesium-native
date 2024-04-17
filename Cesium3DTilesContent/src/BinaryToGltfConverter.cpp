#include <Cesium3DTilesContent/BinaryToGltfConverter.h>
#include <Cesium3DTilesContent/GltfConverters.h>

namespace Cesium3DTilesContent {
CesiumGltfReader::GltfReader BinaryToGltfConverter::_gltfReader;

GltfConverterResult BinaryToGltfConverter::convertImmediate(
    const gsl::span<const std::byte>& gltfBinary,
    const CesiumGltfReader::GltfReaderOptions& options,
    ConverterSubprocessor*) {
  CesiumGltfReader::GltfReaderResult loadedGltf =
      _gltfReader.readGltf(gltfBinary, options);

  GltfConverterResult result;
  result.model = std::move(loadedGltf.model);
  result.errors.errors = std::move(loadedGltf.errors);
  result.errors.warnings = std::move(loadedGltf.warnings);
  return result;
}

CesiumAsync::Future<GltfConverterResult> BinaryToGltfConverter::convert(
    const gsl::span<const std::byte>& gltfBinary,
    const CesiumGltfReader::GltfReaderOptions& options,
    ConverterSubprocessor* subprocessor) {
  return subprocessor->asyncSystem.createResolvedFuture(
      convertImmediate(gltfBinary, options, subprocessor));
}
} // namespace Cesium3DTilesContent
