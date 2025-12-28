#include <CesiumUtility/JsonHelpers.h>

#include <doctest/doctest.h>
#include <rapidjson/document.h>

TEST_CASE("JsonHelpers") {
  SUBCASE("getInt64OrDefault works with 32-bit integers") {
    rapidjson::Document json;
    json.Parse(R"({"smallInt": 123456})");

    int64_t value =
        CesiumUtility::JsonHelpers::getInt64OrDefault(json, "smallInt", -1);
    CHECK(value == 123456);
  }

  SUBCASE("getInt64OrDefault works with 64-bit integers") {
    rapidjson::Document json;
    json.Parse(R"({"bigInt": 9223372036854775807})");

    int64_t value =
        CesiumUtility::JsonHelpers::getInt64OrDefault(json, "bigInt", -1);
    CHECK(value == 9223372036854775807);
  }

  SUBCASE("getInt64OrDefault works with negative integers") {
    rapidjson::Document json;
    json.Parse(R"({"value": -123})");

    int64_t value =
        CesiumUtility::JsonHelpers::getInt64OrDefault(json, "value", 0);
    CHECK(value == -123);
  }

  SUBCASE("getUint64OrDefault works with 32-bit integers") {
    rapidjson::Document json;
    json.Parse(R"({"smallInt": 123456})");

    uint64_t value =
        CesiumUtility::JsonHelpers::getUint64OrDefault(json, "smallInt", 0);
    CHECK(value == 123456);
  }

  SUBCASE("getUint64OrDefault works with 64-bit integers") {
    rapidjson::Document json;
    json.Parse(R"({"bigInt": 9223372036854775807})");

    uint64_t value =
        CesiumUtility::JsonHelpers::getUint64OrDefault(json, "bigInt", 0);
    CHECK(value == 9223372036854775807);
  }

  SUBCASE("getUint64OrDefault returns default for negative numbers") {
    rapidjson::Document json;
    json.Parse(R"({"value": -1})");

    uint64_t value =
        CesiumUtility::JsonHelpers::getUint64OrDefault(json, "value", 0);
    CHECK(value == 0);
  }

  SUBCASE("getInt32OrDefault works with 32-bit integers") {
    rapidjson::Document json;
    json.Parse(R"({"smallInt": 123456})");

    int32_t value =
        CesiumUtility::JsonHelpers::getInt32OrDefault(json, "smallInt", -1);
    CHECK(value == 123456);
  }

  SUBCASE("getUint32OrDefault works with 32-bit integers") {
    rapidjson::Document json;
    json.Parse(R"({"smallInt": 123456})");

    uint64_t value =
        CesiumUtility::JsonHelpers::getUint32OrDefault(json, "smallInt", 0);
    CHECK(value == 123456);
  }

  SUBCASE("getInt32OrDefault works with negative integers") {
    rapidjson::Document json;
    json.Parse(R"({"value": -123})");

    int32_t value =
        CesiumUtility::JsonHelpers::getInt32OrDefault(json, "value", 0);
    CHECK(value == -123);
  }

  SUBCASE("getUint32OrDefault returns default for negative numbers") {
    rapidjson::Document json;
    json.Parse(R"({"value": -1})");

    uint32_t value =
        CesiumUtility::JsonHelpers::getUint32OrDefault(json, "value", 0);
    CHECK(value == 0);
  }

  SUBCASE("getInt32OrDefault returns default for 64-bit integers") {
    rapidjson::Document json;
    json.Parse(R"({"bigInt": 9223372036854775807})");

    int32_t value =
        CesiumUtility::JsonHelpers::getInt32OrDefault(json, "bigInt", -1);
    CHECK(value == -1);
  }

  SUBCASE("getUint32OrDefault returns default for 64-bit integers") {
    rapidjson::Document json;
    json.Parse(R"({"bigInt": 9223372036854775807})");

    uint32_t value =
        CesiumUtility::JsonHelpers::getUint32OrDefault(json, "bigInt", 0);
    CHECK(value == 0);
  }
}
