#pragma once

#include <span>

namespace CesiumUtility {
/**
 * @brief This function converts between span types. This function
 * has the same rules with C++ reintepret_cast
 * https://en.cppreference.com/w/cpp/language/reinterpret_cast. So please use it
 * carefully
 */
template <typename To, typename From>
std::span<To> reintepretCastSpan(const std::span<From>& from) noexcept {
  return std::span<To>(
      reinterpret_cast<To*>(from.data()),
      from.size() * sizeof(From) / sizeof(To));
}
} // namespace CesiumUtility
