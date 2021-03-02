#pragma once
#include <CesiumGltf/Skin.h>
#include "JsonWriter.h"
#include <vector>

namespace CesiumGltf {
    void writeSkin(
        const std::vector<Skin>& skins,
        CesiumGltf::JsonWriter& jsonWriter
    );
}