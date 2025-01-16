#include <Cesium3DTilesContent/BinaryToGltfConverter.h>
#include <Cesium3DTilesContent/GltfConverterResult.h>
#include <Cesium3DTilesContent/GltfConverters.h>
#include <CesiumAsync/Future.h>
#include <CesiumGeometry/Axis.h>
#include <CesiumGltfReader/GltfReader.h>

#include <cstddef>
#include <span>
#include <type_traits>
#include <utility>

namespace Cesium3DTilesContent {
CesiumGltfReader::GltfReader BinaryToGltfConverter::_gltfReader;

GltfConverterResult BinaryToGltfConverter::convertImmediate(
    const std::span<const std::byte>& gltfBinary,
    const CesiumGltfReader::GltfReaderOptions& options,
    const AssetFetcher& assetFetcher) {
  CesiumGltfReader::GltfReaderResult loadedGltf =
      _gltfReader.readGltf(gltfBinary, options);

  if (loadedGltf.model) {
    loadedGltf.model->extras["gltfUpAxis"] =
        static_cast<std::underlying_type_t<CesiumGeometry::Axis>>(
            assetFetcher.upAxis);
  }
  GltfConverterResult result;
  result.model = std::move(loadedGltf.model);
  result.errors.errors = std::move(loadedGltf.errors);
  result.errors.warnings = std::move(loadedGltf.warnings);
  return result;
}

CesiumAsync::Future<GltfConverterResult> BinaryToGltfConverter::convert(
    const std::span<const std::byte>& gltfBinary,
    const CesiumGltfReader::GltfReaderOptions& options,
    const AssetFetcher& assetFetcher) {
  return assetFetcher.asyncSystem.createResolvedFuture(
      convertImmediate(gltfBinary, options, assetFetcher));
}
} // namespace Cesium3DTilesContent
