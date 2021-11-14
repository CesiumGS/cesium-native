#pragma once

#include <vector>

// forward declarations
namespace CesiumGltf {
class Accessor;
}

// forward declarations
namespace CesiumJsonWriter {
class JsonWriter;
}

namespace CesiumGltfWriter {
void writeAccessor(
    const std::vector<CesiumGltf::Accessor>& accessors,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
