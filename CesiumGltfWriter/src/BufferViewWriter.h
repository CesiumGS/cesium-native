#pragma once
#include "JsonWriter.h"
#include <CesiumGltf/BufferView.h>
#include <vector>

namespace CesiumGltf {
void writeBufferView(
    const std::vector<BufferView>& animations,
    CesiumGltf::JsonWriter& jsonWriter);
}
