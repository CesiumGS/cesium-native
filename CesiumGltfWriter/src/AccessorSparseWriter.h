#pragma once

#include <CesiumGltf/AccessorSparse.h>
#include <CesiumJsonWriter/JsonWriter.h>

namespace CesiumGltfWriter {
void writeAccessorSparse(
    const CesiumGltf::AccessorSparse& accessorSparse,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
