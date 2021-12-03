#pragma once

#include <CesiumGltf/Animation.h>
#include <CesiumGltfWriter/WriteModelResult.h>
#include <CesiumJsonWriter/JsonWriter.h>

#include <vector>

namespace CesiumGltfWriter {
void writeAnimation(
    CesiumGltfWriter::WriteModelResult& result,
    const std::vector<CesiumGltf::Animation>& animations,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
