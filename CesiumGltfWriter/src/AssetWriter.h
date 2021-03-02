#pragma once
#include <CesiumGltf/Asset.h>
#include "JsonWriter.h"

namespace CesiumGltf {
    void writeAsset(
        const Asset& asset,
        CesiumGltf::JsonWriter& jsonWriter
    );
}