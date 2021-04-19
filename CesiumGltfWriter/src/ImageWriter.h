#pragma once
#include "JsonWriter.h"
#include <CesiumGltf/Image.h>
#include <CesiumGltf/WriteGLTFCallback.h>
#include <CesiumGltf/WriteModelOptions.h>
#include <vector>

namespace CesiumGltf {
void writeImage(
    const std::vector<Image>& images,
    CesiumGltf::JsonWriter& jsonWriter,
    const WriteModelOptions& flags,
    WriteGLTFCallback writeGLTFCallback = noopGltfWriter);
}
