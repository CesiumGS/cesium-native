#pragma once

#include <CesiumGltf/Image.h>
#include <CesiumGltfWriter/WriteGLTFCallback.h>
#include <CesiumGltfWriter/WriteModelOptions.h>
#include <CesiumGltfWriter/WriteModelResult.h>
#include <CesiumJsonWriter/JsonWriter.h>

#include <vector>

namespace CesiumGltfWriter {
void writeImage(
    WriteModelResult& result,
    const std::vector<CesiumGltf::Image>& images,
    CesiumJsonWriter::JsonWriter& jsonWriter,
    const WriteModelOptions& flags,
    WriteGLTFCallback writeGLTFCallback = noopGltfWriter);
}
