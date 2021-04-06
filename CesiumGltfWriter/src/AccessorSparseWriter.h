#pragma once
#include "CesiumGltf/AccessorSparse.h"
#include "JsonWriter.h"

namespace CesiumGltf {
void writeAccessorSparse(
    const AccessorSparse& accessorSparse,
    CesiumGltf::JsonWriter& jsonWriter);
}
