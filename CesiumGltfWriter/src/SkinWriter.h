#pragma once
#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumGltf/Skin.h>
#include <vector>

namespace CesiumGltf {
void writeSkin(
    const std::vector<Skin>& skins,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
