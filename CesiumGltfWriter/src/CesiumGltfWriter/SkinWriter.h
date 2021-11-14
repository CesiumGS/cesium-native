#pragma once

// forward declarations
namespace CesiumGltf {
struct Skin;
}

// forward declarations
namespace CesiumJsonWriter {
class JsonWriter;
}

#include <vector>

namespace CesiumGltfWriter {
void writeSkin(
    const std::vector<CesiumGltf::Skin>& skins,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
