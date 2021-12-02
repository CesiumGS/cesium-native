#pragma once

#include <CesiumGltf/Material.h>
#include <CesiumJsonWriter/JsonWriter.h>

#include <vector>

namespace CesiumGltfWriter {
void writeMaterial(
    const std::vector<CesiumGltf::Material>& materials,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
