#pragma once
#include <CesiumJsonWriter/JsonWriter.h>
#include <CesiumGltf/Asset.h>

namespace CesiumGltf {
void writeAsset(const Asset& asset, CesiumJsonWriter::JsonWriter& jsonWriter);
}
