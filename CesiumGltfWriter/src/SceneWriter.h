#pragma once
#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumGltf/Scene.h>
#include <vector>

namespace CesiumGltf {
void writeScene(
    const std::vector<Scene>& scenes,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
