#include "../src/FindMinMax.h"
#include <catch2/catch.hpp>
#include <cstdint>

TEST_CASE("TestFindMinMax.cpp") {
    const std::vector<float> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    SECTION("1 channel has minMax of [1,9]") {
        const auto [min, max] = findMinMaxValues(numbers, 1);
        REQUIRE(min == std::vector<double>{1});
        REQUIRE(max == std::vector<double>{9});
    }

    SECTION("3 channel has minMax of [{1,2,3}, {7,8,9}]") {
        const auto [min, max] = findMinMaxValues(numbers, 3);
        REQUIRE(min == std::vector<double>{1, 2, 3});
        REQUIRE(max == std::vector<double>{7, 8, 9});
    }

    SECTION("Throws on emptyArray") {
        const std::vector<std::uint8_t> emptyArray {};
        REQUIRE_THROWS(findMinMaxValues(emptyArray, 1));
    }

    SECTION("Throws on non-positive channelAmount") {
        REQUIRE_THROWS(findMinMaxValues(numbers, 0));
    }

    SECTION("Throws on non evenly divisible array ") {
        // 9 % 2 -> 1
        REQUIRE_THROWS(findMinMaxValues(numbers, 2));
    }
}
