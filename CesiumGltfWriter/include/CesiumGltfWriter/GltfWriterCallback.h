#pragma once

#include <functional>
#include <string_view>

namespace CesiumGltfWriter {

using GltfWriterCallback =
    const std::function<void(std::string_view, const std::vector<std::byte>&)>;

inline void
noopGltfWriter(std::string_view, const std::vector<std::byte>&) noexcept {}
} // namespace CesiumGltfWriter
