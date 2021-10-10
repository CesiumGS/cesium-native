#include <CesiumUtility/JsonValue.h>

#include <catch2/catch.hpp>

#include <cstdint>
#include <limits>

using namespace CesiumUtility;

TEST_CASE("JsonValue turns NaN / inf floating point values into null") {
  CHECK(JsonValue(std::numeric_limits<float>::quiet_NaN()).isNull());
  CHECK(JsonValue(std::numeric_limits<double>::quiet_NaN()).isNull());
  CHECK(JsonValue(std::numeric_limits<float>::infinity()).isNull());
  CHECK(JsonValue(std::numeric_limits<double>::infinity()).isNull());
  CHECK(JsonValue(-std::numeric_limits<float>::infinity()).isNull());
  CHECK(JsonValue(-std::numeric_limits<double>::infinity()).isNull());
}

TEST_CASE(
    "JsonValue does not have precision loss when storing / retreving numbers") {
  const auto int64Max = std::numeric_limits<std::int64_t>::max();
  const auto int64Min = std::numeric_limits<std::int64_t>::min();
  REQUIRE(JsonValue(int64Max).getInt64() == int64Max);
  REQUIRE(JsonValue(int64Min).getInt64() == int64Min);

  const auto uint64Max = std::numeric_limits<std::uint64_t>::max();
  const auto uint64Min = std::numeric_limits<std::uint64_t>::min();
  REQUIRE(JsonValue(uint64Max).getUint64() == uint64Max);
  REQUIRE(JsonValue(uint64Min).getUint64() == uint64Min);

  const auto doubleMax = std::numeric_limits<double>::max();
  const auto doubleMin = std::numeric_limits<double>::min();
  REQUIRE(JsonValue(doubleMax).getDouble() == doubleMax);
  REQUIRE(JsonValue(doubleMin).getDouble() == doubleMin);

  const auto floatMax = std::numeric_limits<float>::max();
  const auto floatMin = std::numeric_limits<float>::min();
  REQUIRE(JsonValue(floatMax).getDouble() == floatMax);
  REQUIRE(JsonValue(floatMin).getDouble() == floatMin);
}

TEST_CASE("JsonValue::getSafeNumber() throws if narrowing conversion error "
          "would occur") {
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

TEST_CASE("JsonValue::getSafeNumberOrDefault() returns default if narrowing "
          "conversion error would occur") {
  SECTION("2^64 - 1 cannot be converted back to a double") {
    auto value = JsonValue(std::numeric_limits<std::uint64_t>::max());
    REQUIRE(value.getSafeNumberOrDefault<double>(1995));
  }

  SECTION("-2^64 cannot be converted back to a double") {
    // -9223372036854775807L cannot be represented exactly as a double
    auto value = JsonValue(std::numeric_limits<std::int64_t>::min() + 1);
    REQUIRE(value.getSafeNumberOrDefault<double>(-1995) == -1995);
  }

  SECTION("1024.0 cannot be converted back to a std::uint8_t") {
    auto value = JsonValue(1024.0);
    REQUIRE(value.getSafeNumberOrDefault<std::uint8_t>(255) == 255);
  }

  SECTION("1.5 cannot be converted back to a std::uint16_t") {
    auto value = JsonValue(1.5);
    REQUIRE(value.getSafeNumberOrDefault<std::uint16_t>(365) == 365);
  }
}
