#include <Cesium3DTilesSelection/Exp_BinaryToGltfConverter.h>


namespace Cesium3DTilesSelection {
CesiumGltfReader::GltfReader BinaryToGltfConverter::_gltfReader;

GltfConverterResult
BinaryToGltfConverter::convert(const gsl::span<const std::byte>& gltfBinary) {
  CesiumGltfReader::GltfReaderOptions readOptions;
  CesiumGltfReader::GltfReaderResult loadedGltf =
      _gltfReader.readGltf(gltfBinary, readOptions);

  GltfConverterResult result;
  result.model = std::move(loadedGltf.model);
  result.errors.errors = std::move(loadedGltf.errors);
  result.errors.warnings = std::move(loadedGltf.warnings);
  return result;
}
} // namespace Cesium3DTilesSelection
