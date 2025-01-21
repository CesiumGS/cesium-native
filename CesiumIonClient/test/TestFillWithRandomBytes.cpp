#include "../src/fillWithRandomBytes.h"

#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

using namespace CesiumIonClient;

TEST_CASE("fillWithRandomBytes") {
  std::vector<size_t> sizes = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  for (size_t i = 0; i < sizes.size(); ++i) {
    size_t size = sizes[i];

    // Allocate an extra byte to make sure we don't overflow the buffer
    std::vector<uint8_t> buffer(size + 1);
    std::span<uint8_t> bufferSpan(buffer.data(), size);

    fillWithRandomBytes(bufferSpan);

    // Make sure the rest are non-zeros
    for (size_t j = 0; j < size; ++j) {
      // In the unlikely event the value is zero, generate some new random
      // values. Repeat this up to 10 times. The chances that the random value
      // at a particular position is zero ten times in a row is vanishingly
      // small. We don't care if previous positions get a zero on regeneration,
      // we're just trying to test for the possibility of an off-by-one error
      // making a particular position _always_ zero.
      for (int k = 0; buffer[j] == 0 && k < 10; ++k)
        fillWithRandomBytes(bufferSpan);
      CHECK(buffer[j] != 0);
    }

    // Make sure the last byte was not overwritten
    CHECK(buffer[size] == 0);
  }
}
