#pragma once

namespace CesiumGltf {
struct Model;
}

namespace CesiumGltfReader {
struct MeshPrimitiveModeOptions;

void convertPrimitiveModes(
    CesiumGltf::Model& model,
    const MeshPrimitiveModeOptions& options);

bool requiresPrimitiveModeConversion(const MeshPrimitiveModeOptions& options);
} // namespace CesiumGltfReader
