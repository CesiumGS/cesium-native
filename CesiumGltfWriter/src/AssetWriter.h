#pragma once
#include <CesiumGltf/Asset.h>
#include <CesiumJsonWriter/JsonWriter.h>

namespace CesiumGltf {
void writeAsset(const Asset& asset, CesiumJsonWriter::JsonWriter& jsonWriter);
}
