#include <CesiumUtility/joinToString.h>

#include <doctest/doctest.h>

#include <string>
#include <vector>

using namespace CesiumUtility;

TEST_CASE("joinToString") {
  SUBCASE("joins vector with non-empty elements") {
    CHECK(
        joinToString(std::vector<std::string>{"test", "this"}, "--") ==
        "test--this");
    CHECK(
        joinToString(std::vector<std::string>{"test", "this", "thing"}, " ") ==
        "test this thing");
    CHECK(
        joinToString(std::vector<std::string>{"test", "this", "thing"}, "") ==
        "testthisthing");
  }

  SUBCASE("joins vector with empty elements") {
    CHECK(
        joinToString(std::vector<std::string>{"", "aa", ""}, "--") == "--aa--");
    CHECK(joinToString(std::vector<std::string>{"", ""}, "--") == "--");
  }

  SUBCASE("handles single-element vector") {
    CHECK(joinToString(std::vector<std::string>{"test"}, "--") == "test");
    CHECK(joinToString(std::vector<std::string>{""}, "--") == "");
  }

  SUBCASE("handles empty vector") {
    CHECK(joinToString(std::vector<std::string>{}, "--") == "");
  }
}
