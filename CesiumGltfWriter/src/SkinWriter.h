#pragma once
#include <CesiumGltf/Skin.h>
#include <CesiumJsonWriter/JsonWriter.h>
#include <vector>

namespace CesiumGltf {
void writeSkin(
    const std::vector<Skin>& skins,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
