#pragma once
#include <CesiumGltf/Scene.h>
#include <CesiumJsonWriter/JsonWriter.h>
#include <vector>

namespace CesiumGltf {
void writeScene(
    const std::vector<Scene>& scenes,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
