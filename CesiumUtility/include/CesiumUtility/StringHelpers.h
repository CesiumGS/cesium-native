#pragma once

#include <string>

namespace CesiumUtility {

/**
 * @brief Helper functions for working with strings.
 */
class StringHelpers {
public:
  /**
   * @brief Converts a `u8string` to a `string` without changing its encoding.
   * The output string is encoded in UTF-8, just like the input.
   *
   * @param s The `std::u8string`.
   * @return The equivalent `std::string`.
   */
  static std::string toStringUtf8(const std::u8string& s);
};

} // namespace CesiumUtility
