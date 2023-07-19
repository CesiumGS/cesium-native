#pragma once

namespace CesiumGltf {
struct Model;
}

namespace CesiumGltfReader {

void decodeMeshOpt(CesiumGltf::Model& model);
} // namespace CesiumGltfReader
