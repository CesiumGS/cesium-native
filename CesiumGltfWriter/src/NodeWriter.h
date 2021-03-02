#pragma once
#include <CesiumGltf/Node.h>
#include "JsonWriter.h"
#include <vector>

namespace CesiumGltf {
    void writeNode(
        const std::vector<Node>& images,
        CesiumGltf::JsonWriter& jsonWriter
    );
}