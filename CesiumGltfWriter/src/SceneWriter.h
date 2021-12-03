#pragma once

#include <CesiumGltf/Scene.h>
#include <CesiumJsonWriter/JsonWriter.h>

#include <vector>

namespace CesiumGltfWriter {
void writeScene(
    const std::vector<CesiumGltf::Scene>& scenes,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
