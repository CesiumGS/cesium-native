#pragma once
#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumGltf/Node.h>
#include <vector>

namespace CesiumGltf {
void writeNode(
    const std::vector<Node>& images,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
