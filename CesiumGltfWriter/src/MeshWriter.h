#pragma once
#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumGltf/Mesh.h>
#include <vector>

namespace CesiumGltf {
void writeMesh(
    const std::vector<Mesh>& meshes,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
