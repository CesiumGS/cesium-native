#pragma once
#include "JsonWriter.h"
#include <CesiumGltf/Asset.h>

namespace CesiumGltf {
void writeAsset(const Asset& asset, CesiumGltf::JsonWriter& jsonWriter);
}
