#pragma once
#include <gsl/span>

#include <vector>

namespace CesiumUtility {
static bool isGzip(const gsl::span<const std::byte>& data) {
  if (data.size() < 3) {
    return false;
  }
  return data[0] == std::byte{31} && data[1] == std::byte{139};
}

/**
 * Gunzip data. If successful, it will return true and the result will be in the
 * provided vector.
 */
extern bool
gunzip(const gsl::span<const std::byte>& data, std::vector<std::byte>& out);
} // namespace CesiumUtility
