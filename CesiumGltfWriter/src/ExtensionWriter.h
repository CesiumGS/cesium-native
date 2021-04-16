#pragma once
#include "JsonWriter.h"
#include <any>
#include <unordered_map>

namespace CesiumGltf {
void writeExtensions(
    const std::unordered_map<std::string, std::any>& extensions,
    CesiumGltf::JsonWriter& jsonWriter);
}
