#pragma once
#include <CesiumGltf/Camera.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

namespace CesiumGltf {
    void writeCamera(
        const std::vector<Camera>& cameras, 
        rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter
    );
}