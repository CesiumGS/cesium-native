#pragma once
#include <CesiumGltf/Animation.h>
#include "JsonWriter.h"
#include <vector>

namespace CesiumGltf {
    void writeAnimation(
        const std::vector<Animation>& animations,
        CesiumGltf::JsonWriter& jsonWriter);
}