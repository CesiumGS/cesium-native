#pragma once

#include <gsl/span>

#include <cstdint>

namespace CesiumIonClient {

void fillWithRandomBytes(const gsl::span<uint8_t>& buffer);

}
