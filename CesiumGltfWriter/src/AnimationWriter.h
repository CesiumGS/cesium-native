#pragma once
#include <CesiumGltf/Animation.h>
#include <CesiumGltf/WriteModelResult.h>
#include <CesiumJsonWriter/JsonWriter.h>
#include <vector>

namespace CesiumGltf {
void writeAnimation(
    CesiumGltf::WriteModelResult& result,
    const std::vector<Animation>& animations,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
