#pragma once
#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumGltf/Camera.h>

namespace CesiumGltf {
void writeCamera(
    const std::vector<Camera>& cameras,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
