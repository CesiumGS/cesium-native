#define _CRT_RAND_S
#include "fillWithRandomBytes.h"

#include <cassert>
#include <climits>
#include <ctime>
#include <random>

namespace CesiumIonClient {

// using unsigned short as unsigned char is not supported by
// independent_bits_engine
using random_bytes_engine = std::
    independent_bits_engine<std::default_random_engine, CHAR_BIT, uint16_t>;

uint8_t randomByte() {
  static thread_local auto seed = static_cast<uint16_t>(time(nullptr));
  static thread_local random_bytes_engine engine{seed};
  return static_cast<uint8_t>(engine());
}

void fillWithRandomBytes(const gsl::span<uint8_t>& buffer) {
  std::generate(begin(buffer), end(buffer), std::ref(randomByte));
}

} // namespace CesiumIonClient
