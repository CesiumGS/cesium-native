#pragma once
#include "JsonWriter.h"
#include <CesiumGltf/Image.h>
#include <CesiumGltf/WriteFlags.h>
#include <CesiumGltf/WriteGLTFCallback.h>
#include <vector>

namespace CesiumGltf {
void writeImage(
    const std::vector<Image>& images,
    CesiumGltf::JsonWriter& jsonWriter,
    WriteFlags flags,
    WriteGLTFCallback writeGLTFCallback = noopGltfWriter);
}
