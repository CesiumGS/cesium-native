#pragma once

namespace CesiumGltf {

struct ModelReaderResult;
struct JsonReaderContext;

void decodeDataUrls(
    const JsonReaderContext& context,
    ModelReaderResult& readModel,
    bool clearDecodedDataUrls);
} // namespace CesiumGltf
