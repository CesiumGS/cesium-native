#pragma once
#include "WriterLibrary.h"
namespace CesiumGltf {

enum class GltfExportType { GLB, GLTF };

CESIUMGLTFWRITER_API struct WriteOptions {
  GltfExportType exportType = GltfExportType::GLB;
  bool prettyPrint = false;
  bool autoConvertDataToBase64 = false;
};

} // namespace CesiumGltf
