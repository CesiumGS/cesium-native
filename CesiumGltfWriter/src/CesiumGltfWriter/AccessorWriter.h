#pragma once

#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/AccessorSpec.h>
#include <CesiumJsonWriter/JsonWriter.h>

namespace CesiumGltfWriter {
void writeAccessor(
    const std::vector<CesiumGltf::Accessor>& accessors,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
