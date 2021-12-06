#pragma once

#include <CesiumGltf/Buffer.h>
#include <CesiumGltfWriter/WriteGLTFCallback.h>
#include <CesiumGltfWriter/WriteModelOptions.h>
#include <CesiumGltfWriter/WriteModelResult.h>
#include <CesiumJsonWriter/JsonWriter.h>

#include <cstdint>
#include <utility>
#include <vector>

namespace CesiumGltfWriter {
void writeBuffer(
    WriteModelResult& result,
    const std::vector<CesiumGltf::Buffer>& buffers,
    CesiumJsonWriter::JsonWriter& jsonWriter,
    const WriteModelOptions& options,
    WriteGLTFCallback writeGLTFCallback = noopGltfWriter);
}
