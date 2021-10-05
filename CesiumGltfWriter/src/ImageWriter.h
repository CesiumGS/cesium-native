#pragma once
#include <CesiumGltf/Image.h>
#include <CesiumGltf/WriteGLTFCallback.h>
#include <CesiumGltf/WriteModelOptions.h>
#include <CesiumGltf/WriteModelResult.h>
#include <CesiumJsonWriter/JsonWriter.h>

#include <vector>

namespace CesiumGltf {
void writeImage(
    WriteModelResult& result,
    const std::vector<Image>& images,
    CesiumJsonWriter::JsonWriter& jsonWriter,
    const WriteModelOptions& flags,
    WriteGLTFCallback writeGLTFCallback = noopGltfWriter);
}
