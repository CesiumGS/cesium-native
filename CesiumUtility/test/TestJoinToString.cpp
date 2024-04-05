#include <CesiumUtility/joinToString.h>

#include <catch2/catch.hpp>

using namespace CesiumUtility;

TEST_CASE("joinToString") {
  CHECK(
      joinToString(std::vector<std::string>{"test", "this"}, "--") ==
      "test--this");
  CHECK(
      joinToString(std::vector<std::string>{"test", "this", "thing"}, " ") ==
      "test this thing");
  CHECK(
      joinToString(std::vector<std::string>{"test", "this", "thing"}, "") ==
      "testthisthing");
  CHECK(joinToString(std::vector<std::string>{"test"}, "--") == "test");
  CHECK(joinToString(std::vector<std::string>{""}, "--") == "");
  CHECK(joinToString(std::vector<std::string>{"", "aa", ""}, "--") == "--aa--");
  CHECK(joinToString(std::vector<std::string>{"", ""}, "--") == "--");
  CHECK(joinToString(std::vector<std::string>{}, "--") == "");
}
