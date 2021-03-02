#pragma once
#include <CesiumGltf/Scene.h>
#include "JsonWriter.h"
#include <vector>

namespace CesiumGltf {
    void writeScene(
        const std::vector<Scene>& scenes,
        CesiumGltf::JsonWriter& jsonWriter
    );
}