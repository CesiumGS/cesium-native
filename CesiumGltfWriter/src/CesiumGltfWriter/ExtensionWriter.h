#pragma once

#include <any>
#include <map>
#include <string>

// forward declarations
namespace CesiumJsonWriter {
class JsonWriter;
}

namespace CesiumGltfWriter {
void writeExtensions(
    const std::map<std::string, std::any>& extensions,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
