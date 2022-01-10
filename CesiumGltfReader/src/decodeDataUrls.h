#pragma once

namespace CesiumGltfReader {

struct ModelReaderResult;
class GltfReader;

void decodeDataUrls(
    const GltfReader& reader,
    ModelReaderResult& readModel,
    bool clearDecodedDataUrls);
} // namespace CesiumGltfReader
