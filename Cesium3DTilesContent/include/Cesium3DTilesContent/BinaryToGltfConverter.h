#pragma once

#include <Cesium3DTilesContent/GltfConverterResult.h>
#include <CesiumAsync/Future.h>
#include <CesiumGltfReader/GltfReader.h>

#include <cstddef>
#include <span>

namespace Cesium3DTilesContent {
struct AssetFetcher;

/**
 * @brief Converts a binary glTF model (glb) to a \ref CesiumGltf::Model.
 */
struct BinaryToGltfConverter {
public:
  /**
   * @brief Converts a glb binary file to a glTF model.
   *
   * @param gltfBinary The bytes loaded for the glb model.
   * @param options Options for how the glTF should be loaded.
   * @param assetFetcher The \ref AssetFetcher containing information used by
   * loaded assets.
   * @returns A future that resolves to a \ref GltfConverterResult.
   */
  static CesiumAsync::Future<GltfConverterResult> convert(
      const std::span<const std::byte>& gltfBinary,
      const CesiumGltfReader::GltfReaderOptions& options,
      const AssetFetcher& assetFetcher);

private:
  static GltfConverterResult convertImmediate(
      const std::span<const std::byte>& gltfBinary,
      const CesiumGltfReader::GltfReaderOptions& options,
      const AssetFetcher& assetFetcher);
  static CesiumGltfReader::GltfReader _gltfReader;
};
} // namespace Cesium3DTilesContent
