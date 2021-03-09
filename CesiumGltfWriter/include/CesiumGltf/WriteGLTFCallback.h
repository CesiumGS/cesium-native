#pragma once
#include <cstdint>
#include <functional>
#include <string_view>

namespace CesiumGltf {
using WriteGLTFCallback =
    const std::function<void(std::string_view, const std::vector<std::byte>&)>;
inline void noopGltfWriter(std::string_view, const std::vector<std::byte>&) {}
} // namespace CesiumGltf