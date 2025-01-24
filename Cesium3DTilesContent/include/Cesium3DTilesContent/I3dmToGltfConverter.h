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
 * @brief Converts an i3dm (Instanced 3D Model) file to a glTF model.
 *
 * For more information on the i3dm format, see
 * https://github.com/CesiumGS/3d-tiles/blob/main/specification/TileFormats/Instanced3DModel/README.adoc
 */
struct I3dmToGltfConverter {
  /**
   * @brief Converts an i3dm binary file to a glTF model.
   *
   * @param instancesBinary The bytes loaded for the i3dm model.
   * @param options Options for how the glTF should be loaded.
   * @param assetFetcher The \ref AssetFetcher containing information used by
   * loaded assets.
   * @returns A future that resolves to a \ref GltfConverterResult.
   */
  static CesiumAsync::Future<GltfConverterResult> convert(
      const std::span<const std::byte>& instancesBinary,
      const CesiumGltfReader::GltfReaderOptions& options,
      const AssetFetcher& assetFetcher);
};
} // namespace Cesium3DTilesContent
