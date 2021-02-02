#pragma once
#include <CesiumGltf/Node.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <vector>

namespace CesiumGltf {
    void writeNode(
        const std::vector<Node>& images,
        rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter
    );
}