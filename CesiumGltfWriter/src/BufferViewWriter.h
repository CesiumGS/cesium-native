#pragma once
#include <CesiumGltf/BufferView.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <vector>

namespace CesiumGltf {
    void writeBufferView(
        const std::vector<BufferView>& animations,
        rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter);
}