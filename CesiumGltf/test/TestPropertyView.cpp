#include "CesiumGltf/PropertyView.h"

#include "CesiumGltf/PropertyValue.h"

#include <catch2/catch.hpp>

using namespace CesiumGltf;
using namespace CesiumUtility;

TEST_CASE("TEST") { TestPropertyTablePropertyView<int32_t, true> normalizedView;
  REQUIRE(normalizedView.normalized());
  REQUIRE(normalizedView.getValue() == 1.0);

  TestPropertyTablePropertyView<int32_t, false> regView;
  REQUIRE(!regView.normalized());
  REQUIRE(regView.getValue() == 1);
}

TEST_CASE("Boolean PropertyView") {
  SECTION("Constructs empty PropertyView") {
    PropertyView<bool> view;
    REQUIRE(view.status() == PropertyViewStatus::ErrorNonexistentProperty);
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

  SECTION("Reports type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;

    PropertyView<bool> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorTypeMismatch);
  }

  SECTION("Reports array type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;
    classProperty.array = true;

    PropertyView<bool> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorArrayTypeMismatch);
  }

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
    REQUIRE(view.status() == PropertyViewStatus::Valid);
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
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.arrayCount() == 0);
  }

  SECTION("Constructs with defaultProperty") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;
    classProperty.required = false;
    classProperty.defaultProperty = false;

    PropertyView<bool> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
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
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.required());
    REQUIRE(!view.defaultValue());
  }

  SECTION("Reports default value invalid type") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;
    classProperty.defaultProperty = 1;

    PropertyView<bool> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);
  }
}

TEST_CASE("Scalar PropertyView") {
  SECTION("Constructs empty PropertyView") {
    PropertyView<uint8_t> view;
    REQUIRE(view.status() == PropertyViewStatus::ErrorNonexistentProperty);
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

  SECTION("Reports type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;

    PropertyView<uint8_t> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorTypeMismatch);
  }

  SECTION("Reports component type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::INT8;

    PropertyView<uint8_t> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SECTION("Reports array type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.array = true;

    PropertyView<uint8_t> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SECTION("Reports invalid normalization") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.normalized = true;

    PropertyView<float> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNormalization);
  }

  SECTION("Constructs with non-optional properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.normalized = true;
    classProperty.required = true;

    PropertyView<uint8_t> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
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
    REQUIRE(view.status() == PropertyViewStatus::Valid);
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
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.offset());
    REQUIRE(view.scale());
    REQUIRE(view.max());
    REQUIRE(view.min());

    REQUIRE(*view.offset() == 5);
    REQUIRE(*view.scale() == 2);
    REQUIRE(*view.max() == 10);
    REQUIRE(*view.min() == -10);
  }

  SECTION("Constructs with noData and defaultProperty") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.required = false;
    classProperty.noData = 0;
    classProperty.defaultProperty = 1;

    PropertyView<uint8_t> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
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
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.required());
    REQUIRE(!view.noData());
    REQUIRE(!view.defaultValue());
  }

  SECTION("Reports errors for out-of-bounds values") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.defaultProperty = 2000;

    PropertyView<int8_t> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    classProperty.noData = -129;
    view = PropertyView<int8_t>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);

    classProperty.min = -1000;
    view = PropertyView<int8_t>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMin);

    classProperty.max = 1000;
    view = PropertyView<int8_t>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMax);

    classProperty.scale = 200;
    view = PropertyView<int8_t>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    classProperty.offset = 1234;
    view = PropertyView<int8_t>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }

  SECTION("Reports errors for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.defaultProperty = JsonValue::Array{1};

    PropertyView<int8_t> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    classProperty.noData = "0";
    view = PropertyView<int8_t>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);

    classProperty.min = -12.7f;
    view = PropertyView<int8_t>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMin);

    classProperty.max = 5.5;
    view = PropertyView<int8_t>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMax);

    classProperty.scale = false;
    view = PropertyView<int8_t>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    classProperty.offset = JsonValue::Array{};
    view = PropertyView<int8_t>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }
}

TEST_CASE("VecN PropertyView") {
  SECTION("Constructs empty PropertyView") {
    PropertyView<glm::vec3> view;
    REQUIRE(view.status() == PropertyViewStatus::ErrorNonexistentProperty);
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

  SECTION("Reports type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC2;

    PropertyView<glm::vec3> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorTypeMismatch);
  }

  SECTION("Reports component type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::INT8;

    PropertyView<glm::vec3> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SECTION("Reports array type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.array = true;

    PropertyView<glm::vec3> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SECTION("Reports invalid normalization") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.normalized = true;

    PropertyView<glm::vec3> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNormalization);
  }

  SECTION("Constructs with non-optional properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::INT16;
    classProperty.normalized = true;
    classProperty.required = true;

    PropertyView<glm::i16vec3> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
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
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.arrayCount() == 0);
  }

  SECTION("Constructs with offset, scale, max, and min") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::INT16;
    classProperty.offset = {-1, 1, 2};
    classProperty.scale = {2, 1, 3};
    classProperty.max = {10, 5, 6};
    classProperty.min = {-11, -12, -13};

    PropertyView<glm::i16vec3> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.offset());
    REQUIRE(view.scale());
    REQUIRE(view.max());
    REQUIRE(view.min());

    REQUIRE(*view.offset() == glm::i16vec3(-1, 1, 2));
    REQUIRE(*view.scale() == glm::i16vec3(2, 1, 3));
    REQUIRE(*view.max() == glm::i16vec3(10, 5, 6));
    REQUIRE(*view.min() == glm::i16vec3(-11, -12, -13));
  }

  SECTION("Constructs with noData and defaultProperty") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC4;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.required = false;
    classProperty.noData = {0.0f, 0.0f, 0.0f, 0.0f};
    classProperty.defaultProperty = {1.0f, 2.0f, 3.0f, 4.0f};

    PropertyView<glm::vec4> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
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
    classProperty.noData = {0.0f, 0.0f, 0.0f, 0.0f};
    classProperty.defaultProperty = {1.0f, 2.0f, 3.0f, 4.0f};

    PropertyView<glm::vec4> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.required());
    REQUIRE(!view.noData());
    REQUIRE(!view.defaultValue());
  }

  SECTION("Reports errors for out-of-bounds values") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.defaultProperty = {128, 129};

    PropertyView<glm::i8vec2> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    classProperty.noData = {-128, -129};
    view = PropertyView<glm::i8vec2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);

    classProperty.min = {-200, 0};
    view = PropertyView<glm::i8vec2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMin);

    classProperty.max = {10, 5};
    view = PropertyView<glm::i8vec2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMax);

    classProperty.scale = {1, 128};
    view = PropertyView<glm::i8vec2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    classProperty.offset = {-1, -222};
    view = PropertyView<glm::i8vec2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }

  SECTION("Reports errors for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.defaultProperty = {1.0f, 20.9f};

    PropertyView<glm::i8vec2> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    classProperty.noData = "0";
    view = PropertyView<glm::i8vec2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);

    classProperty.min = {-10, -1};
    view = PropertyView<glm::i8vec2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMin);

    classProperty.max = {10, 20, 30, 40};
    view = PropertyView<glm::i8vec2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMax);

    classProperty.scale = 1;
    view = PropertyView<glm::i8vec2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    classProperty.offset = "(1, 2, 3)";
    view = PropertyView<glm::i8vec2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }
}

TEST_CASE("MatN PropertyView") {
  SECTION("Constructs empty PropertyView") {
    PropertyView<glm::mat2> view;
    REQUIRE(view.status() == PropertyViewStatus::ErrorNonexistentProperty);
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

  SECTION("Reports type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT4;

    PropertyView<glm::mat2> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorTypeMismatch);
  }

  SECTION("Reports component type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;

    PropertyView<glm::mat2> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SECTION("Reports array type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.array = true;

    PropertyView<glm::mat2> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SECTION("Reports invalid normalization") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.normalized = true;

    PropertyView<glm::mat2> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNormalization);
  }

  SECTION("Constructs with non-optional properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT3;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.normalized = true;
    classProperty.required = true;

    PropertyView<glm::imat3x3> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
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
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.arrayCount() == 0);
  }

  SECTION("Constructs with offset, scale, max, and min") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT3;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    // clang-format off
    classProperty.offset = {
      -1,  1, 2,
       3, -1, 4,
      -5, -5, 0};
    classProperty.scale = {
      1, 1, 1,
      2, 2, 3,
      3, 4, 5};
    classProperty.max = {
      20,  5, 20,
      30, 22, 43,
      37,  1,  8};
    classProperty.min = {
      -10, -2, -3,
        0, 20,  4,
        9,  4,  5};
    // clang-format on

    PropertyView<glm::imat3x3> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
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

  SECTION("Constructs with noData and defaultProperty") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.required = false;
    // clang-format off
    classProperty.noData = {
      0.0f, 0.0f,
      0.0f, 0.0f};
    classProperty.defaultProperty = {
      1.0f, 2.0f,
      3.0f, 4.0f};
    // clang-format on

    PropertyView<glm::mat2> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
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
    classProperty.noData = {
      0.0f, 0.0f,
      0.0f, 0.0f};
    classProperty.defaultProperty = {
      1.0f, 2.0f,
      3.0f, 4.0f};
    // clang-format on

    PropertyView<glm::mat2> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.required());
    REQUIRE(!view.noData());
    REQUIRE(!view.defaultValue());
  }

  SECTION("Reports errors for out-of-bounds values") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;

    // clang-format off
    classProperty.defaultProperty = {
      999, 1,
      2, 0};
    // clang-format on

    PropertyView<glm::i8mat2x2> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    // clang-format off
    classProperty.noData = {
      0, 0,
       1, -129};
    // clang-format on
    view = PropertyView<glm::i8mat2x2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);

    // clang-format off
    classProperty.min = {
      -29, -40,
      -55, -43};
    // clang-format on
    view = PropertyView<glm::i8mat2x2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMin);

    // clang-format off
    classProperty.max = {
      10, 240,
       1,   8};
    // clang-format on
    view = PropertyView<glm::i8mat2x2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMax);

    //clang-format off
    classProperty.scale = {1, 197, 4, 6};
    // clang-format on
    view = PropertyView<glm::i8mat2x2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    // clang-format off
    classProperty.offset = {
       -1,  2,
      129, -2};
    // clang-format on
    view = PropertyView<glm::i8mat2x2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }

  SECTION("Reports errors for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;

    // clang-format off
    classProperty.defaultProperty =
        JsonValue::Array{JsonValue::Array{999, 1, 2, 0}};
    // clang-format on

    PropertyView<glm::i8mat2x2> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    // clang-format off
    classProperty.noData = {
      0.45, 0.0,
       1.0, -1.4};
    // clang-format on
    view = PropertyView<glm::i8mat2x2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);

    classProperty.min = {0, 0};
    view = PropertyView<glm::i8mat2x2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMin);

    classProperty.max = {10, 20, 30, 40, 50};
    view = PropertyView<glm::i8mat2x2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMax);

    classProperty.scale = 1;
    view = PropertyView<glm::i8mat2x2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    classProperty.offset = "(1, 2, 3, 4)";
    view = PropertyView<glm::i8mat2x2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }
}

TEST_CASE("String PropertyView") {
  SECTION("Constructs empty PropertyView") {
    PropertyView<std::string_view> view;
    REQUIRE(view.status() == PropertyViewStatus::ErrorNonexistentProperty);
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

  SECTION("Reports type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;

    PropertyView<std::string_view> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorTypeMismatch);
  }

  SECTION("Reports array type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.array = true;

    PropertyView<std::string_view> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SECTION("Ignores non-applicable properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.normalized = true;
    classProperty.offset = "offset";
    classProperty.scale = "scale";
    classProperty.max = "max";
    classProperty.min = "min";

    PropertyView<std::string_view> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
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
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.arrayCount() == 0);
  }

  SECTION("Constructs with noData and defaultProperty") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.required = false;
    classProperty.noData = "null";
    classProperty.defaultProperty = "default";

    PropertyView<std::string_view> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
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
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.required());
    REQUIRE(!view.noData());
    REQUIRE(!view.defaultValue());
  }

  SECTION("Reports errors for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.defaultProperty = true;

    PropertyView<std::string_view> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    classProperty.noData = JsonValue::Array{"null"};
    view = PropertyView<std::string_view>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);
  }
}

TEST_CASE("Boolean Array PropertyView") {
  SECTION("Constructs empty PropertyView") {
    PropertyView<PropertyArrayView<bool>> view;
    REQUIRE(view.status() == PropertyViewStatus::ErrorNonexistentProperty);
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

  SECTION("Reports type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.array = true;

    PropertyView<PropertyArrayView<bool>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorTypeMismatch);
  }

  SECTION("Reports array type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;
    classProperty.array = false;

    PropertyView<PropertyArrayView<bool>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SECTION("Ignores non-applicable properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;
    classProperty.array = true;
    classProperty.normalized = true;
    classProperty.offset = JsonValue::Array{true};
    classProperty.scale = JsonValue::Array{true};
    classProperty.max = JsonValue::Array{true};
    classProperty.min = JsonValue::Array{true};
    classProperty.noData = JsonValue::Array{true};

    PropertyView<PropertyArrayView<bool>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
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
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.arrayCount() == 5);
  }

  SECTION("Constructs with defaultProperty") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;
    classProperty.array = true;
    classProperty.required = false;
    classProperty.defaultProperty = JsonValue::Array{false, true};

    PropertyView<PropertyArrayView<bool>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(!view.required());
    REQUIRE(view.defaultValue());

    PropertyArrayView<bool> defaultValue = *view.defaultValue();
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
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.required());
    REQUIRE(!view.defaultValue());
  }

  SECTION("Reports errors for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;
    classProperty.array = true;
    classProperty.defaultProperty = true;

    PropertyView<PropertyArrayView<bool>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);
  }
}

TEST_CASE("Scalar Array PropertyView") {
  SECTION("Constructs empty PropertyView") {
    PropertyView<PropertyArrayView<uint8_t>> view;
    REQUIRE(view.status() == PropertyViewStatus::ErrorNonexistentProperty);
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

  SECTION("Reports type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.array = true;

    PropertyView<PropertyArrayView<uint8_t>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorTypeMismatch);
  }

  SECTION("Reports component type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.array = true;

    PropertyView<PropertyArrayView<uint8_t>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SECTION("Reports array type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.array = false;

    PropertyView<PropertyArrayView<uint8_t>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SECTION("Reports invalid normalization") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.array = true;
    classProperty.normalized = true;

    PropertyView<PropertyArrayView<float>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNormalization);
  }

  SECTION("Constructs with non-optional properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.array = true;
    classProperty.normalized = true;
    classProperty.required = true;

    PropertyView<PropertyArrayView<uint8_t>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
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

  SECTION("Constructs with count") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.array = true;
    classProperty.count = 5;

    PropertyView<PropertyArrayView<uint8_t>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.arrayCount() == *classProperty.count);
  }

  SECTION("Constructs with offset, scale, max, and min") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::INT16;
    classProperty.array = true;
    classProperty.offset = JsonValue::Array{5, 10};
    classProperty.scale = JsonValue::Array{2, 1};
    classProperty.max = JsonValue::Array{10, 20};
    classProperty.min = JsonValue::Array{-10, -1};

    PropertyView<PropertyArrayView<int16_t>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.offset());
    REQUIRE(view.scale());
    REQUIRE(view.max());
    REQUIRE(view.min());

    PropertyArrayView<int16_t> value = *view.offset();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == 5);
    REQUIRE(value[1] == 10);

    value = *view.scale();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == 2);
    REQUIRE(value[1] == 1);

    value = *view.max();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == 10);
    REQUIRE(value[1] == 20);

    value = *view.min();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == -10);
    REQUIRE(value[1] == -1);
  }

  SECTION("Constructs with noData and defaultProperty") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.array = true;
    classProperty.required = false;
    classProperty.noData = {0, 1};
    classProperty.defaultProperty = {2, 3};

    PropertyView<PropertyArrayView<uint8_t>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(!view.required());
    REQUIRE(view.noData());
    REQUIRE(view.defaultValue());

    PropertyArrayView<uint8_t> value = view.noData().value();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == 0);
    REQUIRE(value[1] == 1);

    value = *view.defaultValue();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == 2);
    REQUIRE(value[1] == 3);
  }

  SECTION("Ignores noData and defaultProperty when required is true") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.array = true;
    classProperty.required = true;
    classProperty.noData = {0, 1};
    classProperty.defaultProperty = {2, 3};

    PropertyView<PropertyArrayView<uint8_t>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.required());
    REQUIRE(!view.noData());
    REQUIRE(!view.defaultValue());
  }

  SECTION("Reports errors for out-of-bounds values") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.array = true;
    classProperty.defaultProperty = {256, 256};

    PropertyView<PropertyArrayView<uint8_t>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    classProperty.noData = {-1, 0};
    view = PropertyView<PropertyArrayView<uint8_t>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);

    classProperty.min = {0, -1};
    view = PropertyView<PropertyArrayView<uint8_t>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMin);

    classProperty.max = {256, 255};
    view = PropertyView<PropertyArrayView<uint8_t>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMax);

    classProperty.scale = {20, 300};
    view = PropertyView<PropertyArrayView<uint8_t>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    classProperty.offset = {2, -100};
    view = PropertyView<PropertyArrayView<uint8_t>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }

  SECTION("Reports errors for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.array = true;
    classProperty.defaultProperty = "[256, 256]";

    PropertyView<PropertyArrayView<uint8_t>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    classProperty.noData = 0;
    view = PropertyView<PropertyArrayView<uint8_t>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);

    classProperty.min = false;
    view = PropertyView<PropertyArrayView<uint8_t>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMin);

    classProperty.max = JsonValue::Array{10.4, 30.0};
    view = PropertyView<PropertyArrayView<uint8_t>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMax);

    classProperty.scale = JsonValue::Array{JsonValue::Array{2.3, 3.04}};
    view = PropertyView<PropertyArrayView<uint8_t>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    classProperty.offset = "10";
    view = PropertyView<PropertyArrayView<uint8_t>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }
}

TEST_CASE("VecN Array PropertyView") {
  SECTION("Constructs empty PropertyView") {
    PropertyView<PropertyArrayView<glm::vec3>> view;
    REQUIRE(view.status() == PropertyViewStatus::ErrorNonexistentProperty);
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

  SECTION("Reports type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC2;
    classProperty.array = true;

    PropertyView<PropertyArrayView<glm::vec3>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorTypeMismatch);
  }

  SECTION("Reports component type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.array = true;

    PropertyView<PropertyArrayView<glm::vec3>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SECTION("Reports array type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.array = false;

    PropertyView<PropertyArrayView<glm::vec3>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SECTION("Reports invalid normalization") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.array = true;
    classProperty.normalized = true;

    PropertyView<PropertyArrayView<glm::vec3>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNormalization);
  }

  SECTION("Constructs with non-optional properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::INT16;
    classProperty.array = true;
    classProperty.normalized = true;
    classProperty.required = true;

    PropertyView<PropertyArrayView<glm::i16vec3>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
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

  SECTION("Constructs with count") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::INT16;
    classProperty.array = true;
    classProperty.count = 5;

    PropertyView<PropertyArrayView<glm::i16vec3>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.arrayCount() == classProperty.count);
  }

  SECTION("Constructs with offset, scale, max, and min") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.array = true;
    classProperty.offset = {{-1, 1, 2}, {4, 4, 0}};
    classProperty.scale = {{2, 1, 3}, {8, 2, 3}};
    classProperty.max = {{14, 28, 12}, {10, 5, 6}};
    classProperty.min = {{-11, -12, -13}, {-2, -4, 6}};

    PropertyView<PropertyArrayView<glm::i8vec3>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.offset());
    REQUIRE(view.scale());
    REQUIRE(view.max());
    REQUIRE(view.min());

    PropertyArrayView<glm::i8vec3> value = *view.offset();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == glm::i8vec3(-1, 1, 2));
    REQUIRE(value[1] == glm::i8vec3(4, 4, 0));

    value = *view.scale();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == glm::i8vec3(2, 1, 3));
    REQUIRE(value[1] == glm::i8vec3(8, 2, 3));

    value = *view.max();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == glm::i8vec3(14, 28, 12));
    REQUIRE(value[1] == glm::i8vec3(10, 5, 6));

    value = *view.min();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == glm::i8vec3(-11, -12, -13));
    REQUIRE(value[1] == glm::i8vec3(-2, -4, 6));
  }

  SECTION("Constructs with noData and defaultProperty") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC2;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.array = true;
    classProperty.required = false;
    classProperty.noData = {{0.0f, 0.0f}, {1.0f, 2.0f}};
    classProperty.defaultProperty = {{3.0f, 4.0f}, {5.0f, 6.0f}};

    PropertyView<PropertyArrayView<glm::vec2>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(!view.required());
    REQUIRE(view.noData());
    REQUIRE(view.defaultValue());

    PropertyArrayView<glm::vec2> value = *view.noData();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == glm::vec2(0.0f, 0.0f));
    REQUIRE(value[1] == glm::vec2(1.0f, 2.0f));

    value = *view.defaultValue();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == glm::vec2(3.0f, 4.0f));
    REQUIRE(value[1] == glm::vec2(5.0f, 6.0f));
  }

  SECTION("Ignores noData and defaultProperty when required is true") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC2;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.array = true;
    classProperty.required = true;
    classProperty.noData = {{0.0f, 0.0f}, {1.0f, 2.0f}};
    classProperty.defaultProperty = {{3.0f, 4.0f}, {5.0f, 6.0f}};

    PropertyView<PropertyArrayView<glm::vec2>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.required());
    REQUIRE(!view.noData());
    REQUIRE(!view.defaultValue());
  }

  SECTION("Reports errors for out-of-bounds values") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.array = true;
    classProperty.defaultProperty = {{128, 129}, {0, 2}};

    PropertyView<PropertyArrayView<glm::i8vec2>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    classProperty.noData = {{0, 0}, {-128, -129}};
    view = PropertyView<PropertyArrayView<glm::i8vec2>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);

    classProperty.min = {{-2, -3}, {-200, 0}};
    view = PropertyView<PropertyArrayView<glm::i8vec2>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMin);

    classProperty.max = {{10, 5}, {808, 3}};
    view = PropertyView<PropertyArrayView<glm::i8vec2>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMax);

    classProperty.scale = {{1, 128}, {2, 2}};
    view = PropertyView<PropertyArrayView<glm::i8vec2>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    classProperty.offset = {{0, 0}, {-1, -222}};
    view = PropertyView<PropertyArrayView<glm::i8vec2>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }

  SECTION("Reports errors for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.array = true;
    classProperty.defaultProperty = {1, 20};

    PropertyView<PropertyArrayView<glm::i8vec2>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    classProperty.noData = JsonValue::Array{{2.0f, 5.4f}};
    view = PropertyView<PropertyArrayView<glm::i8vec2>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);

    classProperty.min = {{-10, -1, 4}, {0, 0, 0}};
    view = PropertyView<PropertyArrayView<glm::i8vec2>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMin);

    classProperty.max = {{10, 20, 30, 40}, {1, 2, 3, 4}};
    view = PropertyView<PropertyArrayView<glm::i8vec2>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMax);

    classProperty.scale = 2;
    view = PropertyView<PropertyArrayView<glm::i8vec2>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    classProperty.offset = "(1, 2)";
    view = PropertyView<PropertyArrayView<glm::i8vec2>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }
}

TEST_CASE("MatN Array PropertyView") {
  SECTION("Constructs empty PropertyView") {
    PropertyView<PropertyArrayView<glm::mat2>> view;
    REQUIRE(view.status() == PropertyViewStatus::ErrorNonexistentProperty);
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

  SECTION("Reports type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT4;
    classProperty.array = true;

    PropertyView<PropertyArrayView<glm::mat2>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorTypeMismatch);
  }

  SECTION("Reports component type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.array = true;

    PropertyView<PropertyArrayView<glm::mat2>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SECTION("Reports array type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.array = false;

    PropertyView<PropertyArrayView<glm::mat2>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SECTION("Reports invalid normalization") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.array = true;
    classProperty.normalized = true;

    PropertyView<PropertyArrayView<glm::mat2>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNormalization);
  }

  SECTION("Constructs with non-optional properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT3;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.array = true;
    classProperty.normalized = true;
    classProperty.required = true;

    PropertyView<PropertyArrayView<glm::imat3x3>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
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

  SECTION("Constructs with count") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT3;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.array = true;
    classProperty.count = 5;

    PropertyView<PropertyArrayView<glm::imat3x3>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.arrayCount() == classProperty.count);
  }

  SECTION("Constructs with offset, scale, max, and min") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.array = true;
    // clang-format off
    classProperty.offset = {
      {-1,  1,
        0,  2},
      {2, 40,
       6, -8},
    };
    classProperty.scale = {
      {1, 1,
       1, 0},
      {-2, 5,
       7, 1}
    };
    classProperty.max = {
      {2, 4,
       8, 0},
      {-7, 8,
       4, 4},
    };
    classProperty.min = {
      {-1, -6,
       -1, 2},
      {0, 1,
       2, 3},
    };
    // clang-format on

    PropertyView<PropertyArrayView<glm::i8mat2x2>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.offset());
    REQUIRE(view.scale());
    REQUIRE(view.max());
    REQUIRE(view.min());

    PropertyArrayView<glm::i8mat2x2> value = *view.offset();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == glm::i8mat2x2(-1, 1, 0, 2));
    REQUIRE(value[1] == glm::i8mat2x2(2, 40, 6, -8));

    value = *view.scale();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == glm::i8mat2x2(1, 1, 1, 0));
    REQUIRE(value[1] == glm::i8mat2x2(-2, 5, 7, 1));

    value = *view.max();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == glm::i8mat2x2(2, 4, 8, 0));
    REQUIRE(value[1] == glm::i8mat2x2(-7, 8, 4, 4));

    value = *view.min();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == glm::i8mat2x2(-1, -6, -1, 2));
    REQUIRE(value[1] == glm::i8mat2x2(0, 1, 2, 3));
  }

  SECTION("Constructs with noData and defaultProperty") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.array = true;
    classProperty.required = false;
    // clang-format off
    classProperty.noData = {
      {0, 0,
       0, 0},
      {-1, -1,
       -1, -1},
    };
    classProperty.defaultProperty = {
      {1, 1,
       1, 1},
      {2, 2,
       2, 2},
    };
    // clang-format on

    PropertyView<PropertyArrayView<glm::i8mat2x2>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(!view.required());
    REQUIRE(view.noData());
    REQUIRE(view.defaultValue());

    PropertyArrayView<glm::i8mat2x2> value = *view.noData();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == glm::i8mat2x2(0, 0, 0, 0));
    REQUIRE(value[1] == glm::i8mat2x2(-1, -1, -1, -1));

    value = view.defaultValue().value();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == glm::i8mat2x2(1, 1, 1, 1));
    REQUIRE(value[1] == glm::i8mat2x2(2, 2, 2, 2));
  }

  SECTION("Ignores noData and defaultProperty when required is true") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.array = true;
    classProperty.required = true;
    // clang-format off
    classProperty.noData = {
      {0, 0,
       0, 0},
      {-1, -1,
       -1, -1},
    };
    classProperty.defaultProperty = {
      {1, 1,
       1, 1},
      {2, 2,
       2, 2},
    };
    // clang-format on

    PropertyView<PropertyArrayView<glm::mat2>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.required());
    REQUIRE(!view.noData());
    REQUIRE(!view.defaultValue());
  }

  SECTION("Reports errors for out-of-bounds values") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.array = true;

    // clang-format off
    classProperty.defaultProperty = {
      {1, 1,
       1, 290},
      {2, 2,
       2, 2},
    };
    // clang-format on

    PropertyView<PropertyArrayView<glm::i8mat2x2>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    // clang-format off
    classProperty.noData = {
      {0, 0,
       0, 0},
      {-140, -1,
       -1, -1},
    };
    // clang-format on
    view = PropertyView<PropertyArrayView<glm::i8mat2x2>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);

    // clang-format off
    classProperty.min = {
      {-129, 0,
       0, 0},
      {-1, -1,
       -1, -1},
    };
    // clang-format on
    view = PropertyView<PropertyArrayView<glm::i8mat2x2>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMin);

    // clang-format off
    classProperty.max = {
      {-128, 189,
         20,   2},
      {10, 12,
       8, 4},
    };
    // clang-format on
    view = PropertyView<PropertyArrayView<glm::i8mat2x2>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMax);

    //clang-format off
    classProperty.scale = {
        {1, 2, 3, 4},
        {256, 80, 9, 52},
    };
    // clang-format on
    view = PropertyView<PropertyArrayView<glm::i8mat2x2>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    // clang-format off
    classProperty.offset = {
      {129, 0,
       0, 2},
      {4, 0,
       0, 8},};
    // clang-format on
    view = PropertyView<PropertyArrayView<glm::i8mat2x2>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }

  SECTION("Reports errors for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.array = true;
    classProperty.defaultProperty = {4, 1, 2, 0};

    PropertyView<PropertyArrayView<glm::i8mat2x2>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    // clang-format off
    classProperty.noData = {
      {0.45, 0.0,
       1.0, -1.4}
    };
    // clang-format on
    view = PropertyView<PropertyArrayView<glm::i8mat2x2>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);

    classProperty.min = {{0, 1, 2, 3, 4, 5, 6}};
    view = PropertyView<PropertyArrayView<glm::i8mat2x2>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMin);

    classProperty.max = {{0, 1, 2, 3}, false};
    view = PropertyView<PropertyArrayView<glm::i8mat2x2>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMax);

    classProperty.scale = 1;
    view = PropertyView<PropertyArrayView<glm::i8mat2x2>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    classProperty.offset = "[(1, 2, 3, 4)]";
    view = PropertyView<PropertyArrayView<glm::i8mat2x2>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }
}

TEST_CASE("String Array PropertyView") {
  SECTION("Constructs empty PropertyView") {
    PropertyView<PropertyArrayView<std::string_view>> view;
    REQUIRE(view.status() == PropertyViewStatus::ErrorNonexistentProperty);
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

  SECTION("Reports type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;
    classProperty.array = true;

    PropertyView<PropertyArrayView<std::string_view>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorTypeMismatch);
  }

  SECTION("Reports array type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.array = false;

    PropertyView<PropertyArrayView<std::string_view>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorArrayTypeMismatch);
  }

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
    REQUIRE(view.status() == PropertyViewStatus::Valid);
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
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.arrayCount() == classProperty.count);
  }

  SECTION("Constructs with noData and defaultProperty") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.array = true;
    classProperty.required = false;
    classProperty.noData = {"null", "0"};
    classProperty.defaultProperty = {"default1", "default2"};

    PropertyView<PropertyArrayView<std::string_view>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
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
    classProperty.array = true;
    classProperty.required = true;
    classProperty.noData = {"null", "0"};
    classProperty.defaultProperty = {"default1", "default2"};

    PropertyView<PropertyArrayView<std::string_view>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.required());
    REQUIRE(!view.noData());
    REQUIRE(!view.defaultValue());
  }

  SECTION("Reports errors for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.array = true;
    classProperty.defaultProperty = true;

    PropertyView<PropertyArrayView<std::string_view>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    classProperty.noData = JsonValue::Array{"null", 0};
    view = PropertyView<PropertyArrayView<std::string_view>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);
  }
}
