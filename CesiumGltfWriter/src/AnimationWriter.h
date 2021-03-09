#pragma once
#include "JsonWriter.h"
#include <CesiumGltf/Animation.h>
#include <vector>

namespace CesiumGltf {
void writeAnimation(
    const std::vector<Animation>& animations,
    CesiumGltf::JsonWriter& jsonWriter);
}