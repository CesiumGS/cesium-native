#pragma once
#include <cstdint>
#include <functional>
#include <string_view>

namespace CesiumGltf {

/** Callback for glTF / GLB writing **/
using WriteGLTFCallback =
    const std::function<void(std::string_view, const std::vector<std::byte>&)>;

/** Default no-op callback for glTF / GLB writing */
inline void
noopGltfWriter(std::string_view, const std::vector<std::byte>&) noexcept {}
} // namespace CesiumGltf
