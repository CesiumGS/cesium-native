#pragma once

namespace CesiumGltfReader {
struct GltfReaderResult;
struct SpzDecoderOptions;

void decodeSpz(GltfReaderResult& readGltf, const SpzDecoderOptions& options);
bool hasSpzExtension(GltfReaderResult& readGltf);
} // namespace CesiumGltfReader
