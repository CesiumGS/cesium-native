#pragma once

#include "CesiumGltfWriter/WriteGLTFCallback.h"

// forward declarations
namespace CesiumGltfWriter {
struct WriteModelResult;
struct WriteModelOptions;
} // namespace CesiumGltfWriter

// forward declarations
namespace CesiumGltf {
struct Buffer;
}

// forward declarations
namespace CesiumJsonWriter {
class JsonWriter;
}

namespace CesiumGltfWriter {
void writeBuffer(
    WriteModelResult& result,
    const std::vector<CesiumGltf::Buffer>& buffers,
    CesiumJsonWriter::JsonWriter& jsonWriter,
    const WriteModelOptions& options,
    WriteGLTFCallback& writeGLTFCallback = noopGltfWriter);
}
