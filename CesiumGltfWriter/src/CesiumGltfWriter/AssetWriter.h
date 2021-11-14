#pragma once

// forward declarations
namespace CesiumGltf {
struct Asset;
}

// forward declarations
namespace CesiumJsonWriter {
struct JsonWriter;
}

namespace CesiumGltfWriter {
void writeAsset(
    const CesiumGltf::Asset& asset,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
