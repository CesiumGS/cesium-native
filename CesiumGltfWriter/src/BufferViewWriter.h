#pragma once
#include <CesiumGltf/BufferView.h>
#include "JsonWriter.h"
#include <vector>

namespace CesiumGltf {
    void writeBufferView(
        const std::vector<BufferView>& animations,
        CesiumGltf::JsonWriter& jsonWriter);
}