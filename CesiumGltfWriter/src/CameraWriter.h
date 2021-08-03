#pragma once
#include <CesiumGltf/Camera.h>
#include <CesiumJsonWriter/JsonWriter.h>

namespace CesiumGltf {
void writeCamera(
    const std::vector<Camera>& cameras,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
