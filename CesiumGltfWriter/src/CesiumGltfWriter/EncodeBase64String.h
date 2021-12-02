#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace CesiumGltfWriter {
[[nodiscard]] std::string
encodeAsBase64String(const std::vector<std::byte>& data) noexcept;
}
