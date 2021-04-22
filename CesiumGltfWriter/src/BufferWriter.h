#pragma once
#include "JsonWriter.h"
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/WriteGLTFCallback.h>
#include <CesiumGltf/WriteModelOptions.h>
#include <CesiumGltf/WriteModelResult.h>
#include <cstdint>
#include <utility>
#include <vector>

namespace CesiumGltf {
void writeBuffer(
    WriteModelResult& result,
    const std::vector<Buffer>& buffers,
    JsonWriter& jsonWriter,
    const WriteModelOptions& options,
    WriteGLTFCallback writeGLTFCallback = noopGltfWriter);
}
