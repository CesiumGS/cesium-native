#pragma once

#include <CesiumGltf/Sampler.h>
#include <CesiumJsonWriter/JsonWriter.h>

#include <vector>

namespace CesiumGltfWriter {
void writeSampler(
    const std::vector<CesiumGltf::Sampler>& samplers,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
