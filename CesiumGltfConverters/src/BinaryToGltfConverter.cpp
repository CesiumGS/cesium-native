#include <CesiumAsync/Future.h>
#include <CesiumGeometry/Axis.h>
#include <CesiumGltfConverters/BinaryToGltfConverter.h>
#include <CesiumGltfConverters/GltfConverterResult.h>
#include <CesiumGltfConverters/GltfConverters.h>
#include <CesiumGltfReader/GltfReader.h>

#include <cstddef>
#include <span>
#include <type_traits>
#include <utility>

namespace CesiumGltfConverters {
CesiumGltfReader::GltfReader BinaryToGltfConverter::_gltfReader;

CesiumAsync::Future<GltfConverterResult> BinaryToGltfConverter::convert(
    const std::span<const std::byte>& gltfBinary,
    const CesiumGltfReader::GltfReaderOptions& options,
    const AssetFetcher& assetFetcher) {
  return BinaryToGltfConverter::_gltfReader
      .readGltfAndExternalData(
          gltfBinary,
          assetFetcher.asyncSystem,
          assetFetcher.requestHeaders,
          assetFetcher.pAssetAccessor,
          assetFetcher.baseUrl,
          options)
      .thenInWorkerThread([upAxis = assetFetcher.upAxis](
                              CesiumGltfReader::GltfReaderResult&& gltfResult) {
        if (gltfResult.model) {
          gltfResult.model->extras["gltfUpAxis"] =
              static_cast<std::underlying_type_t<CesiumGeometry::Axis>>(upAxis);
        }
        GltfConverterResult result;
        result.model = std::move(gltfResult.model);
        result.errors.errors = std::move(gltfResult.errors);
        result.errors.warnings = std::move(gltfResult.warnings);
        return result;
      });
}
} // namespace CesiumGltfConverters
