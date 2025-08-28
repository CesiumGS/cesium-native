#pragma once

namespace CesiumGltfReader {
struct GltfReaderResult;

void decodeSpz(GltfReaderResult& readGltf);
bool hasSpzExtension(GltfReaderResult& readGltf);
} // namespace CesiumGltfReader
