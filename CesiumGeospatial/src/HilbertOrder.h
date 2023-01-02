#pragma once

#include <cstdint>

namespace CesiumGeospatial {

class HilbertOrder {
public:
  static uint64_t encode2D(uint32_t level, uint32_t x, uint32_t y);
};

} // namespace CesiumGeospatial
