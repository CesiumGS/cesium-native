#pragma once

namespace CesiumGltf {
struct Model;
}

namespace CesiumGltfReader {
struct GltfReaderResult;
}

namespace CesiumGltfReader {

/**
 * @brief Decodes the mesh data in the model according to the
 * EXT_meshopt_compression extension
 *
 * The decompressed buffer may be in a quantized format as specified by the
 * KHR_mesh_quantization extension, in which case the data will have to be
 * dequantized to get the original values.
 **/
void decodeMeshOpt(
    CesiumGltf::Model& model,
    CesiumGltfReader::GltfReaderResult& readGltf);
} // namespace CesiumGltfReader
