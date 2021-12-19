#pragma once

#include <CesiumGltf/Skin.h>
#include <CesiumJsonWriter/JsonWriter.h>

#include <vector>

namespace CesiumGltfWriter {
void writeSkin(
    const std::vector<CesiumGltf::Skin>& skins,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
