#pragma once

#include <cstddef>
#include <cstdint>

namespace CesiumUtility {

/**
 * @brief Contains functions for working with hashes.
 *
 */
struct Hash {
  /**
   * @brief Combines two hash values, usually generated using `std::hash`, to
   * form a single hash value.
   *
   * @param first The first hash value.
   * @param second The second hash value.
   * @return A new hash value which is a combination of the two.
   */
  static size_t combine(uint64_t first, uint64_t second);
};

} // namespace CesiumUtility
