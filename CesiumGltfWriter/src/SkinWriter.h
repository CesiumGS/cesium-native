#pragma once
#include "JsonWriter.h"
#include <CesiumGltf/Skin.h>
#include <vector>

namespace CesiumGltf {
void writeSkin(
    const std::vector<Skin>& skins,
    CesiumGltf::JsonWriter& jsonWriter);
}
