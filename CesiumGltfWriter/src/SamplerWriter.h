#pragma once
#include <CesiumGltf/Sampler.h>
#include <CesiumJsonWriter/JsonWriter.h>
#include <vector>

namespace CesiumGltf {
void writeSampler(
    const std::vector<Sampler>& samplers,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
