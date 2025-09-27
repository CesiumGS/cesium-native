#include <CesiumUtility/StringHelpers.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace CesiumUtility {

/*static*/ std::string StringHelpers::toStringUtf8(const std::u8string& s) {
  return std::string(reinterpret_cast<const char*>(s.data()), s.size());
}

/*static*/ std::string_view
StringHelpers::trimWhitespace(const std::string_view& s) {
  size_t end = s.find_last_not_of(" \t");
  if (end == std::string::npos)
    return {};

  std::string_view trimmedRight = s.substr(0, end + 1);
  size_t start = trimmedRight.find_first_not_of(" \t");
  if (start == std::string::npos)
    return {};

  return trimmedRight.substr(start);
}

/*static*/ std::vector<std::string_view> StringHelpers::splitOnCharacter(
    const std::string_view& s,
    char separator,
    const SplitOptions& options) {
  std::vector<std::string_view> result;
  if (s.empty())
    return result;

  size_t start = 0;

  auto addPart = [&](size_t end) {
    std::string_view trimmed = s.substr(start, end - start);
    if (options.trimWhitespace) {
      trimmed = trimWhitespace(trimmed);
    }
    if (!trimmed.empty() || !options.omitEmptyParts)
      result.emplace_back(std::move(trimmed));
  };

  for (size_t i = 0, length = s.size(); i < length; ++i) {
    if (s[i] == separator) {
      addPart(i);
      start = i + 1;
    }
  }

  addPart(s.size());

  return result;
}

} // namespace CesiumUtility
