#pragma once
#include <CesiumGltf/BufferView.h>
#include <CesiumJsonWriter/JsonWriter.h>

#include <vector>

namespace CesiumGltf {
void writeBufferView(
    const std::vector<BufferView>& animations,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
