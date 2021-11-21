#include "CesiumGltfWriter/ExtensionMaterialsUnlit.h"

#include <CesiumGltf/ExtensionMaterialsUnlit.h>
#include <CesiumJsonWriter/JsonWriter.h>

namespace CesiumGltfWriter {

void writeMaterialsUnlit(
    const CesiumGltf::ExtensionMaterialsUnlit& extensionMaterialsUnlit,
    CesiumJsonWriter::JsonWriter& jsonWriter) {
  (void)extensionMaterialsUnlit;
  auto& j = jsonWriter;
  j.StartObject();
  j.EndObject();
}

} // namespace CesiumGltfWriter
