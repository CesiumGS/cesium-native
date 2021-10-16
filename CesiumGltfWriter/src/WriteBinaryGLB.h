#pragma once

#include <CesiumGltf/Model.h>

#include <cstdint>
#include <string_view>
#include <vector>

enum GLBChunkType { JSON = 0x4E4F534A, BIN = 0x004E4942 };

namespace CesiumGltf {
std::vector<std::byte> writeBinaryGLB(
    const std::vector<std::byte>& binaryChunk,
    const std::string_view& gltfJson);
}
