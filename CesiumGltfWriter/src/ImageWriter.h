#pragma once
#include "JsonWriter.h"
#include <CesiumGltf/Image.h>
#include <vector>

namespace CesiumGltf {
    void writeImage(
        const std::vector<Image>& images,
        CesiumGltf::JsonWriter& jsonWriter
    );
}