#pragma once

#include <vector>

// forward declarations
namespace CesiumGltf {
struct Mesh;
}

// forward declarations
namespace CesiumJsonWriter {
class JsonWriter;
}

namespace CesiumGltfWriter {
void writeMesh(
    const std::vector<CesiumGltf::Mesh>& meshes,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
