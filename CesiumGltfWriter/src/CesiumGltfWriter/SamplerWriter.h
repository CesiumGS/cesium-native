#pragma once

#include <vector>

// forward declarations
namespace CesiumGltf {
struct Sampler;
}

// forward declarations
namespace CesiumJsonWriter {
class JsonWriter;
}

namespace CesiumGltfWriter {
void writeSampler(
    const std::vector<CesiumGltf::Sampler>& samplers,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
