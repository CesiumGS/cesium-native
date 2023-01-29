#define _CRT_SECURE_NO_WARNINGS
#include "InternalTimegm.h"

#include <catch2/catch.hpp>

using namespace CesiumAsync;

TEST_CASE("Test custom timegm() method") {
  SECTION("test current time") {
    std::time_t currentTime = std::time(nullptr);
    std::tm* gmt = std::gmtime(&currentTime);
    REQUIRE(internalTimegm(gmt) == currentTime);
  }
}
