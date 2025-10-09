#include <CesiumUtility/StringHelpers.h>

#include <doctest/doctest.h>

#include <ostream>
#include <string>
#include <string_view>
#include <vector>

using namespace CesiumUtility;

TEST_CASE("trimWhitespace") {
  SUBCASE("handles empty string view") {
    std::string_view input("");
    CHECK_EQ(StringHelpers::trimWhitespace(input), input);
  }

  SUBCASE("returns as-is for no whitespace") {
    std::string_view input("this is fine");
    CHECK_EQ(StringHelpers::trimWhitespace(input), input);
  }

  SUBCASE("returns empty for all whitespace") {
    std::string_view input("\t\t\t");
    CHECK_EQ(StringHelpers::trimWhitespace(input), std::string_view(""));
  }

  SUBCASE("trims whitespace at front") {
    std::string_view input("\tfront tab");
    CHECK_EQ(StringHelpers::trimWhitespace(input), input.substr(1));

    input = std::string_view("     front spaces");
    CHECK_EQ(StringHelpers::trimWhitespace(input), input.substr(5));
  }

  SUBCASE("trims whitespace at back") {
    std::string_view input("back tab\t");
    CHECK_EQ(StringHelpers::trimWhitespace(input), input.substr(0, 8));

    input = std::string_view("back spaces     ");
  }

  SUBCASE("trims whitespace on both sides") {
    std::string_view input("\t\t a lot of whitespace\t  \t");
    CHECK_EQ(StringHelpers::trimWhitespace(input), input.substr(3, 19));
  }
}

TEST_CASE("splitOnCharacter") {
  SUBCASE("handles empty string") {
    std::vector<std::string_view> result =
        StringHelpers::splitOnCharacter("", ',');
    CHECK_EQ(result.size(), 0);
  }

  SUBCASE("returns single-element vector if separator not present") {
    std::string_view input("this string has no commas!");
    std::vector<std::string_view> result =
        StringHelpers::splitOnCharacter(input, ',');
    REQUIRE_EQ(result.size(), 1);
    CHECK_EQ(result[0], input);
  }

  SUBCASE("separates with default options") {
    std::string_view input("test0, test1,, a bit of whitespace , , ");
    std::vector<std::string_view> result =
        StringHelpers::splitOnCharacter(input, ',');
    REQUIRE_EQ(result.size(), 3);
    CHECK_EQ(result[0], std::string_view("test0"));
    CHECK_EQ(result[1], std::string_view("test1"));
    CHECK_EQ(result[2], std::string_view("a bit of whitespace"));
  }

  SUBCASE("separates with trimWhitespace = false") {
    std::string_view input("test0, test1,, a bit of whitespace , , ");
    std::vector<std::string_view> result = StringHelpers::splitOnCharacter(
        input,
        ',',
        {.trimWhitespace = false, .omitEmptyParts = true});
    REQUIRE_EQ(result.size(), 5);
    CHECK_EQ(result[0], std::string_view("test0"));
    CHECK_EQ(result[1], std::string_view(" test1"));
    CHECK_EQ(result[2], std::string_view(" a bit of whitespace "));
    CHECK_EQ(result[3], std::string_view(" "));
    CHECK_EQ(result[4], std::string_view(" "));
  }

  SUBCASE("separates with omitEmptyParts = false") {
    std::string_view input("test0, test1,, a bit of whitespace , , ");
    std::vector<std::string_view> result = StringHelpers::splitOnCharacter(
        input,
        ',',
        {.trimWhitespace = true, .omitEmptyParts = false});
    REQUIRE_EQ(result.size(), 6);
    CHECK_EQ(result[0], std::string_view("test0"));
    CHECK_EQ(result[1], std::string_view("test1"));
    CHECK_EQ(result[2], std::string_view(""));
    CHECK_EQ(result[3], std::string_view("a bit of whitespace"));
    CHECK_EQ(result[4], std::string_view(""));
    CHECK_EQ(result[5], std::string_view(""));
  }
}
