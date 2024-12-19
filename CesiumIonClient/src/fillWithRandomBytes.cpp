#include "fillWithRandomBytes.h"

#include <openssl/rand.h>

#include <cstdint>
#include <span>
#include <stdexcept>

namespace CesiumIonClient {

void fillWithRandomBytes(const std::span<uint8_t>& buffer) {
  if (buffer.empty()) {
    return;
  }

  int size = static_cast<int>(buffer.size());
  auto result = RAND_bytes(buffer.data(), size);
  if (result != 1) {
    throw std::runtime_error("Failed to generate random bytes");
  }
}

} // namespace CesiumIonClient
