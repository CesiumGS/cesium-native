#pragma once

#include <vector>

// forward declarations
namespace CesiumGltf {
struct BufferView;
}

// forward declarations
namespace CesiumJsonWriter {
class JsonWriter;
}

namespace CesiumGltfWriter {
void writeBufferView(
    const std::vector<CesiumGltf::BufferView>& animations,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
