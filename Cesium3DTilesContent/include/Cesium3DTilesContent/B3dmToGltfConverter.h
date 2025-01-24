#pragma once

#include <Cesium3DTilesContent/GltfConverterResult.h>
#include <CesiumAsync/Future.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltfReader/GltfReader.h>

#include <optional>
#include <span>

namespace Cesium3DTilesContent {
struct AssetFetcher;

/**
 * @brief Converts a b3dm (Batched 3D Model) file to a glTF.
 *
 * For more information on the b3dm format, see
 * https://github.com/CesiumGS/3d-tiles/blob/main/specification/TileFormats/Batched3DModel/README.adoc
 */
struct B3dmToGltfConverter {
  /**
   * @brief Converts a b3dm binary file to a glTF model.
   *
   * @param b3dmBinary The bytes loaded for the b3dm model.
   * @param options Options for how the glTF should be loaded.
   * @param assetFetcher The \ref AssetFetcher containing information used by
   * loaded assets.
   * @returns A future that resolves to a \ref GltfConverterResult.
   */
  static CesiumAsync::Future<GltfConverterResult> convert(
      const std::span<const std::byte>& b3dmBinary,
      const CesiumGltfReader::GltfReaderOptions& options,
      const AssetFetcher& assetFetcher);
};
} // namespace Cesium3DTilesContent
