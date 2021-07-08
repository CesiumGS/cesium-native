#pragma once
#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumGltf/Animation.h>
#include <CesiumGltf/WriteModelResult.h>
#include <vector>

namespace CesiumGltf {
void writeAnimation(
    CesiumGltf::WriteModelResult& result,
    const std::vector<Animation>& animations,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
