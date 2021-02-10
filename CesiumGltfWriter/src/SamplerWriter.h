#pragma once
#include <CesiumGltf/Sampler.h>
#include "JsonWriter.h"
#include <vector>

namespace CesiumGltf {
    void writeSampler(
        const std::vector<Sampler>& samplers,
        CesiumGltf::JsonWriter& jsonWriter
    );
}