#pragma once
#include "WriterLibrary.h"
namespace CesiumGltf {

enum class GltfExportType { GLB, GLTF };

/**
 * @brief Options for how to write a glTF asset.
 */

CESIUMGLTFWRITER_API struct WriteModelOptions {
  /**
   * @brief If the glTF asset should be exported as a GLB (binary) or glTF (JSON
   * string)
   */
  GltfExportType exportType = GltfExportType::GLB;

  /**
   * @brief If the glTF asset should be pretty printed. Usable with glTF or GLB
   * (not advised).
   */
  bool prettyPrint = false;

  /**
   * @brief If {@link CesiumGltf::BufferCesium} and
   * {@link CesiumGltf::ImageCesium} should be automatically encoded into base64
   * uris or not.
   */
  bool autoConvertDataToBase64 = false;
};

} // namespace CesiumGltf
