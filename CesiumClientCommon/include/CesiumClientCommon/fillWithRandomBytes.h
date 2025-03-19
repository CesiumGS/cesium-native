#pragma once

#include <cstdint>
#include <span>

namespace CesiumClientCommon {

/**
 * @brief Fills the provided buffer with random bytes using a cryptographically
 * secure psuedorandom number generator (CSPRNG).
 *
 * @param buffer The buffer to fill with bytes.
 * @exception std::runtime_error Throws a runtime error if the call to the
 * CSPRNG fails.
 */
void fillWithRandomBytes(const std::span<uint8_t>& buffer);

} // namespace CesiumClientCommon
