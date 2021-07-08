#pragma once
#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumGltf/Material.h>
#include <vector>

namespace CesiumGltf {
void writeMaterial(
    const std::vector<Material>& materials,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
