#include <CesiumUtility/JsonValue.h>

#include <doctest/doctest.h>

#include <cmath>
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

TEST_CASE("JsonValue::getSafeNumber() returns std::nullopt if narrowing "
          "conversion error would occur") {
  SUBCASE("At least one of 2^64 - 1 and 2^64 - 2 cannot be converted back to a "
          "double") {
    auto value1 = JsonValue(std::numeric_limits<std::uint64_t>::max());
    auto value2 = JsonValue(std::numeric_limits<std::uint64_t>::max() - 1);
    bool oneCantConvert = !value1.getSafeNumber<double>().has_value() ||
                          !value2.getSafeNumber<double>().has_value();
    REQUIRE(oneCantConvert);
  }

  SUBCASE("-2^64 cannot be converted back to a double") {
    // -9223372036854775807L cannot be represented exactly as a double
    auto value = JsonValue(std::numeric_limits<std::int64_t>::min() + 1);
    REQUIRE(!value.getSafeNumber<double>().has_value());
  }

  SUBCASE("1024.0 cannot be converted back to a std::uint8_t") {
    auto value = JsonValue(1024.0);
    REQUIRE(!value.getSafeNumber<std::uint8_t>().has_value());
  }

  SUBCASE("1.5 cannot be converted back to a std::uint16_t") {
    auto value = JsonValue(1.5);
    REQUIRE(!value.getSafeNumber<std::uint16_t>().has_value());
  }
}

TEST_CASE("JsonValue::getSafeNumberOrDefault() returns default if narrowing "
          "conversion error would occur") {
  SUBCASE("2^64 - 1 cannot be converted back to a double") {
    auto value = JsonValue(std::numeric_limits<std::uint64_t>::max());
    REQUIRE(value.getSafeNumberOrDefault<double>(1995));
  }

  SUBCASE("-2^64 cannot be converted back to a double") {
    // -9223372036854775807L cannot be represented exactly as a double
    auto value = JsonValue(std::numeric_limits<std::int64_t>::min() + 1);
    REQUIRE(value.getSafeNumberOrDefault<double>(-1995) == -1995);
  }

  SUBCASE("1024.0 cannot be converted back to a std::uint8_t") {
    auto value = JsonValue(1024.0);
    REQUIRE(value.getSafeNumberOrDefault<std::uint8_t>(255) == 255);
  }

  SUBCASE("1.5 cannot be converted back to a std::uint16_t") {
    auto value = JsonValue(1.5);
    REQUIRE(value.getSafeNumberOrDefault<std::uint16_t>(365) == 365);
  }
}

TEST_CASE("JsonValue Equality operator") {
  JsonValue emptyValue;
  JsonValue integerValue(1);
  JsonValue booleanValueTrue(true);
  JsonValue booleanValueTrue2(true);
  JsonValue booleanValueFalse(false);

  CHECK(emptyValue != booleanValueTrue);
  CHECK(integerValue != booleanValueTrue);
  CHECK(booleanValueFalse != booleanValueTrue);

  CHECK(booleanValueTrue2 == booleanValueTrue);
}

TEST_CASE("losslessNarrow") {
  SUBCASE("identity casts") {
    CHECK(losslessNarrow<double, double>(1.0) == 1.0);
    CHECK(losslessNarrow<double, double>(-1.0) == -1.0);
  }

  SUBCASE("float-to-double") {
    CHECK(losslessNarrow<double, float>(1.0f) == 1.0);
    CHECK(losslessNarrow<double, float>(-1.0f) == -1.0);

    std::optional<double> result =
        losslessNarrow<double, float>(std::numeric_limits<float>::quiet_NaN());
    bool nanny = result.has_value() && std::isnan(*result);
    CHECK(nanny);
    CHECK(
        losslessNarrow<double, float>(std::numeric_limits<float>::infinity()) ==
        std::numeric_limits<double>::infinity());
  }

  SUBCASE("double-to-float") {
    CHECK(losslessNarrow<float, double>(1.0) == 1.0f);
    CHECK(losslessNarrow<float, double>(-1.0) == -1.0f);
    CHECK(losslessNarrow<float, double>(1e300) == std::nullopt);
    CHECK(losslessNarrow<float, double>(-1e300) == std::nullopt);
    CHECK(losslessNarrow<float, double>(1.2345678901234) == std::nullopt);

    std::optional<float> result =
        losslessNarrow<float, double>(std::numeric_limits<double>::quiet_NaN());
    bool nanny = result.has_value() && std::isnan(*result);
    CHECK(nanny);
    CHECK(
        losslessNarrow<float, double>(
            std::numeric_limits<double>::infinity()) ==
        std::numeric_limits<float>::infinity());
  }

  SUBCASE("double-to-integer") {
    CHECK(losslessNarrow<int8_t, double>(1.0) == 1);
    CHECK(losslessNarrow<int8_t, double>(-1.0) == -1);
    CHECK(losslessNarrow<int8_t, double>(1.1) == std::nullopt);
    CHECK(losslessNarrow<int8_t, double>(127.0) == 127);
    CHECK(losslessNarrow<int8_t, double>(128.0) == std::nullopt);
    CHECK(losslessNarrow<uint8_t, double>(1.0) == 1);
    CHECK(losslessNarrow<uint8_t, double>(-1.0) == std::nullopt);
    CHECK(losslessNarrow<uint8_t, double>(255.0) == 255);
    CHECK(losslessNarrow<uint8_t, double>(256.0) == std::nullopt);
    CHECK(
        losslessNarrow<uint8_t, double>(
            std::numeric_limits<double>::quiet_NaN()) == std::nullopt);
    CHECK(
        losslessNarrow<uint8_t, double>(
            std::numeric_limits<double>::infinity()) == std::nullopt);
  }

  SUBCASE("integer-to-double") {
    auto isCorrectOrNullOpt = [](std::optional<double> result,
                                 int64_t expected) {
      if (result.has_value()) {
        CHECK(expected == static_cast<int64_t>(result.value()));
      }
    };

    CHECK(losslessNarrow<double, int64_t>(1) == 1.0);
    CHECK(
        losslessNarrow<double, int64_t>(4294967296LL) == 4294967296.0); // 2^32
    isCorrectOrNullOpt(
        losslessNarrow<double, int64_t>(9223372036854775807LL),
        9223372036854775807LL); // 2^63 - 1
    isCorrectOrNullOpt(
        losslessNarrow<double, int64_t>(9223372036854775806LL),
        9223372036854775806LL); // 2^63 - 2
    isCorrectOrNullOpt(
        losslessNarrow<double, int64_t>(9223372036854775805LL),
        9223372036854775805LL); // 2^63 - 3
  }

  SUBCASE("signed integers") {
    CHECK(losslessNarrow<int8_t, int64_t>(1) == 1);
    CHECK(losslessNarrow<int8_t, int64_t>(127) == 127);
    CHECK(losslessNarrow<int8_t, int64_t>(128) == std::nullopt);
    CHECK(losslessNarrow<int8_t, int64_t>(-1) == -1);
    CHECK(losslessNarrow<int8_t, int64_t>(-127) == -127);
    CHECK(losslessNarrow<int8_t, int64_t>(-128) == -128);
    CHECK(losslessNarrow<int8_t, int64_t>(-129) == std::nullopt);
  }

  SUBCASE("unsigned integers") {
    CHECK(losslessNarrow<uint8_t, uint64_t>(1) == 1);
    CHECK(losslessNarrow<uint8_t, uint64_t>(255) == 255);
    CHECK(losslessNarrow<uint8_t, uint64_t>(256) == std::nullopt);
  }

  SUBCASE("signed integers to unsigned integers") {
    CHECK(losslessNarrow<uint8_t, int8_t>(1) == 1.0);
    CHECK(losslessNarrow<uint8_t, int8_t>(-1) == std::nullopt);
    CHECK(losslessNarrow<uint8_t, int8_t>(127) == 127);
    CHECK(losslessNarrow<uint8_t, int64_t>(127) == 127);
    CHECK(losslessNarrow<uint8_t, int64_t>(128) == 128);
    CHECK(losslessNarrow<uint8_t, int64_t>(127) == 127);
    CHECK(losslessNarrow<uint8_t, int64_t>(255) == 255);
    CHECK(losslessNarrow<uint8_t, int64_t>(256) == std::nullopt);
  }

  SUBCASE("unsigned integers to signed integers") {
    CHECK(losslessNarrow<int8_t, uint8_t>(127) == 127);
    CHECK(losslessNarrow<int8_t, uint8_t>(128) == std::nullopt);
    CHECK(losslessNarrow<int16_t, uint8_t>(128) == 128);
  }
}
