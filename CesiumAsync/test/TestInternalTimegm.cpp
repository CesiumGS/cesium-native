// NOLINTNEXTLINE(bugprone-reserved-identifier, cert-dcl37-c, cert-dcl51-cpp)
#define _CRT_SECURE_NO_WARNINGS
#include "InternalTimegm.h"

#include <catch2/catch_test_macros.hpp>

#include <ctime>

using namespace CesiumAsync;

TEST_CASE("Test custom timegm() method") {
  SECTION("test current time") {
    std::time_t currentTime = std::time(nullptr);
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    std::tm* gmt = std::gmtime(&currentTime);
    REQUIRE(internalTimegm(gmt) == currentTime);
  }
}
