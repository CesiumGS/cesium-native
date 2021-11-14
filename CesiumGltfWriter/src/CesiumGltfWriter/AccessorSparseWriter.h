#pragma once

// forward declarations
namespace CesiumGltf {
struct AccessorSparse;
}

// forward declarations
namespace CesiumJsonWriter {
class JsonWriter;
}

namespace CesiumGltfWriter {
void writeAccessorSparse(
    const CesiumGltf::AccessorSparse& accessorSparse,
    CesiumJsonWriter::JsonWriter& jsonWriter);
}
