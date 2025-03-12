#pragma once

#include <cstdint>
#include <span>

namespace CesiumClientCommon {

void fillWithRandomBytes(const std::span<uint8_t>& buffer);

}
