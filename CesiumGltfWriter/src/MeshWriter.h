#pragma once
#include <CesiumGltf/Mesh.h>
#include <CesiumJsonWriter/JsonWriter.h>

#include <vector>

namespace CesiumGltf {
void writeMesh(
    const std::vector<Mesh>& meshes,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
