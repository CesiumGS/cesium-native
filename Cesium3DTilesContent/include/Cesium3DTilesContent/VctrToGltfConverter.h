#pragma once

#include "GltfConverterResult.h"

#include <Cesium3DTilesContent/GltfConverters.h> // Include for AssetFetcher definition
#include <CesiumAsync/Future.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltfReader/GltfReader.h>

#include <optional>
#include <span>

namespace Cesium3DTilesContent {

/**
 * @brief Converts a vctr (Vector) file to a glTF.
 *
 * For more information on the vctr format, see
 * https://github.com/CesiumGS/3d-tiles/blob/main/specification/TileFormats/VectorData/README.md
 */
struct VctrToGltfConverter {
  /**
   * @brief Converts a vctr binary file to a glTF model.
   *
   * @param vctrBinary The bytes loaded for the vctr model.
   * @param options Options for how the glTF should be loaded.
   * @param assetFetcher The \ref AssetFetcher containing information used by
   * loaded assets.
   * @returns A future that resolves to a \ref GltfConverterResult.
   */
  static CesiumAsync::Future<GltfConverterResult> convert(
      const std::span<const std::byte>& vctrBinary,
      const CesiumGltfReader::GltfReaderOptions& options,
      const AssetFetcher& assetFetcher);
};
} // namespace Cesium3DTilesContent
