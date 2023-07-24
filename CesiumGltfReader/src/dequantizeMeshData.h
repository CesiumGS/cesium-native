#pragma once

namespace CesiumGltf {
struct Model;
}

namespace CesiumGltfReader {

void dequantizeMeshData(CesiumGltf::Model& model);
} // namespace CesiumGltfReader
