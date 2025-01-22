#pragma once

#include <doctest/doctest.h>

#include <vector>

namespace CesiumNativeTests {

/**
 * @brief Compares the contents of two vectors and returns `true` if the
 * contents are exactly the same.
 */
template <typename T>
bool compareVectors(const std::vector<T>& a, const std::vector<T>& b) {
  if (a.size() != b.size()) {
    return false;
  }

  for (size_t i = 0; i < a.size(); i++) {
    if (a[i] != b[i]) {
      return false;
    }
  }

  return true;
}

/**
 * @brief Compares the contents of two vectors of doubles and returns `true` if
 * the contents are approximately the same, determined by calling @ref
 * doctest::Approx on each pair of values.
 */
template <>
bool compareVectors(
    const std::vector<double>& a,
    const std::vector<double>& b) {
  if (a.size() != b.size()) {
    return false;
  }

  for (size_t i = 0; i < a.size(); i++) {
    if (a[i] != doctest::Approx(b[i])) {
      return false;
    }
  }

  return true;
}

} // namespace CesiumNativeTests