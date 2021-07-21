#pragma once
#include <CesiumGltf/Node.h>
#include <CesiumJsonWriter/JsonWriter.h>
#include <vector>

namespace CesiumGltf {
void writeNode(
    const std::vector<Node>& images,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
