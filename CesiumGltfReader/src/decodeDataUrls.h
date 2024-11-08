#pragma once

namespace CesiumGltfReader {

struct GltfReaderResult;
struct GltfReaderOptions;
class GltfReader;

void decodeDataUrls(
    GltfReaderResult& readGltf,
    const GltfReaderOptions& clearDecodedDataUrls);
} // namespace CesiumGltfReader
