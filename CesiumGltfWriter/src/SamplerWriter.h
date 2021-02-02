#pragma once
#include <CesiumGltf/Sampler.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <vector>

namespace CesiumGltf {
    void writeSampler(
        const std::vector<Sampler>& samplers,
        rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter
    );
}