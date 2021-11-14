#pragma once

#include <vector>

// forward declarations
namespace CesiumGltf {
struct Accessor;
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
