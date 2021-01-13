#pragma once

namespace CesiumGltf {
    struct ModelReaderResult;
    struct Model;

    void decodeDraco(ModelReaderResult& readModel);
    void removeDracoExtensionsAndClearBuffers(CesiumGltf::Model& model);
}
