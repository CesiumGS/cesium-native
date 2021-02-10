#pragma once
#include <any>
#include "JsonWriter.h"

namespace CesiumGltf {
    void writeExtensions(
        const std::vector<std::any>& extensions, 
        CesiumGltf::JsonWriter& jsonWriter
    );
}