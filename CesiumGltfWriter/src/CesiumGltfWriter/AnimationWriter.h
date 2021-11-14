#pragma once

#include <vector>

namespace CesiumGltfWriter {
struct WriteModelResult;
}
namespace CesiumGltf {
struct Animation;
}
namespace CesiumJsonWriter {
struct JsonWriter;
}

namespace CesiumGltfWriter {
void writeAnimation(
    WriteModelResult& result,
    const std::vector<CesiumGltf::Animation>& animations,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
