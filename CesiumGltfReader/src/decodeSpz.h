#pragma once

namespace CesiumGltfReader {
struct GltfReaderResult;
struct SpzDecoderOptions;

void decodeSpz(const SpzDecoderOptions& options, GltfReaderResult& readGltf);
bool hasSpzExtension(GltfReaderResult& readGltf);
} // namespace CesiumGltfReader
