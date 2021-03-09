#pragma once
#include "JsonWriter.h"
#include <CesiumGltf/Sampler.h>
#include <vector>

namespace CesiumGltf {
void writeSampler(
    const std::vector<Sampler>& samplers,
    CesiumGltf::JsonWriter& jsonWriter);
}