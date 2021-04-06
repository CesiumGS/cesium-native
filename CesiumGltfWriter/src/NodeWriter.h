#pragma once
#include "JsonWriter.h"
#include <CesiumGltf/Node.h>
#include <vector>

namespace CesiumGltf {
void writeNode(
    const std::vector<Node>& images,
    CesiumGltf::JsonWriter& jsonWriter);
}
