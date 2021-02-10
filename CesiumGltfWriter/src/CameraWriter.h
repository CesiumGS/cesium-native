#pragma once
#include <CesiumGltf/Camera.h>
#include "JsonWriter.h"

namespace CesiumGltf {
    void writeCamera(
        const std::vector<Camera>& cameras, 
        CesiumGltf::JsonWriter& jsonWriter
    );
}