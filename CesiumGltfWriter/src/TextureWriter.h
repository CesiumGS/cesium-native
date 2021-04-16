#pragma once
#include "JsonWriter.h"
#include <CesiumGltf/Texture.h>
#include <vector>

namespace CesiumGltf {
void writeTexture(
    const std::vector<Texture>& textures,
    CesiumGltf::JsonWriter& jsonWriter);
}
