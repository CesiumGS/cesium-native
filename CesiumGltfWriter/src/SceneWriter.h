#pragma once
#include <CesiumGltf/Scene.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <vector>

namespace CesiumGltf {
    void writeScene(
        const std::vector<Scene>& scenes,
        rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter
    );
}