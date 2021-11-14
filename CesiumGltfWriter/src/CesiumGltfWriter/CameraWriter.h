#pragma once

#include <vector>

// forward declarations
namespace CesiumGltf {
struct Camera;
}

// forward declarations
namespace CesiumJsonWriter {
class JsonWriter;
}

namespace CesiumGltfWriter {
void writeCamera(
    const std::vector<CesiumGltf::Camera>& cameras,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
