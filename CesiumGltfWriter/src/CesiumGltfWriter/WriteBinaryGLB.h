#pragma once

#include <string_view>
#include <vector>

namespace CesiumGltfWriter {

enum GLBChunkType { JSON = 0x4E4F534A, BIN = 0x004E4942 };

std::vector<std::byte> writeBinaryGLB(
    const std::vector<std::byte>& binaryChunk,
    const std::string_view& gltfJson);
} // namespace CesiumGltfWriter
