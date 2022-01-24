#pragma once

namespace CesiumGltfReader {

struct GltfReaderResult;
class GltfReader;

void decodeDataUrls(
    const GltfReader& reader,
    GltfReaderResult& readGltf,
    bool clearDecodedDataUrls);
} // namespace CesiumGltfReader
