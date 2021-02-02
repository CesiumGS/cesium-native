#pragma once
#include <CesiumGltf/Material.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <vector>

namespace CesiumGltf {
    void writeMaterial(
        const std::vector<Material>& materials,
        rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter
    );
}