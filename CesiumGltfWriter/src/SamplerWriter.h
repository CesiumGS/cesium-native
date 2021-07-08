#pragma once
#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumGltf/Sampler.h>
#include <vector>

namespace CesiumGltf {
void writeSampler(
    const std::vector<Sampler>& samplers,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
