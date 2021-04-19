#pragma once
#include "JsonWriter.h"
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/WriteGLTFCallback.h>
#include <CesiumGltf/WriteOptions.h>
#include <cstdint>
#include <utility>
#include <vector>

namespace CesiumGltf {
void writeBuffer(
    const std::vector<Buffer>& buffers,
    JsonWriter& jsonWriter,
    const WriteOptions& options,
    WriteGLTFCallback writeGLTFCallback = noopGltfWriter);
}
