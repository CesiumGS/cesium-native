#pragma once

#include <cstdint>
#include <span>

namespace CesiumIonClient {

void fillWithRandomBytes(const std::span<uint8_t>& buffer);

}
