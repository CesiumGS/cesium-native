#pragma once
#include "CesiumGltf/Model.h"
#include "CesiumGltf/WriterLibrary.h"
#include <cstddef>
#include <cstdint>
#include <vector>
namespace CesiumGltf {
    CESIUMGLTFWRITER_API enum class WriteOptions : uint32_t {
        GLB = 1,
        GLTF = 2,
        EmbedImages = 4,
        EmbedBuffers = 8,
        PrettyPrint = 16,
    };

    CESIUMGLTFWRITER_API inline constexpr WriteOptions
    operator&(WriteOptions x, WriteOptions y) {
        return static_cast<WriteOptions>(
            static_cast<int>(x) & static_cast<int>(y));
    }

    CESIUMGLTFWRITER_API inline constexpr WriteOptions
    operator|(WriteOptions x, WriteOptions y) {
        return static_cast<WriteOptions>(
            static_cast<int>(x) | static_cast<int>(y));
    }

    CESIUMGLTFWRITER_API inline constexpr WriteOptions
    operator^(WriteOptions x, WriteOptions y) {
        return static_cast<WriteOptions>(
            static_cast<int>(x) ^ static_cast<int>(y));
    }

    CESIUMGLTFWRITER_API inline constexpr WriteOptions
    operator~(WriteOptions x) {
        return static_cast<WriteOptions>(~static_cast<int>(x));
    }

    CESIUMGLTFWRITER_API inline WriteOptions&
    operator&=(WriteOptions& x, WriteOptions y) {
        x = x & y;
        return x;
    }

    CESIUMGLTFWRITER_API inline WriteOptions&
    operator|=(WriteOptions& x, WriteOptions y) {
        x = x | y;
        return x;
    }

    CESIUMGLTFWRITER_API inline WriteOptions&
    operator^=(WriteOptions& x, WriteOptions y) {
        x = x ^ y;
        return x;
    }

    CESIUMGLTFWRITER_API std::vector<std::byte> writeModelToByteArray(
        const Model& model,
        WriteOptions options = WriteOptions::GLB | WriteOptions::EmbedBuffers |
                               WriteOptions::EmbedImages);
}