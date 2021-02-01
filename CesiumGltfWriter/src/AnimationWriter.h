#pragma once
#include <CesiumGltf/Animation.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <vector>

namespace CesiumGltf {
    void writeAnimation(
        const std::vector<Animation>& animations,
        rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter);
}