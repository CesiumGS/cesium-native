#pragma once

#include <cstddef>

namespace CesiumUtility {

struct Hash {
  static std::size_t combine(std::size_t first, std::size_t second);
};

} // namespace CesiumUtility
