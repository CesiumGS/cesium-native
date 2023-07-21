#pragma once

namespace CesiumGltf {
struct Model;
}

namespace CesiumGltfReader {

void unquantizeMeshData(CesiumGltf::Model& model);
} // namespace CesiumGltfReader
