#pragma once

namespace CesiumGltf {
struct Model;
}

namespace CesiumGltfReader {
struct GltfReaderResult;
}

namespace CesiumGltfReader {

void decodeMeshOpt(
    CesiumGltf::Model& model,
    CesiumGltfReader::GltfReaderResult& readGltf);
} // namespace CesiumGltfReader
