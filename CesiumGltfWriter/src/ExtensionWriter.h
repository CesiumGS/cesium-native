#pragma once

#include <CesiumJsonWriter/JsonWriter.h>

#include <any>
#include <map>

namespace CesiumGltf {
void writeExtensions(
    const std::map<std::string, std::any>& extensions,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
