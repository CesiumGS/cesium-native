#pragma once

#include <vector>

// forward declarations
namespace CesiumGltf {
struct Material;
}

// forward declarations
namespace CesiumJsonWriter {
class JsonWriter;
}

namespace CesiumGltfWriter {
void writeMaterial(
    const std::vector<CesiumGltf::Material>& materials,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
