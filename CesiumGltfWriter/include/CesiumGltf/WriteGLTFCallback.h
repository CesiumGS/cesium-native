#pragma once
#include <string_view>
#include <functional>
#include <cstdint>

namespace CesiumGltf {
    using WriteGLTFCallback = const std::function<void(std::string_view, const std::vector<std::uint8_t>&)>;
    inline void noopGltfWriter(std::string_view, const std::vector<std::uint8_t>&) {}
}