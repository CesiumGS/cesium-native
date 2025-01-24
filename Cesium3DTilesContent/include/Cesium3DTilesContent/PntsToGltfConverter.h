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
 * @brief Converts a pnts (Point Cloud) file to a glTF model.
 *
 * For more information on the pnts format, see
 * https://github.com/CesiumGS/3d-tiles/blob/main/specification/TileFormats/PointCloud/README.adoc
 */
struct PntsToGltfConverter {
  /**
   * @brief Converts an pnts binary file to a glTF model.
   *
   * @param pntsBinary The bytes loaded for the pnts model.
   * @param options Options for how the glTF should be loaded.
   * @param assetFetcher The \ref AssetFetcher containing information used by
   * loaded assets.
   * @returns A future that resolves to a \ref GltfConverterResult.
   */
  static CesiumAsync::Future<GltfConverterResult> convert(
      const std::span<const std::byte>& pntsBinary,
      const CesiumGltfReader::GltfReaderOptions& options,
      const AssetFetcher& assetFetcher);
};
} // namespace Cesium3DTilesContent
