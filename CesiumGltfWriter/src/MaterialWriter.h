#pragma once
#include <CesiumGltf/Material.h>
#include "JsonWriter.h"
#include <vector>

namespace CesiumGltf {
    void writeMaterial(
        const std::vector<Material>& materials,
        CesiumGltf::JsonWriter& jsonWriter
    );
}