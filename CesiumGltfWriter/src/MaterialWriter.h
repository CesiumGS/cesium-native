#pragma once
#include <CesiumGltf/Material.h>
#include <CesiumJsonWriter/JsonWriter.h>
#include <vector>

namespace CesiumGltf {
void writeMaterial(
    const std::vector<Material>& materials,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
