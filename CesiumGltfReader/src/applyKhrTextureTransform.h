#pragma once

namespace CesiumGltf {
struct Model;
}

namespace CesiumGltfReader {

/**
 * @brief Applies the KHR_texture_transform extension to the texture coordinates
 * of a model. This function modifies the model by transforming the UV
 * coordinates of each texture according to the offset, rotation, and scale
 * properties specified by the extension.
 * @post The function will create a copy of the original UV buffer with updated
 * coordinates that reflect the applied transformations and store it in a new
 * buffer view and accessor.
 */
void applyKhrTextureTransform(CesiumGltf::Model& model);
} // namespace CesiumGltfReader
