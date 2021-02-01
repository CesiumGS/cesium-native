#pragma once
#include <CesiumGltf/Asset.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

namespace CesiumGltf {
    void writeAsset(
        const Asset& asset,
        rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter
    );
}