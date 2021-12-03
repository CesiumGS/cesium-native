#pragma once

#include <CesiumGltf/Mesh.h>
#include <CesiumJsonWriter/JsonWriter.h>

#include <vector>

namespace CesiumGltfWriter {
void writeMesh(
    const std::vector<CesiumGltf::Mesh>& meshes,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
