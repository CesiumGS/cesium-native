#pragma once
#include <CesiumGltf/Skin.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <vector>

namespace CesiumGltf {
    void writeSkin(
        const std::vector<Skin>& skins,
        rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter
    );
}