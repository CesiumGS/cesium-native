#pragma once

#include <CesiumGltf/Camera.h>
#include <CesiumJsonWriter/JsonWriter.h>

namespace CesiumGltfWriter {
void writeCamera(
    const std::vector<CesiumGltf::Camera>& cameras,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
