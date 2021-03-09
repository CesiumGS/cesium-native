#pragma once
#include "JsonWriter.h"
#include <CesiumGltf/Scene.h>
#include <vector>

namespace CesiumGltf {
void writeScene(
    const std::vector<Scene>& scenes,
    CesiumGltf::JsonWriter& jsonWriter);
}