#pragma once

namespace CesiumGltf {
    struct ModelReaderResult;

    void decodeDataUrls(ModelReaderResult& readModel, bool clearDecodedDataUrls);
}
