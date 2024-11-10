#pragma once

#include <cstddef>

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
  static std::size_t combine(std::size_t first, std::size_t second);
};

} // namespace CesiumUtility
