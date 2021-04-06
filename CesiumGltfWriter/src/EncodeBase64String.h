#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace CesiumGltf {
[[nodiscard]] std::string
encodeAsBase64String(const std::vector<std::byte>& data) noexcept;
}
