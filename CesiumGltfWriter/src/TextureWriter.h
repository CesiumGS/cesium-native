#pragma once
#include <CesiumGltf/Texture.h>
#include "JsonWriter.h"
#include <vector>

namespace CesiumGltf {
    void writeTexture(
        const std::vector<Texture>& textures,
        CesiumGltf::JsonWriter& jsonWriter);
}