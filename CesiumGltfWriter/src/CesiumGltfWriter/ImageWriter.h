#pragma once

#include "CesiumGltfWriter/WriteGLTFCallback.h"

#include <vector>

// forward declarations
namespace CesiumGltfWriter {
struct WriteModelResult;
struct WriteModelOptions;
} // namespace CesiumGltfWriter

// forward declarations
namespace CesiumGltf {
struct Image;
}

// forward declarations
namespace CesiumJsonWriter {
class JsonWriter;
}

namespace CesiumGltfWriter {
void writeImage(
    WriteModelResult& result,
    const std::vector<CesiumGltf::Image>& images,
    CesiumJsonWriter::JsonWriter& jsonWriter,
    const WriteModelOptions& flags,
    WriteGLTFCallback& writeGLTFCallback = noopGltfWriter);
}
