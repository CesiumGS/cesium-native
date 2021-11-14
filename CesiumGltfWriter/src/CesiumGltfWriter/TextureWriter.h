#pragma once

#include <vector>

// forward declarations
namespace CesiumGltf {
struct Texture;
}

// forward declarations
namespace CesiumJsonWriter {
class JsonWriter;
}

namespace CesiumGltfWriter {
void writeTexture(
    const std::vector<CesiumGltf::Texture>& textures,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
