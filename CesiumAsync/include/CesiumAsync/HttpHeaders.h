#pragma once

#include <map>
#include <string>

namespace CesiumAsync {

/**
 * @brief A case-insensitive `less-then` string comparison.
 *
 * This can be used as a `Compare` function, for example for a `std::map`.
 * It will compare strings case-insensitively, by converting them to
 * lower-case and comparing the results (leaving the exact behavior for
 * non-ASCII strings unspecified).
 */
struct CaseInsensitiveCompare {
  /** @brief Performs a case-insensitive comparison of the two strings using
   * `std::lexicographical_compare`. */
  bool operator()(const std::string& s1, const std::string& s2) const;
};

/**
 * @brief Http Headers that maps case-insensitive header key with header value.
 */
using HttpHeaders = std::map<std::string, std::string, CaseInsensitiveCompare>;
} // namespace CesiumAsync
