#pragma once

#include <CesiumGltf/BufferView.h>
#include <CesiumJsonWriter/JsonWriter.h>

#include <vector>

namespace CesiumGltfWriter {
void writeBufferView(
    const std::vector<CesiumGltf::BufferView>& animations,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
