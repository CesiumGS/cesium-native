#pragma once

namespace CesiumGltf {

struct ModelWriterResult;
class GltfWriter;

void decodeDataUrls(
    const GltfWriter& writer,
    ModelWriterResult& writeModel,
    bool clearDecodedDataUrls);
} // namespace CesiumGltf
