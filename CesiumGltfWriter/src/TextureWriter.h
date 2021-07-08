#pragma once
#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumGltf/Texture.h>
#include <vector>

namespace CesiumGltf {
void writeTexture(
    const std::vector<Texture>& textures,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
