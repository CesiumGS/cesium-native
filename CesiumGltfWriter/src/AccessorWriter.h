#pragma once
#include "CesiumGltf/Accessor.h"
#include "CesiumGltf/AccessorSpec.h"
#include "JsonWriter.h"

namespace CesiumGltf {
void writeAccessor(
    const std::vector<Accessor>& accessors,
    CesiumGltf::JsonWriter& jsonWriter);
}
