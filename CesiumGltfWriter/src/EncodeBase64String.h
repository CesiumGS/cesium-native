#pragma once
#include <base64_encode.hpp>
#include <vector>
#include <cstdint>

[[nodiscard]] std::string encodeAsBase64String(const std::vector<std::uint8_t>& data) noexcept {
    return base64_encode(data.data(), data.size());
}