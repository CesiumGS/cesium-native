#pragma once
#include "WriterLibrary.h"

namespace CesiumGltf {
    CESIUMGLTFWRITER_API enum WriteFlags {
        GLB = 1,
        GLTF = 2,
        EmbedImages = 4,
        EmbedBuffers = 8,
        ExternalBuffers = 16,
        PrettyPrint = 32,
        AutoConvertDataToBase64 = 64
    };

    CESIUMGLTFWRITER_API inline constexpr WriteFlags
    operator|(WriteFlags x, WriteFlags y) {
        return static_cast<WriteFlags>(
            static_cast<int>(x) | static_cast<int>(y));
    }
}