#pragma once
#include <CesiumGltf/Mesh.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <vector>

namespace CesiumGltf {
    void writeMesh(
        const std::vector<Mesh>& meshes,
        rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter);
}