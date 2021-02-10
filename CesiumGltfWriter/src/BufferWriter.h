#pragma once
#include <CesiumGltf/Buffer.h>
#include "JsonWriter.h"
#include <utility>
#include <vector>
#include <cstdint>

namespace CesiumGltf {
    std::vector<std::uint8_t> writeBuffer(
        const std::vector<Buffer>& buffers,
        CesiumGltf::JsonWriter& jsonWriter
    );
}