#pragma once

#include <algorithm>
#include <cctype>
#include <string_view>

namespace CesiumI3SReader {

/**
 * @brief Case-insensitive equality comparison for two string views.
 *
 * Performs a zero-allocation, character-by-character comparison using
 * `std::tolower`. Used when matching JSON keys or enum strings that may vary
 * in capitalisation across I3S service implementations.
 */
inline bool iequal(std::string_view a, std::string_view b) noexcept {
  if (a.size() != b.size())
    return false;
  for (std::size_t i = 0; i < a.size(); ++i)
    if (std::tolower(static_cast<unsigned char>(a[i])) !=
        std::tolower(static_cast<unsigned char>(b[i])))
      return false;
  return true;
}

} // namespace CesiumI3SReader
