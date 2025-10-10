#pragma once

#include <string>
#include <string_view>
#include <vector>

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

  /**
   * @brief Trims whitespace (spaces and tabs) from the start and end of a
   * string view.
   */
  static std::string_view trimWhitespace(const std::string_view& s);

  /**
   * @brief Options that control the behavior of \ref splitOnCharacter.
   */
  struct SplitOptions {
    /**
     * @brief If this option is specified, whitespace (spaces and tabs) will be
     * trimmed from each part before it is added to the result.
     */
    bool trimWhitespace{true};

    /**
     * @brief If this option is specified, empty parts will be omitted from the
     * result.
     *
     * For example, splitting the string ",a,,b," on ',' would yield the parts
     * "a" and "b" if this option is specified, but it would yield "", "a", "",
     * "b", and "" if this option is not specified.
     */
    bool omitEmptyParts{true};
  };

  /**
   * @brief Splits a string view on a specified character.
   *
   * @param s The string view to split.
   * @param separator The character to split on.
   * @param options Options that control the behavior of the split.
   * @return A vector of string views representing the parts of the split
   * string.
   */
  static std::vector<std::string_view> splitOnCharacter(
      const std::string_view& s,
      char separator,
      const SplitOptions& options =
          SplitOptions{.trimWhitespace = true, .omitEmptyParts = true});
};

} // namespace CesiumUtility
