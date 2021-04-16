#pragma once
#include "JsonWriter.h"
#include <any>

namespace CesiumGltf {
void writeExtensions(
    const std::unordered_map<std::string, std::any>& extensions,
    CesiumGltf::JsonWriter& jsonWriter);
}
