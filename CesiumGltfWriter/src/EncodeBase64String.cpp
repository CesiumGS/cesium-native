#include "EncodeBase64String.h"
#include <base64_encode.hpp>
std::string CesiumGltf::encodeAsBase64String(const std::vector<std::byte>& data) noexcept {
    return base64_encode(data);
}