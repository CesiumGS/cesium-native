#pragma once
#include <CesiumGltf/Buffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <utility>
#include <vector>
#include <cstdint>

namespace CesiumGltf {
    std::vector<std::uint8_t> writeBuffer(
        const std::vector<Buffer>& buffers,
        rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter
    );
}