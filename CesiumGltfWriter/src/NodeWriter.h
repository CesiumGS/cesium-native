#pragma once

#include <CesiumGltf/Node.h>
#include <CesiumJsonWriter/JsonWriter.h>

#include <vector>

namespace CesiumGltfWriter {
void writeNode(
    const std::vector<CesiumGltf::Node>& images,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
