#pragma once

namespace CesiumGltf {

struct ModelReaderResult;
struct ReaderContext;

void decodeDataUrls(
    const ReaderContext& context,
    ModelReaderResult& readModel,
    bool clearDecodedDataUrls);
} // namespace CesiumGltf
