#include "HilbertOrder.h"

#include <CesiumUtility/Assert.h>

#include <cstdint>

using namespace CesiumGeospatial;

namespace {

void rotate(uint32_t n, uint32_t& x, uint32_t& y, bool rx, bool ry) {
  if (ry) {
    return;
  }

  if (rx) {
    x = n - 1 - x;
    y = n - 1 - y;
  }

  uint32_t t = x;
  x = y;
  y = t;
}

} // namespace

/*static*/ uint64_t
HilbertOrder::encode2D(uint32_t level, uint32_t x, uint32_t y) {
  uint32_t n = 1U << level;

  CESIUM_ASSERT(x < n && y < n);

  uint64_t index = 0;

  for (uint64_t s = n >> 1; s > 0; s >>= 1) {
    bool rx = (x & s) > 0;
    bool ry = (y & s) > 0;

    index += ((3ULL * uint64_t(rx)) ^ uint64_t(ry)) * s * s;
    rotate(n, x, y, rx, ry);
  }

  return index;
}
