#include "catch2/catch.hpp"
#include <CesiumGltf/JsonValue.h>
#include <limits>
#include <cmath>
#include <cstdint>

using namespace CesiumGltf;

TEST_CASE("JsonValue::getNumber refuses to cast if data loss would occur") {
    SECTION("2^64 - 1 cannot be converted back to a double") {
        auto value = JsonValue(std::numeric_limits<std::uint64_t>::max());
        REQUIRE_THROWS(value.getSafeNumber<double>());
    }
    
    SECTION("-2^64 cannot be converted back to a double") {
        // -9223372036854775807L cannot be represented exactly as a double
        auto value = JsonValue(std::numeric_limits<std::int64_t>::min() + 1);
        REQUIRE_THROWS(value.getSafeNumber<double>());
    }
    
    SECTION("1024.0 cannot be converted back to a std::uint8_t") {
        auto value = JsonValue(1024.0);
        REQUIRE_THROWS(value.getSafeNumber<std::uint8_t>());
    }

    SECTION("1.5 cannot be converted back to a std::uint16_t") {
        auto value = JsonValue(1.5);
        REQUIRE_THROWS(value.getSafeNumber<std::uint16_t>());
    }
}

TEST_CASE("JsonValue::getNumber casts if no data loss would occur") {
    SECTION("255 can be cast to std::uint8_t") {
        auto value = JsonValue(255.0);
        REQUIRE(std::uint8_t(255.0) == value.getSafeNumber<std::uint8_t>());
    }
}