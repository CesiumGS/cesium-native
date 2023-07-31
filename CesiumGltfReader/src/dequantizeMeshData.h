#pragma once

namespace CesiumGltf {
struct Model;
}

namespace CesiumGltfReader {

/**
 * @brief Dequantizes any quantized data in the accessors of the glTF model and
 * converts them to floating-point data as specified in the
 * KHR_quantization extension.
 */
void dequantizeMeshData(CesiumGltf::Model& model);
} // namespace CesiumGltfReader
