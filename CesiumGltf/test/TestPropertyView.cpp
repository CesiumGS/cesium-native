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
    classProperty.offset = true;
    classProperty.scale = true;
    classProperty.max = true;
    classProperty.min = true;
    classProperty.noData = true;

    PropertyView<bool> view(classProperty);
    REQUIRE(!view.normalized());
    REQUIRE(!view.offset());
    REQUIRE(!view.scale());
    REQUIRE(!view.max());
    REQUIRE(!view.min());
    REQUIRE(!view.noData());
  }

  SECTION("Ignores count") {
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
    classProperty.defaultProperty = false;

    PropertyView<bool> view(classProperty);
    REQUIRE(!view.required());
    REQUIRE(view.defaultValue());
    REQUIRE(!*view.defaultValue());
  }

  SECTION("Ignores defaultProperty when required is true") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;
    classProperty.required = true;
    classProperty.defaultProperty = false;

    PropertyView<bool> view(classProperty);
    REQUIRE(view.required());
    REQUIRE(!view.defaultValue());
  }

  SECTION("Returns nullopt for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;
    classProperty.defaultProperty = 1;

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

  SECTION("Ignores count") {
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
    classProperty.offset = 5;
    classProperty.scale = 2;
    classProperty.max = 10;
    classProperty.min = -10;

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

  SECTION("Returns nullopt for out-of-bounds values") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.offset = 20;
    classProperty.scale = 255;
    classProperty.max = 127;
    classProperty.min = -1000;

    PropertyView<int8_t> view(classProperty);
    REQUIRE(view.offset());
    REQUIRE(view.max());

    REQUIRE(*view.offset() == 20);
    REQUIRE(*view.max() == 127);

    REQUIRE(!view.scale());
    REQUIRE(!view.min());
  }

  SECTION("Returns nullopt for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.offset = "10";
    classProperty.scale = false;
    classProperty.max = 5.5;
    classProperty.min = JsonValue::Array{1};

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
    classProperty.noData = 0;
    classProperty.defaultProperty = 1;

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
    classProperty.noData = 0;
    classProperty.defaultProperty = 1;

    PropertyView<uint8_t> view(classProperty);
    REQUIRE(view.required());
    REQUIRE(!view.noData());
    REQUIRE(!view.defaultValue());
  }
}

TEST_CASE("VecN PropertyView") {
  SECTION("Constructs with non-optional properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::INT16;
    classProperty.normalized = true;
    classProperty.required = true;

    PropertyView<glm::i16vec3> view(classProperty);
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

  SECTION("Ignores count") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::INT16;
    classProperty.count = 5;

    PropertyView<glm::i16vec3> view(classProperty);
    REQUIRE(view.arrayCount() == 0);
  }

  SECTION("Constructs with offset, scale, max, and min") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::INT16;
    classProperty.offset = JsonValue::Array{-1, 1, 2};
    classProperty.scale = JsonValue::Array{2, 1, 3};
    classProperty.max = JsonValue::Array{10, 5, 6};
    classProperty.min = JsonValue::Array{-11, -12, -13};

    PropertyView<glm::i16vec3> view(classProperty);
    REQUIRE(view.offset());
    REQUIRE(view.scale());
    REQUIRE(view.max());
    REQUIRE(view.min());

    REQUIRE(*view.offset() == glm::i16vec3(-1, 1, 2));
    REQUIRE(*view.scale() == glm::i16vec3(2, 1, 3));
    REQUIRE(*view.max() == glm::i16vec3(10, 5, 6));
    REQUIRE(*view.min() == glm::i16vec3(-11, -12, -13));
  }

  SECTION("Returns nullopt for out-of-bounds values") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.offset = JsonValue::Array{-1, 2};
    classProperty.scale = JsonValue::Array{1, 128};
    classProperty.max = JsonValue::Array{10, 5};
    classProperty.min = JsonValue::Array{-200, 0};

    PropertyView<glm::i8vec2> view(classProperty);
    REQUIRE(view.offset());
    REQUIRE(view.max());

    REQUIRE(*view.offset() == glm::i8vec2(-1, 2));
    REQUIRE(*view.max() == glm::i8vec2(10, 5));

    REQUIRE(!view.scale());
    REQUIRE(!view.min());
  }

  SECTION("Returns nullopt for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.offset = "(1, 2, 3)";
    classProperty.scale = 1;
    classProperty.max = JsonValue::Array{10, 20, 30, 40};
    classProperty.min = JsonValue::Array{-10, -1};

    PropertyView<glm::i8vec3> view(classProperty);
    REQUIRE(!view.offset());
    REQUIRE(!view.scale());
    REQUIRE(!view.max());
    REQUIRE(!view.min());
  }

  SECTION("Constructs with noData and defaultProperty") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC4;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.required = false;
    classProperty.noData = JsonValue::Array{0.0f, 0.0f, 0.0f, 0.0f};
    classProperty.defaultProperty = JsonValue::Array{1.0f, 2.0f, 3.0f, 4.0f};

    PropertyView<glm::vec4> view(classProperty);
    REQUIRE(!view.required());
    REQUIRE(view.noData());
    REQUIRE(view.defaultValue());

    REQUIRE(*view.noData() == glm::vec4(0.0f));
    REQUIRE(*view.defaultValue() == glm::vec4(1.0f, 2.0f, 3.0f, 4.0f));
  }

  SECTION("Ignores noData and defaultProperty when required is true") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC4;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.required = true;
    classProperty.noData = JsonValue::Array{0.0f, 0.0f, 0.0f, 0.0f};
    classProperty.defaultProperty = JsonValue::Array{1.0f, 2.0f, 3.0f, 4.0f};

    PropertyView<glm::vec4> view(classProperty);
    REQUIRE(view.required());
    REQUIRE(!view.noData());
    REQUIRE(!view.defaultValue());
  }
}

TEST_CASE("MatN PropertyView") {
  SECTION("Constructs with non-optional properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT3;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.normalized = true;
    classProperty.required = true;

    PropertyView<glm::imat3x3> view(classProperty);
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

  SECTION("Ignores count") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT3;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.count = 5;

    PropertyView<glm::imat3x3> view(classProperty);
    REQUIRE(view.arrayCount() == 0);
  }

  SECTION("Constructs with offset, scale, max, and min") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT3;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    // clang-format off
    classProperty.offset = JsonValue::Array{
      -1,  1, 2,
       3, -1, 4,
      -5, -5, 0};
    classProperty.scale = JsonValue::Array{
      1, 1, 1,
      2, 2, 3,
      3, 4, 5};
    classProperty.max = JsonValue::Array{
      20,  5, 20,
      30, 22, 43,
      37,  1,  8};
    classProperty.min = JsonValue::Array{
      -10, -2, -3,
        0, 20,  4,
        9,  4,  5};
    // clang-format on

    PropertyView<glm::imat3x3> view(classProperty);
    REQUIRE(view.offset());
    REQUIRE(view.scale());
    REQUIRE(view.max());
    REQUIRE(view.min());

    // clang-format off
    glm::imat3x3 expectedOffset(
      -1,  1, 2,
       3, -1, 4,
      -5, -5, 0);
    REQUIRE(*view.offset() == expectedOffset);

    glm::imat3x3 expectedScale(
      1, 1, 1,
      2, 2, 3,
      3, 4, 5);
    REQUIRE(*view.scale() == expectedScale);

    glm::imat3x3 expectedMax(
      20,  5, 20,
      30, 22, 43,
      37,  1,  8);
    REQUIRE(*view.max() == expectedMax);

    glm::imat3x3 expectedMin(
      -10, -2, -3,
        0, 20,  4,
        9,  4,  5);
    REQUIRE(*view.min() == expectedMin);
    // clang-format on
  }

  SECTION("Returns nullopt for out-of-bounds values") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    // clang-format off
    classProperty.offset = JsonValue::Array{
       -1,  2,
      129, -2};
    classProperty.scale = JsonValue::Array{
      1, 7,
      4, 6};
    classProperty.max = JsonValue::Array{
      10, 240,
       1,   8};
    classProperty.min = JsonValue::Array{
      -29, -40,
      -55, -43};
    // clang-format on

    PropertyView<glm::i8mat2x2> view(classProperty);
    REQUIRE(view.scale());
    REQUIRE(view.min());

    // clang-format off
    glm::i8mat2x2 expectedScale(
      1, 7,
      4, 6);
    REQUIRE(*view.scale() == expectedScale);

    glm::i8mat2x2 expectedMin(
      -29, -40,
      -55, -43);
    REQUIRE(*view.min() == expectedMin);
    // clang-format on

    REQUIRE(!view.offset());
    REQUIRE(!view.max());
  }

  SECTION("Returns nullopt for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.offset = "(1, 2, 3, 4)";
    classProperty.scale = 1;
    classProperty.max = JsonValue::Array{10, 20, 30, 40, 50};
    classProperty.min = JsonValue::Array{0, 0};

    PropertyView<glm::i8mat2x2> view(classProperty);
    REQUIRE(!view.offset());
    REQUIRE(!view.scale());
    REQUIRE(!view.max());
    REQUIRE(!view.min());
  }

  SECTION("Constructs with noData and defaultProperty") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.required = false;
    // clang-format off
    classProperty.noData = JsonValue::Array{
      0.0f, 0.0f,
      0.0f, 0.0f};
    classProperty.defaultProperty = JsonValue::Array{
      1.0f, 2.0f,
      3.0f, 4.0f};
    // clang-format on

    PropertyView<glm::mat2> view(classProperty);
    REQUIRE(!view.required());
    REQUIRE(view.noData());
    REQUIRE(view.defaultValue());

    // clang-format off
    glm::mat2 expectedNoData(
      0.0f, 0.0f,
      0.0f, 0.0f);
    REQUIRE(*view.noData() == expectedNoData);

    glm::mat2 expectedDefaultValue(
      1.0f, 2.0f,
      3.0f, 4.0f);
    REQUIRE(*view.defaultValue() == expectedDefaultValue);
  }

  SECTION("Ignores noData and defaultProperty when required is true") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.required = true;
    // clang-format off
    classProperty.noData = JsonValue::Array{
      0.0f, 0.0f,
      0.0f, 0.0f};
    classProperty.defaultProperty = JsonValue::Array{
      1.0f, 2.0f,
      3.0f, 4.0f};
    // clang-format on

    PropertyView<glm::mat2> view(classProperty);
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
    classProperty.offset = "offset";
    classProperty.scale = "scale";
    classProperty.max = "max";
    classProperty.min = "min";

    PropertyView<std::string_view> view(classProperty);
    REQUIRE(!view.normalized());
    REQUIRE(!view.offset());
    REQUIRE(!view.scale());
    REQUIRE(!view.max());
    REQUIRE(!view.min());
  }

  SECTION("Ignores count") {
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
    classProperty.noData = "null";
    classProperty.defaultProperty = "default";

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
    classProperty.noData = "null";
    classProperty.defaultProperty = "default";

    PropertyView<std::string_view> view(classProperty);
    REQUIRE(view.required());
    REQUIRE(!view.noData());
    REQUIRE(!view.defaultValue());
  }

  SECTION("Returns nullopt for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.noData = JsonValue::Array{"null"};
    classProperty.defaultProperty = true;

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
    classProperty.offset = JsonValue::Array{true};
    classProperty.scale = JsonValue::Array{true};
    classProperty.max = JsonValue::Array{true};
    classProperty.min = JsonValue::Array{true};
    classProperty.noData = JsonValue::Array{true};

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
    classProperty.defaultProperty = JsonValue::Array{false, true};

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
    classProperty.defaultProperty = JsonValue::Array{false, true};

    PropertyView<PropertyArrayView<bool>> view(classProperty);
    REQUIRE(view.required());
    REQUIRE(!view.defaultValue());
  }

  SECTION("Returns nullopt for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;
    classProperty.array = true;
    classProperty.defaultProperty = true;

    PropertyView<PropertyArrayView<bool>> view(classProperty);
    REQUIRE(!view.required());
    REQUIRE(!view.defaultValue());
  }
}

TEST_CASE("String Array PropertyView") {
  SECTION("Ignores non-applicable properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.array = true;
    classProperty.normalized = true;
    classProperty.offset = {"offset"};
    classProperty.scale = {"scale"};
    classProperty.max = {"max"};
    classProperty.min = {"min"};

    PropertyView<PropertyArrayView<std::string_view>> view(classProperty);
    REQUIRE(!view.normalized());
    REQUIRE(!view.offset());
    REQUIRE(!view.scale());
    REQUIRE(!view.max());
    REQUIRE(!view.min());
  }

  SECTION("Constructs with count") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.array = true;
    classProperty.count = 5;

    PropertyView<PropertyArrayView<std::string_view>> view(classProperty);
    REQUIRE(view.arrayCount() == classProperty.count);
  }

  SECTION("Constructs with noData and defaultProperty") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.array = true;
    classProperty.required = false;
    classProperty.noData = JsonValue::Array{"null", "0"};
    classProperty.defaultProperty = JsonValue::Array{"default1", "default2"};

    PropertyView<PropertyArrayView<std::string_view>> view(classProperty);
    REQUIRE(!view.required());
    REQUIRE(view.noData());
    REQUIRE(view.defaultValue());

    const auto noData = *view.noData();
    REQUIRE(noData.size() == 2);
    REQUIRE(noData[0] == "null");
    REQUIRE(noData[1] == "0");

    const auto defaultValue = *view.defaultValue();
    REQUIRE(defaultValue.size() == 2);
    REQUIRE(defaultValue[0] == "default1");
    REQUIRE(defaultValue[1] == "default2");
  }

  SECTION("Ignores noData and defaultProperty when required is true") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.required = true;
    classProperty.noData = JsonValue::Array{"null", "0"};
    classProperty.defaultProperty = JsonValue::Array{"default1", "default2"};

    PropertyView<PropertyArrayView<std::string_view>> view(classProperty);
    REQUIRE(view.required());
    REQUIRE(!view.noData());
    REQUIRE(!view.defaultValue());
  }

  SECTION("Returns nullopt for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.noData = JsonValue::Array{"null"};
    classProperty.defaultProperty = true;

    PropertyView<std::string_view> view(classProperty);
    REQUIRE(!view.required());
    REQUIRE(!view.noData());
    REQUIRE(!view.defaultValue());
  }
}
