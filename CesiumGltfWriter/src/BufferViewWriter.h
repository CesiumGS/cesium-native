#pragma once
#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumGltf/BufferView.h>
#include <vector>

namespace CesiumGltf {
void writeBufferView(
    const std::vector<BufferView>& animations,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
