#pragma once
#include "CesiumGltf/Model.h"
#include "CesiumGltf/WriterLibrary.h"
#include <cstdint>
#include <vector>

namespace CesiumGltf {
    CESIUMGLTFWRITER_API enum WriteFlags {
        GLB = 1,
        GLTF = 2,
        EmbedImages = 4,
        EmbedBuffers = 8,
        PrettyPrint = 16,
    };

    CESIUMGLTFWRITER_API inline constexpr WriteFlags
    operator|(WriteFlags x, WriteFlags y) {
        return static_cast<WriteFlags>(
            static_cast<int>(x) | static_cast<int>(y));
    }

    CESIUMGLTFWRITER_API std::vector<std::uint8_t> writeModelToByteArray(
        const Model& model,
        WriteFlags options = WriteFlags::GLB | WriteFlags::EmbedBuffers |
                               WriteFlags::EmbedImages);
}