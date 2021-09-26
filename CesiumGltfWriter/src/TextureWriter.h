#pragma once
#include <CesiumGltf/Texture.h>
#include <CesiumJsonWriter/JsonWriter.h>

#include <vector>

namespace CesiumGltf {
void writeTexture(
    const std::vector<Texture>& textures,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
