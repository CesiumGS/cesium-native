#pragma once
#include <vector>
#include <string>
#include <cstdint>
namespace CesiumGltf {
    [[nodiscard]] std::string encodeAsBase64String(const std::vector<std::byte>& data) noexcept;
}