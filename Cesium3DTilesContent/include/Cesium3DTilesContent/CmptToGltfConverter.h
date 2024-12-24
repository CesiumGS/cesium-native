#pragma once

#include <Cesium3DTilesContent/GltfConverterResult.h>
#include <CesiumAsync/Future.h>
#include <CesiumGltfReader/GltfReader.h>

#include <cstddef>
#include <span>

namespace Cesium3DTilesContent {
struct AssetFetcher;

/**
 * @brief Converts a cmpt (Composite) file into a glTF model.
 *
 * For more information on the Composite format, see
 * https://github.com/CesiumGS/3d-tiles/blob/main/specification/TileFormats/Composite/README.adoc
 */
struct CmptToGltfConverter {
  /**
   * @brief Converts a cmpt binary file to a glTF model.
   *
   * @param cmptBinary The bytes loaded for the cmpt model.
   * @param options Options for how the glTF should be loaded.
   * @param assetFetcher The \ref AssetFetcher containing information used by
   * loaded assets.
   * @returns A future that resolves to a \ref GltfConverterResult.
   */
  static CesiumAsync::Future<GltfConverterResult> convert(
      const std::span<const std::byte>& cmptBinary,
      const CesiumGltfReader::GltfReaderOptions& options,
      const AssetFetcher& assetFetcher);
};
} // namespace Cesium3DTilesContent
