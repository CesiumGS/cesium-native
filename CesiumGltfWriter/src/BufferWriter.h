#pragma once
#include "JsonWriter.h"
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/WriteFlags.h>
#include <CesiumGltf/WriteGLTFCallback.h>
#include <cstdint>
#include <utility>
#include <vector>

namespace CesiumGltf {
void writeBuffer(
    const std::vector<Buffer>& buffers,
    JsonWriter& jsonWriter,
    WriteFlags flags,
    WriteGLTFCallback writeGLTFCallback = noopGltfWriter);
}