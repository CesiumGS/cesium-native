#pragma once
#include "JsonWriter.h"
#include <CesiumGltf/Image.h>
#include <CesiumGltf/WriteGLTFCallback.h>
#include <CesiumGltf/WriteOptions.h>
#include <vector>

namespace CesiumGltf {
void writeImage(
    const std::vector<Image>& images,
    CesiumGltf::JsonWriter& jsonWriter,
    const WriteOptions& flags,
    WriteGLTFCallback writeGLTFCallback = noopGltfWriter);
}
