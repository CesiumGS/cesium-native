#include "CesiumUtility/Uri.h"

#include <catch2/catch.hpp>

TEST_CASE("Uri::resolve") {
  CHECK(
      CesiumUtility::Uri::resolve("https://www.example.com/", "/page/test") ==
      "https://www.example.com/page/test");
  CHECK(
      CesiumUtility::Uri::resolve("//www.example.com", "/page/test") ==
      "https://www.example.com/page/test");
  CHECK(
      CesiumUtility::Uri::resolve("://www.example.com", "/page/test") ==
      "https://www.example.com/page/test");
}
