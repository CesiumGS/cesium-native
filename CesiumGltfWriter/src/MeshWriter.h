#pragma once
#include <CesiumGltf/Mesh.h>
#include "JsonWriter.h"
#include <vector>

namespace CesiumGltf {
    void writeMesh(
        const std::vector<Mesh>& meshes,
        CesiumGltf::JsonWriter& jsonWriter);
}