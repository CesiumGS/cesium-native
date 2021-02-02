#pragma once
#include <CesiumGltf/Image.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <vector>

namespace CesiumGltf {
    void writeImage(
        const std::vector<Image>& images,
        rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter
    );
}