#include <CesiumUtility/joinToString.h>

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

using namespace CesiumUtility;

TEST_CASE("joinToString") {
  SECTION("joins vector with non-empty elements") {
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

  SECTION("joins vector with empty elements") {
    CHECK(
        joinToString(std::vector<std::string>{"", "aa", ""}, "--") == "--aa--");
    CHECK(joinToString(std::vector<std::string>{"", ""}, "--") == "--");
  }

  SECTION("handles single-element vector") {
    CHECK(joinToString(std::vector<std::string>{"test"}, "--") == "test");
    CHECK(joinToString(std::vector<std::string>{""}, "--") == "");
  }

  SECTION("handles empty vector") {
    CHECK(joinToString(std::vector<std::string>{}, "--") == "");
  }
}
