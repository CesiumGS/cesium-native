#pragma once

namespace CesiumGltfReader {

struct GltfReaderResult;
struct GltfReaderOptions;
class GltfReader;

void decodeDataUrls(
    const GltfReader& reader,
    GltfReaderResult& readGltf,
    const GltfReaderOptions& clearDecodedDataUrls);
} // namespace CesiumGltfReader
