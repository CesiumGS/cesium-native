#pragma once
#include "JsonWriter.h"
#include <CesiumGltf/Material.h>
#include <vector>

namespace CesiumGltf {
void writeMaterial(
    const std::vector<Material>& materials,
    CesiumGltf::JsonWriter& jsonWriter);
}
