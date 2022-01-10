#pragma once

#include <CesiumGltf/Asset.h>
#include <CesiumJsonWriter/JsonWriter.h>

namespace CesiumGltfWriter {
void writeAsset(
    const CesiumGltf::Asset& asset,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
