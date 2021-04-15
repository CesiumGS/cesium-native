#pragma once
#include "JsonWriter.h"
#include <any>

namespace CesiumGltf {
void writeExtensions(
    const std::vector<std::any>& extensions,
    CesiumGltf::JsonWriter& jsonWriter);
}
