#pragma once

#include <Cesium3DTilesSelection/Library.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumImage/Ktx2TranscodeTargets.h>

namespace Cesium3DTilesSelection {

/**
 * @brief Options for configuring the parsing the contents of a @ref Tileset,
 * including construction of its glTF models.
 */
struct CESIUM3DTILESSELECTION_API TilesetContentOptions {
  /**
   * @brief Whether to include a water mask within the Gltf extras.
   *
   * Currently only applicable for quantized-mesh tilesets that support the
   * water mask extension.
   */
  bool enableWaterMask = false;

  /**
   * @brief Whether to generate smooth normals when normals are missing in the
   * original Gltf.
   *
   * According to the Gltf spec: "When normals are not specified, client
   * implementations should calculate flat normals." However, calculating flat
   * normals requires duplicating vertices. This option allows the gltfs to be
   * sent with explicit smooth normals when the original gltf was missing
   * normals.
   */
  bool generateMissingNormalsSmooth = false;

  /**
   * @brief For each possible input transmission format, this struct names
   * the ideal target gpu-compressed pixel format to transcode to.
   */
  CesiumImage::Ktx2TranscodeTargets ktx2TranscodeTargets;

  /**
   * @brief Whether or not to transform texture coordinates during load when
   * textures have the `KHR_texture_transform` extension. Set this to false if
   * texture coordinates will be transformed another way, such as in a vertex
   * shader.
   */
  bool applyTextureTransform = true;

  /**
   * @brief Options for handling values of @ref CesiumGltf::MeshPrimitive::Mode
   * that appear in a glTF mesh primitive.
   */
  CesiumGltfReader::MeshPrimitiveModeOptions primitiveModeOptions;

  /**
   * @brief Extracts options related to loading glTFs to a new instance of @ref
   * CesiumGltfReader::GltfReaderOptions .
   */
  CesiumGltfReader::GltfReaderOptions toGltfReaderOptions() const;
};

} // namespace Cesium3DTilesSelection
