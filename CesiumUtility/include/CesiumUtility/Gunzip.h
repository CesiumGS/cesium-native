#pragma once
#include <gsl/span>

#include <vector>

namespace CesiumUtility {
extern bool isGzip(const gsl::span<const std::byte>& data);
/**
 * Gunzip data. If successful, it will return true and the result will be in the
 * provided vector.
 */
extern bool
gunzip(const gsl::span<const std::byte>& data, std::vector<std::byte>& out);
} // namespace CesiumUtility
