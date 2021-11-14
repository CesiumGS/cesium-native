#pragma once

#include <vector>

// forward declarations
namespace CesiumGltf {
struct Node;
}

// forward declarations
namespace CesiumJsonWriter {
class JsonWriter;
}

namespace CesiumGltfWriter {
void writeNode(
    const std::vector<CesiumGltf::Node>& images,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
