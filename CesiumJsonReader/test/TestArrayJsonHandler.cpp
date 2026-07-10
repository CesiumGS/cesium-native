#include <CesiumJsonReader/ArrayJsonHandler.h>
#include <CesiumJsonReader/DoubleJsonHandler.h>
#include <CesiumJsonReader/IntegerJsonHandler.h>
#include <CesiumJsonReader/JsonObjectJsonHandler.h>
#include <CesiumJsonReader/JsonReader.h>
#include <CesiumJsonReader/ObjectJsonHandler.h>
#include <CesiumJsonReader/StringJsonHandler.h>
#include <CesiumUtility/JsonValue.h>

#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

using namespace CesiumJsonReader;
using namespace CesiumUtility;

namespace {

template <typename THandler>
auto readArray(std::string_view json) {
  THandler handler;
  auto bytes = std::span<const std::byte>(
      reinterpret_cast<const std::byte*>(json.data()),
      json.size());
  return JsonReader::readJson(bytes, handler);
}

// A simple struct and handler used to test the primary (non-specialized)
// ArrayJsonHandler template, which is designed to read arrays of objects.
struct TestObject {
  std::string name;
  int32_t value = 0;
};

class TestObjectJsonHandler : public ObjectJsonHandler {
public:
  using ValueType = TestObject;

  TestObjectJsonHandler() noexcept = default;

  void reset(IJsonHandler* pParent, TestObject* pObject) {
    JsonHandler::reset(pParent);
    _pObject = pObject;
  }

  IJsonHandler* readObjectKey(const std::string_view& str) override {
    if (str == "name")
      return this->property("name", _name, _pObject->name);
    if (str == "value")
      return this->property("value", _value, _pObject->value);
    return this->ignoreAndContinue();
  }

private:
  TestObject* _pObject = nullptr;
  StringJsonHandler _name;
  IntegerJsonHandler<int32_t> _value;
};

} // namespace

TEST_CASE("ArrayJsonHandler<double, DoubleJsonHandler>") {
  using Handler = ArrayJsonHandler<double, DoubleJsonHandler>;

  SUBCASE("reads array of doubles") {
    auto result = readArray<Handler>("[1.5, 2.0, -3.25]");
    REQUIRE(result.value.has_value());
    CHECK(result.warnings.empty());
    REQUIRE(result.value->size() == 3);
    CHECK((*result.value)[0] == 1.5);
    CHECK((*result.value)[1] == 2.0);
    CHECK((*result.value)[2] == -3.25);
  }

  SUBCASE("reads integer values as doubles") {
    auto result = readArray<Handler>("[1, 2, 3]");
    REQUIRE(result.value.has_value());
    CHECK(result.warnings.empty());
    REQUIRE(result.value->size() == 3);
    CHECK((*result.value)[0] == 1.0);
    CHECK((*result.value)[1] == 2.0);
    CHECK((*result.value)[2] == 3.0);
  }

  SUBCASE("warns on string element and inserts default value") {
    auto result = readArray<Handler>("[1.0, \"hello\", 3.0]");
    REQUIRE(result.value.has_value());
    REQUIRE(result.warnings.size() == 1);
    CHECK(result.warnings[0].find("string") != std::string::npos);
    REQUIRE(result.value->size() == 3);
    CHECK((*result.value)[0] == 1.0);
    CHECK((*result.value)[1] == 0.0); // default double
    CHECK((*result.value)[2] == 3.0);
  }

  SUBCASE("warns on bool element and inserts default value") {
    auto result = readArray<Handler>("[1.0, true, 3.0]");
    REQUIRE(result.value.has_value());
    REQUIRE(result.warnings.size() == 1);
    CHECK(result.warnings[0].find("bool") != std::string::npos);
    REQUIRE(result.value->size() == 3);
    CHECK((*result.value)[0] == 1.0);
    CHECK((*result.value)[1] == 0.0); // default double
    CHECK((*result.value)[2] == 3.0);
  }

  SUBCASE("warns on null element and inserts default value") {
    auto result = readArray<Handler>("[1.0, null, 3.0]");
    REQUIRE(result.value.has_value());
    REQUIRE(result.warnings.size() == 1);
    CHECK(result.warnings[0].find("null") != std::string::npos);
    REQUIRE(result.value->size() == 3);
    CHECK((*result.value)[0] == 1.0);
    CHECK((*result.value)[1] == 0.0); // default double
    CHECK((*result.value)[2] == 3.0);
  }

  SUBCASE("warns on object element and inserts default value") {
    auto result = readArray<Handler>("[1.0, {\"a\": 1}, 3.0]");
    REQUIRE(result.value.has_value());
    REQUIRE(result.warnings.size() == 1);
    CHECK(result.warnings[0].find("object") != std::string::npos);
    REQUIRE(result.value->size() == 3);
    CHECK((*result.value)[0] == 1.0);
    CHECK((*result.value)[1] == 0.0); // default double
    CHECK((*result.value)[2] == 3.0);
  }

  SUBCASE("skips unexpected elements and can read later valid elements") {
    auto result = readArray<Handler>("[1.0, \"bad\", true, null, 5.0]");
    REQUIRE(result.value.has_value());
    CHECK(result.warnings.size() == 3);
    REQUIRE(result.value->size() == 5);
    CHECK((*result.value)[0] == 1.0);
    CHECK((*result.value)[4] == 5.0);
  }
}

TEST_CASE("ArrayJsonHandler<int32_t, IntegerJsonHandler<int32_t>>") {
  using Handler = ArrayJsonHandler<int32_t, IntegerJsonHandler<int32_t>>;

  SUBCASE("reads array of integers") {
    auto result = readArray<Handler>("[1, 2, -3]");
    REQUIRE(result.value.has_value());
    CHECK(result.warnings.empty());
    REQUIRE(result.value->size() == 3);
    CHECK((*result.value)[0] == 1);
    CHECK((*result.value)[1] == 2);
    CHECK((*result.value)[2] == -3);
  }

  SUBCASE("warns on string element and inserts default value") {
    auto result = readArray<Handler>("[1, \"hello\", 3]");
    REQUIRE(result.value.has_value());
    REQUIRE(result.warnings.size() == 1);
    CHECK(result.warnings[0].find("string") != std::string::npos);
    REQUIRE(result.value->size() == 3);
    CHECK((*result.value)[0] == 1);
    CHECK((*result.value)[1] == 0); // default int
    CHECK((*result.value)[2] == 3);
  }

  SUBCASE("warns on bool element and inserts default value") {
    auto result = readArray<Handler>("[1, true, 3]");
    REQUIRE(result.value.has_value());
    REQUIRE(result.warnings.size() == 1);
    CHECK(result.warnings[0].find("bool") != std::string::npos);
    REQUIRE(result.value->size() == 3);
    CHECK((*result.value)[0] == 1);
    CHECK((*result.value)[1] == 0); // default int
    CHECK((*result.value)[2] == 3);
  }

  SUBCASE("warns on double element and inserts default value") {
    auto result = readArray<Handler>("[1, 2.5, 3]");
    REQUIRE(result.value.has_value());
    REQUIRE(result.warnings.size() == 1);
    CHECK(result.warnings[0].find("double") != std::string::npos);
    REQUIRE(result.value->size() == 3);
    CHECK((*result.value)[0] == 1);
    CHECK((*result.value)[1] == 0); // default int
    CHECK((*result.value)[2] == 3);
  }

  SUBCASE("skips unexpected elements and can read later valid elements") {
    auto result = readArray<Handler>("[1, \"bad\", true, 2.5, 5]");
    REQUIRE(result.value.has_value());
    CHECK(result.warnings.size() == 3);
    REQUIRE(result.value->size() == 5);
    CHECK((*result.value)[0] == 1);
    CHECK((*result.value)[4] == 5);
  }
}

TEST_CASE("ArrayJsonHandler<std::string, StringJsonHandler>") {
  using Handler = ArrayJsonHandler<std::string, StringJsonHandler>;

  SUBCASE("reads array of strings") {
    auto result = readArray<Handler>("[\"hello\", \"world\"]");
    REQUIRE(result.value.has_value());
    CHECK(result.warnings.empty());
    REQUIRE(result.value->size() == 2);
    CHECK((*result.value)[0] == "hello");
    CHECK((*result.value)[1] == "world");
  }

  SUBCASE("warns on integer element and inserts default value") {
    auto result = readArray<Handler>("[\"hello\", 42, \"world\"]");
    REQUIRE(result.value.has_value());
    REQUIRE(result.warnings.size() == 1);
    CHECK(result.warnings[0].find("integer") != std::string::npos);
    REQUIRE(result.value->size() == 3);
    CHECK((*result.value)[0] == "hello");
    CHECK((*result.value)[1] == ""); // default string
    CHECK((*result.value)[2] == "world");
  }

  SUBCASE("warns on bool element and inserts default value") {
    auto result = readArray<Handler>("[\"hello\", true, \"world\"]");
    REQUIRE(result.value.has_value());
    REQUIRE(result.warnings.size() == 1);
    CHECK(result.warnings[0].find("bool") != std::string::npos);
    REQUIRE(result.value->size() == 3);
    CHECK((*result.value)[0] == "hello");
    CHECK((*result.value)[1] == ""); // default string
    CHECK((*result.value)[2] == "world");
  }

  SUBCASE("warns on double element and inserts default value") {
    auto result = readArray<Handler>("[\"hello\", 1.5, \"world\"]");
    REQUIRE(result.value.has_value());
    REQUIRE(result.warnings.size() == 1);
    CHECK(result.warnings[0].find("double") != std::string::npos);
    REQUIRE(result.value->size() == 3);
    CHECK((*result.value)[0] == "hello");
    CHECK((*result.value)[1] == ""); // default string
    CHECK((*result.value)[2] == "world");
  }

  SUBCASE("warns on null element and inserts default value") {
    auto result = readArray<Handler>("[\"hello\", null, \"world\"]");
    REQUIRE(result.value.has_value());
    REQUIRE(result.warnings.size() == 1);
    CHECK(result.warnings[0].find("null") != std::string::npos);
    REQUIRE(result.value->size() == 3);
    CHECK((*result.value)[0] == "hello");
    CHECK((*result.value)[1] == ""); // default string
    CHECK((*result.value)[2] == "world");
  }

  SUBCASE("skips unexpected elements and can read later valid elements") {
    auto result =
        readArray<Handler>("[\"hello\", 42, true, null, \"world\"]");
    REQUIRE(result.value.has_value());
    CHECK(result.warnings.size() == 3);
    REQUIRE(result.value->size() == 5);
    CHECK((*result.value)[0] == "hello");
    CHECK((*result.value)[4] == "world");
  }
}

TEST_CASE("ArrayJsonHandler<JsonValue, JsonObjectJsonHandler>") {
  using Handler = ArrayJsonHandler<JsonValue, JsonObjectJsonHandler>;

  SUBCASE("reads array of mixed JSON values") {
    auto result =
        readArray<Handler>("[1, \"hello\", true, null, 2.5, {\"key\": 42}]");
    REQUIRE(result.value.has_value());
    CHECK(result.warnings.empty());
    REQUIRE(result.value->size() == 6);
    CHECK((*result.value)[0].isInt64());
    CHECK((*result.value)[0].getInt64() == 1);
    CHECK((*result.value)[1].isString());
    CHECK((*result.value)[1].getString() == "hello");
    CHECK((*result.value)[2].isBool());
    CHECK((*result.value)[2].getBool() == true);
    CHECK((*result.value)[3].isNull());
    CHECK((*result.value)[4].isDouble());
    CHECK((*result.value)[4].getDouble() == 2.5);
    CHECK((*result.value)[5].isObject());
  }

  SUBCASE("reads array of booleans") {
    auto result = readArray<Handler>("[true, false, true]");
    REQUIRE(result.value.has_value());
    CHECK(result.warnings.empty());
    REQUIRE(result.value->size() == 3);
    CHECK((*result.value)[0].getBool() == true);
    CHECK((*result.value)[1].getBool() == false);
    CHECK((*result.value)[2].getBool() == true);
  }

  SUBCASE("reads array of strings") {
    auto result = readArray<Handler>("[\"a\", \"b\", \"c\"]");
    REQUIRE(result.value.has_value());
    CHECK(result.warnings.empty());
    REQUIRE(result.value->size() == 3);
    CHECK((*result.value)[0].getString() == "a");
    CHECK((*result.value)[1].getString() == "b");
    CHECK((*result.value)[2].getString() == "c");
  }

  SUBCASE("reads array of numbers") {
    auto result = readArray<Handler>("[1, 2, 3]");
    REQUIRE(result.value.has_value());
    CHECK(result.warnings.empty());
    REQUIRE(result.value->size() == 3);
    CHECK((*result.value)[0].getInt64() == 1);
    CHECK((*result.value)[1].getInt64() == 2);
    CHECK((*result.value)[2].getInt64() == 3);
  }

  SUBCASE("reads array of null values") {
    auto result = readArray<Handler>("[null, null]");
    REQUIRE(result.value.has_value());
    CHECK(result.warnings.empty());
    REQUIRE(result.value->size() == 2);
    CHECK((*result.value)[0].isNull());
    CHECK((*result.value)[1].isNull());
  }

  SUBCASE("reads array of objects") {
    auto result = readArray<Handler>("[{\"a\": 1}, {\"b\": \"two\"}]");
    REQUIRE(result.value.has_value());
    CHECK(result.warnings.empty());
    REQUIRE(result.value->size() == 2);
    CHECK((*result.value)[0].isObject());
    CHECK((*result.value)[1].isObject());
    const auto* pA = (*result.value)[0].getValuePtrForKey("a");
    REQUIRE(pA != nullptr);
    CHECK(pA->getInt64() == 1);
  }

  SUBCASE("mixed types - booleans strings and numbers (original bug case)") {
    // The original bug: keySet can be an array of strings, numbers, or booleans
    auto result = readArray<Handler>("[\"value1\", 42, true]");
    REQUIRE(result.value.has_value());
    CHECK(result.warnings.empty());
    REQUIRE(result.value->size() == 3);
    CHECK((*result.value)[0].getString() == "value1");
    CHECK((*result.value)[1].getInt64() == 42);
    CHECK((*result.value)[2].getBool() == true);
  }
}

TEST_CASE("ArrayJsonHandler<T, THandler> (primary template, objects)") {
  using Handler = ArrayJsonHandler<TestObject, TestObjectJsonHandler>;

  SUBCASE("reads array of objects") {
    auto result = readArray<Handler>(
        "[{\"name\": \"foo\", \"value\": 1},"
        " {\"name\": \"bar\", \"value\": 2}]");
    REQUIRE(result.value.has_value());
    CHECK(result.warnings.empty());
    REQUIRE(result.value->size() == 2);
    CHECK((*result.value)[0].name == "foo");
    CHECK((*result.value)[0].value == 1);
    CHECK((*result.value)[1].name == "bar");
    CHECK((*result.value)[1].value == 2);
  }

  SUBCASE("reads empty array") {
    auto result = readArray<Handler>("[]");
    REQUIRE(result.value.has_value());
    CHECK(result.warnings.empty());
    CHECK(result.value->empty());
  }

  SUBCASE("unknown object keys are ignored without warnings") {
    auto result = readArray<Handler>(
        "[{\"name\": \"foo\", \"unknown\": 99, \"value\": 1}]");
    REQUIRE(result.value.has_value());
    CHECK(result.warnings.empty());
    REQUIRE(result.value->size() == 1);
    CHECK((*result.value)[0].name == "foo");
    CHECK((*result.value)[0].value == 1);
  }

  SUBCASE("warns on string element and inserts default value") {
    auto result = readArray<Handler>(
        "[{\"name\": \"foo\", \"value\": 1}, \"bad\","
        " {\"name\": \"bar\", \"value\": 2}]");
    REQUIRE(result.value.has_value());
    REQUIRE(result.warnings.size() == 1);
    CHECK(result.warnings[0].find("string") != std::string::npos);
    CHECK(result.warnings[0].find("object array") != std::string::npos);
    REQUIRE(result.value->size() == 3);
    CHECK((*result.value)[0].name == "foo");
    // default-constructed element inserted for the invalid "bad" element
    CHECK((*result.value)[1].name == "");
    CHECK((*result.value)[1].value == 0);
    CHECK((*result.value)[2].name == "bar");
  }

  SUBCASE("warns on integer element and inserts default value") {
    auto result = readArray<Handler>(
        "[{\"name\": \"foo\", \"value\": 1}, 42,"
        " {\"name\": \"bar\", \"value\": 2}]");
    REQUIRE(result.value.has_value());
    REQUIRE(result.warnings.size() == 1);
    CHECK(result.warnings[0].find("integer") != std::string::npos);
    CHECK(result.warnings[0].find("object array") != std::string::npos);
    REQUIRE(result.value->size() == 3);
    CHECK((*result.value)[0].name == "foo");
    CHECK((*result.value)[2].name == "bar");
  }

  SUBCASE("warns on bool element and inserts default value") {
    auto result = readArray<Handler>(
        "[{\"name\": \"foo\", \"value\": 1}, true,"
        " {\"name\": \"bar\", \"value\": 2}]");
    REQUIRE(result.value.has_value());
    REQUIRE(result.warnings.size() == 1);
    CHECK(result.warnings[0].find("boolean") != std::string::npos);
    CHECK(result.warnings[0].find("object array") != std::string::npos);
    REQUIRE(result.value->size() == 3);
    CHECK((*result.value)[0].name == "foo");
    CHECK((*result.value)[2].name == "bar");
  }

  SUBCASE("warns on null element and inserts default value") {
    auto result = readArray<Handler>(
        "[{\"name\": \"foo\", \"value\": 1}, null,"
        " {\"name\": \"bar\", \"value\": 2}]");
    REQUIRE(result.value.has_value());
    REQUIRE(result.warnings.size() == 1);
    CHECK(result.warnings[0].find("null") != std::string::npos);
    CHECK(result.warnings[0].find("object array") != std::string::npos);
    REQUIRE(result.value->size() == 3);
    CHECK((*result.value)[0].name == "foo");
    CHECK((*result.value)[2].name == "bar");
  }

  SUBCASE("skips unexpected elements and can read later valid elements") {
    auto result = readArray<Handler>(
        "[{\"name\": \"first\", \"value\": 1},"
        " \"bad\", 42, true, null,"
        " {\"name\": \"last\", \"value\": 5}]");
    REQUIRE(result.value.has_value());
    CHECK(result.warnings.size() == 4);
    REQUIRE(result.value->size() == 6);
    CHECK((*result.value)[0].name == "first");
    CHECK((*result.value)[0].value == 1);
    CHECK((*result.value)[5].name == "last");
    CHECK((*result.value)[5].value == 5);
  }
}
