#include "CesiumGltf/PropertyView.h"

#include <catch2/catch.hpp>

using namespace CesiumGltf;
using namespace CesiumUtility;

TEST_CASE("Constructs empty PropertyView") {
  PropertyView<uint8_t> view;
  REQUIRE(view.arrayCount() == 0);
  REQUIRE(!view.normalized());
  REQUIRE(!view.offset());
  REQUIRE(!view.scale());
  REQUIRE(!view.max());
  REQUIRE(!view.min());
  REQUIRE(!view.required());
  REQUIRE(!view.noData());
  REQUIRE(!view.defaultValue());
}

TEST_CASE("Boolean PropertyView") {
  SECTION("Ignores non-applicable properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;
    classProperty.normalized = true;
    classProperty.offset = JsonValue(true);
    classProperty.scale = JsonValue(true);
    classProperty.max = JsonValue(true);
    classProperty.min = JsonValue(true);
    classProperty.noData = JsonValue(true);

    PropertyView<bool> view(classProperty);
    REQUIRE(!view.normalized());
    REQUIRE(!view.offset());
    REQUIRE(!view.scale());
    REQUIRE(!view.max());
    REQUIRE(!view.min());
    REQUIRE(!view.noData());
  }

  SECTION("Ignores count (array is false)") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;
    classProperty.count = 5;

    PropertyView<bool> view(classProperty);
    REQUIRE(view.arrayCount() == 0);
  }

  SECTION("Constructs with defaultProperty") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;
    classProperty.required = false;
    classProperty.defaultProperty = JsonValue(false);

    PropertyView<bool> view(classProperty);
    REQUIRE(!view.required());
    REQUIRE(view.defaultValue());
    REQUIRE(!*view.defaultValue());
  }

  SECTION("Ignores defaultProperty when required is true") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;
    classProperty.required = true;
    classProperty.defaultProperty = JsonValue(false);

    PropertyView<bool> view(classProperty);
    REQUIRE(view.required());
    REQUIRE(!view.defaultValue());
  }

  SECTION("Returns nullopt for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;
    classProperty.defaultProperty = JsonValue(1);

    PropertyView<bool> view(classProperty);
    REQUIRE(!view.required());
    REQUIRE(!view.defaultValue());
  }
}

TEST_CASE("Scalar PropertyView") {
  SECTION("Constructs with non-optional properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.normalized = true;
    classProperty.required = true;

    PropertyView<uint8_t> view(classProperty);
    REQUIRE(view.normalized());
    REQUIRE(view.required());

    REQUIRE(view.arrayCount() == 0);
    REQUIRE(!view.offset());
    REQUIRE(!view.scale());
    REQUIRE(!view.max());
    REQUIRE(!view.min());
    REQUIRE(!view.noData());
    REQUIRE(!view.defaultValue());
  }

  SECTION("Ignores count (array is false)") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.count = 5;

    PropertyView<uint8_t> view(classProperty);
    REQUIRE(view.arrayCount() == 0);
  }

  SECTION("Constructs with offset, scale, max, and min") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::INT16;
    classProperty.offset = JsonValue(5);
    classProperty.scale = JsonValue(2);
    classProperty.max = JsonValue(10);
    classProperty.min = JsonValue(-10);

    PropertyView<int16_t> view(classProperty);
    REQUIRE(view.offset());
    REQUIRE(view.scale());
    REQUIRE(view.max());
    REQUIRE(view.min());

    REQUIRE(*view.offset() == 5);
    REQUIRE(*view.scale() == 2);
    REQUIRE(*view.max() == 10);
    REQUIRE(*view.min() == -10);
  }

  SECTION("Returns nullopt for out-of-bounds offset, scale, max, and min") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.offset = JsonValue(129);
    classProperty.scale = JsonValue(255);
    classProperty.max = JsonValue(1000);
    classProperty.min = JsonValue(-1000);

    PropertyView<int8_t> view(classProperty);
    REQUIRE(!view.offset());
    REQUIRE(!view.scale());
    REQUIRE(!view.max());
    REQUIRE(!view.min());
  }

  SECTION("Rounds numbers where possible") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.offset = JsonValue(-5.5);
    classProperty.scale = JsonValue(2.4);
    classProperty.max = JsonValue(150);
    classProperty.min = JsonValue(-150);

    PropertyView<int8_t> view(classProperty);
    REQUIRE(view.offset());
    REQUIRE(view.scale());
    REQUIRE(!view.max());
    REQUIRE(!view.min());

    REQUIRE(*view.offset() == -5);
    REQUIRE(*view.scale() == 2);
  }

  SECTION("Returns nullopt for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.offset = JsonValue("10");
    classProperty.scale = JsonValue(false);
    classProperty.max = JsonValue(JsonValue::Array());
    classProperty.min = JsonValue(JsonValue::Array{10});

    PropertyView<int8_t> view(classProperty);
    REQUIRE(!view.offset());
    REQUIRE(!view.scale());
    REQUIRE(!view.max());
    REQUIRE(!view.min());
  }

  SECTION("Constructs with noData and defaultProperty") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.required = false;
    classProperty.noData = JsonValue(0);
    classProperty.defaultProperty = JsonValue(1);

    PropertyView<uint8_t> view(classProperty);
    REQUIRE(!view.required());
    REQUIRE(view.noData());
    REQUIRE(view.defaultValue());

    REQUIRE(*view.noData() == 0);
    REQUIRE(*view.defaultValue() == 1);
  }

  SECTION("Ignores noData and defaultProperty when required is true") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.required = true;
    classProperty.noData = JsonValue(0);
    classProperty.defaultProperty = JsonValue(1);

    PropertyView<uint8_t> view(classProperty);
    REQUIRE(view.required());
    REQUIRE(!view.noData());
    REQUIRE(!view.defaultValue());
  }
}

TEST_CASE("String PropertyView") {
  SECTION("Ignores non-applicable properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.normalized = true;
    classProperty.offset = JsonValue("test");
    classProperty.scale = JsonValue("test");
    classProperty.max = JsonValue("test");
    classProperty.min = JsonValue("test");

    PropertyView<std::string_view> view(classProperty);
    REQUIRE(!view.normalized());
    REQUIRE(!view.offset());
    REQUIRE(!view.scale());
    REQUIRE(!view.max());
    REQUIRE(!view.min());
  }

  SECTION("Ignores count (array is false)") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.count = 5;

    PropertyView<std::string_view> view(classProperty);
    REQUIRE(view.arrayCount() == 0);
  }

  SECTION("Constructs with noData and defaultProperty") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.required = false;
    classProperty.noData = JsonValue("null");
    classProperty.defaultProperty = JsonValue("default");

    PropertyView<std::string_view> view(classProperty);
    REQUIRE(!view.required());
    REQUIRE(view.noData());
    REQUIRE(view.defaultValue());

    REQUIRE(*view.noData() == "null");
    REQUIRE(*view.defaultValue() == "default");
  }

  SECTION("Ignores noData and defaultProperty when required is true") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.required = true;
    classProperty.noData = JsonValue("null");
    classProperty.defaultProperty = JsonValue("default");

    PropertyView<std::string_view> view(classProperty);
    REQUIRE(view.required());
    REQUIRE(!view.noData());
    REQUIRE(!view.defaultValue());
  }

  SECTION("Returns nullopt for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.noData = JsonValue::Array{JsonValue("null")};
    classProperty.defaultProperty = JsonValue(true);

    PropertyView<std::string_view> view(classProperty);
    REQUIRE(!view.required());
    REQUIRE(!view.noData());
    REQUIRE(!view.defaultValue());
  }
}

TEST_CASE("Boolean Array PropertyView") {
  SECTION("Ignores non-applicable properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;
    classProperty.normalized = true;
    classProperty.offset = JsonValue::Array{JsonValue(true)};
    classProperty.scale = JsonValue::Array{JsonValue(true)};
    classProperty.max = JsonValue::Array{JsonValue(true)};
    classProperty.min = JsonValue::Array{JsonValue(true)};
    classProperty.noData = JsonValue::Array{JsonValue(true)};

    PropertyView<PropertyArrayView<bool>> view(classProperty);
    REQUIRE(!view.normalized());
    REQUIRE(!view.offset());
    REQUIRE(!view.scale());
    REQUIRE(!view.max());
    REQUIRE(!view.min());
    REQUIRE(!view.noData());
  }

  SECTION("Constructs with count") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;
    classProperty.array = true;
    classProperty.count = 5;

    PropertyView<PropertyArrayView<bool>> view(classProperty);
    REQUIRE(view.arrayCount() == 5);
  }

  SECTION("Constructs with defaultProperty") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;
    classProperty.array = true;
    classProperty.required = false;
    classProperty.defaultProperty =
        JsonValue::Array{JsonValue(false), JsonValue(true)};

    PropertyView<PropertyArrayView<bool>> view(classProperty);
    REQUIRE(!view.required());
    REQUIRE(view.defaultValue());

    const auto defaultValue = *view.defaultValue();
    REQUIRE(defaultValue.size() == 2);
    REQUIRE(!defaultValue[0]);
    REQUIRE(defaultValue[1]);
  }

  SECTION("Ignores defaultProperty when required is true") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;
    classProperty.array = true;
    classProperty.required = true;
    classProperty.defaultProperty =
        JsonValue::Array{JsonValue(false), JsonValue(true)};

    PropertyView<bool> view(classProperty);
    REQUIRE(view.required());
    REQUIRE(!view.defaultValue());
  }

  SECTION("Returns nullopt for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;
    classProperty.array = true;
    classProperty.defaultProperty = JsonValue(true);

    PropertyView<bool> view(classProperty);
    REQUIRE(!view.required());
    REQUIRE(!view.defaultValue());
  }
}
