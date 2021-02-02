#pragma once
#include <CesiumGltf/Texture.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <vector>

namespace CesiumGltf {
    void writeTexture(
        const std::vector<Texture>& textures,
        rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter);
}