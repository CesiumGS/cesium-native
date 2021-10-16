#pragma once

#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/AccessorSpec.h>
#include <CesiumJsonWriter/JsonWriter.h>

namespace CesiumGltf {
void writeAccessor(
    const std::vector<Accessor>& accessors,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
