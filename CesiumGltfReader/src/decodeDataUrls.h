#pragma once

namespace CesiumGltf {

struct ModelReaderResult;
struct ReadModelOptions;
class GltfReader;

void decodeDataUrls(
    const GltfReader& reader,
    ModelReaderResult& readModel,
    const ReadModelOptions& options);
} // namespace CesiumGltf
