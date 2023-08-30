#pragma once

#include <gsl/span>

namespace CesiumIonClient {

void fillWithRandomBytes(const gsl::span<uint8_t>& buffer);

}
