#pragma once

#include <vector>

// forward declarations
namespace CesiumGltf {
struct Scene;
}

// forward declarations
namespace CesiumJsonWriter {
class JsonWriter;
}

namespace CesiumGltfWriter {
void writeScene(
    const std::vector<CesiumGltf::Scene>& scenes,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
