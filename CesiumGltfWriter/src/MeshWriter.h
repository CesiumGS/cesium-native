#pragma once
#include "JsonWriter.h"
#include <CesiumGltf/Mesh.h>
#include <vector>

namespace CesiumGltf {
void writeMesh(
    const std::vector<Mesh>& meshes,
    CesiumGltf::JsonWriter& jsonWriter);
}