#pragma once

namespace CesiumGltf {

struct ModelReaderResult;
class GltfReader;

void decodeDataUrls(
    const GltfReader& reader,
    ModelReaderResult& readModel,
    bool clearDecodedDataUrls);
} // namespace CesiumGltf
