#pragma once

#include <map>
#include <string>

namespace CesiumAsync {
struct CaseInsensitiveCompare {
  bool operator()(const std::string& s1, const std::string& s2) const;
};

/**
 * @brief Http Headers that maps case-insensitive header key with header value.
 */
using HttpHeaders = std::map<std::string, std::string, CaseInsensitiveCompare>;
} // namespace CesiumAsync
