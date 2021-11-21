#pragma once

// forward declarations
namespace CesiumGltf {
struct ExtensionMaterialsUnlit;
}

// forward declarations
namespace CesiumJsonWriter {
class JsonWriter;
}

namespace CesiumGltfWriter {
void writeMaterialsUnlit(
    const CesiumGltf::ExtensionMaterialsUnlit& extensionMaterialsUnlit,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
