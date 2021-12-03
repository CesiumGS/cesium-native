#pragma once

#include <CesiumGltf/Texture.h>
#include <CesiumJsonWriter/JsonWriter.h>

#include <vector>

namespace CesiumGltfWriter {
void writeTexture(
    const std::vector<CesiumGltf::Texture>& textures,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
