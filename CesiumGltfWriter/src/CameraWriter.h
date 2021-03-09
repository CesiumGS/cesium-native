#pragma once
#include "JsonWriter.h"
#include <CesiumGltf/Camera.h>

namespace CesiumGltf {
void writeCamera(
    const std::vector<Camera>& cameras,
    CesiumGltf::JsonWriter& jsonWriter);
}