#pragma once
#include "CesiumGltf/AccessorSparse.h"
#include <CesiumJsonWriter/JsonWriter.h>

namespace CesiumGltf {
void writeAccessorSparse(
    const AccessorSparse& accessorSparse,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
