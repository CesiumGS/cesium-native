#pragma once

namespace CesiumGltf {
struct Model;
}

namespace CesiumGltfReader {

void decodeQuantized(CesiumGltf::Model& model);
} // namespace CesiumGltfReader
