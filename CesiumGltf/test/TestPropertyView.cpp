#include <CesiumGltf/ClassProperty.h>
#include <CesiumGltf/PropertyArrayView.h>
#include <CesiumGltf/PropertyView.h>
#include <CesiumUtility/JsonValue.h>

#include <doctest/doctest.h>
#include <glm/ext/matrix_double2x2.hpp>
#include <glm/ext/matrix_double3x3.hpp>
#include <glm/ext/matrix_float2x2.hpp>
#include <glm/ext/matrix_float3x3.hpp>
#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/ext/vector_int2_sized.hpp>
#include <glm/ext/vector_int3.hpp>
#include <glm/ext/vector_int3_sized.hpp>
#include <glm/ext/vector_int4_sized.hpp>
#include <glm/ext/vector_uint3_sized.hpp>
#include <glm/fwd.hpp>

#include <cstdint>
#include <ostream>
#include <string_view>

using namespace CesiumGltf;
using namespace CesiumUtility;

TEST_CASE("Boolean PropertyView") {
  SUBCASE("Constructs empty PropertyView") {
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

  SUBCASE("Reports type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;

    PropertyView<bool> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Reports array type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;
    classProperty.array = true;

    PropertyView<bool> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Constructs with defaultProperty") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;
    classProperty.required = false;
    classProperty.defaultProperty = false;

    PropertyView<bool> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(!view.required());
    REQUIRE(view.defaultValue() == false);
    ;
  }

  SUBCASE("Reports errors for incorrectly defined properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;
    classProperty.required = true;
    classProperty.defaultProperty = false;

    PropertyView<bool> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);
  }

  SUBCASE("Reports default value invalid type") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;
    classProperty.defaultProperty = 1;

    PropertyView<bool> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);
  }
}

TEST_CASE("Scalar PropertyView") {
  SUBCASE("Constructs empty PropertyView") {
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

  SUBCASE("Reports type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;

    PropertyView<uint8_t> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Reports component type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::INT8;

    PropertyView<uint8_t> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Reports array type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.array = true;

    PropertyView<uint8_t> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Reports invalid normalization") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.normalized = true;

    PropertyView<int8_t> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Constructs with offset, scale, max, and min") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.offset = 5.04f;
    classProperty.scale = 2.2f;
    classProperty.max = 10.5f;
    classProperty.min = -10.5f;

    PropertyView<float> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.offset() == 5.04f);
    REQUIRE(view.scale() == 2.2f);
    REQUIRE(view.max() == 10.5f);
    REQUIRE(view.min() == -10.5f);
  }

  SUBCASE("Constructs with noData and defaultProperty") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.required = false;
    classProperty.noData = 0;
    classProperty.defaultProperty = 1;

    PropertyView<uint8_t> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(!view.required());
    REQUIRE(view.noData() == 0);
    REQUIRE(view.defaultValue() == 1);
  }

  SUBCASE("Reports errors for incorrectly defined properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.required = true;
    classProperty.defaultProperty = 1;

    PropertyView<int8_t> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    classProperty.noData = 0;
    view = PropertyView<int8_t>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);

    classProperty.scale = 200;
    view = PropertyView<int8_t>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    classProperty.offset = 1234;
    view = PropertyView<int8_t>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }

  SUBCASE("Reports errors for out-of-bounds values") {
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
  }

  SUBCASE("Reports errors for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.defaultProperty = JsonValue::Array{1};

    PropertyView<float> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    classProperty.noData = "0";
    view = PropertyView<float>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);

    classProperty.scale = false;
    view = PropertyView<float>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    classProperty.offset = JsonValue::Array{};
    view = PropertyView<float>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }
}

TEST_CASE("Scalar PropertyView (normalized)") {
  SUBCASE("Constructs empty PropertyView") {
    PropertyView<uint8_t, true> view;
    REQUIRE(view.status() == PropertyViewStatus::ErrorNonexistentProperty);
    REQUIRE(view.arrayCount() == 0);
    REQUIRE(view.normalized());
    REQUIRE(!view.offset());
    REQUIRE(!view.scale());
    REQUIRE(!view.max());
    REQUIRE(!view.min());
    REQUIRE(!view.required());
    REQUIRE(!view.noData());
    REQUIRE(!view.defaultValue());
  }

  SUBCASE("Reports type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;

    PropertyView<uint8_t, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Reports component type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::INT8;

    PropertyView<uint8_t, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Reports array type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.array = true;

    PropertyView<uint8_t, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Reports invalid normalization") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.normalized = false;

    PropertyView<int8_t, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Constructs with offset, scale, max, and min") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.normalized = true;
    classProperty.offset = 5.04f;
    classProperty.scale = 2.2f;
    classProperty.max = 10.5f;
    classProperty.min = -10.5f;

    PropertyView<int32_t, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.offset() == 5.04f);
    REQUIRE(view.scale() == 2.2f);
    REQUIRE(view.max() == 10.5f);
    REQUIRE(view.min() == -10.5f);
  }

  SUBCASE("Constructs with noData and defaultProperty") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.normalized = true;
    classProperty.required = false;
    classProperty.noData = 0;
    classProperty.defaultProperty = 1.5;

    PropertyView<uint8_t, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(!view.required());
    REQUIRE(view.noData() == 0);
    REQUIRE(view.defaultValue() == 1.5);
  }

  SUBCASE("Reports errors for incorrectly defined properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.normalized = true;
    classProperty.required = true;
    classProperty.defaultProperty = 1.0;

    PropertyView<int8_t, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    classProperty.noData = 0;
    view = PropertyView<int8_t, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);
  }

  SUBCASE("Reports errors for out-of-bounds values") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.normalized = true;
    classProperty.noData = -129;

    PropertyView<int8_t, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);
  }

  SUBCASE("Reports errors for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.normalized = true;
    classProperty.defaultProperty = JsonValue::Array{1};

    PropertyView<int8_t, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    classProperty.noData = "0";
    view = PropertyView<int8_t, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);

    classProperty.scale = false;
    view = PropertyView<int8_t, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    classProperty.offset = JsonValue::Array{};
    view = PropertyView<int8_t, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }
}

TEST_CASE("VecN PropertyView") {
  SUBCASE("Constructs empty PropertyView") {
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

  SUBCASE("Reports type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC2;

    PropertyView<glm::vec3> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Reports component type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::INT8;

    PropertyView<glm::vec3> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Reports array type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.array = true;

    PropertyView<glm::vec3> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Reports invalid normalization") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.normalized = true;

    PropertyView<glm::i8vec3> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Constructs with offset, scale, max, and min") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.offset = {-1, 1, 2};
    classProperty.scale = {2, 1, 3};
    classProperty.max = {10, 5, 6};
    classProperty.min = {-11, -12, -13};

    PropertyView<glm::vec3> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.offset() == glm::vec3(-1, 1, 2));
    REQUIRE(view.scale() == glm::vec3(2, 1, 3));
    REQUIRE(view.max() == glm::vec3(10, 5, 6));
    REQUIRE(view.min() == glm::vec3(-11, -12, -13));
  }

  SUBCASE("Constructs with noData and defaultProperty") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC4;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.required = false;
    classProperty.noData = {0.0f, 0.0f, 0.0f, 0.0f};
    classProperty.defaultProperty = {1.0f, 2.0f, 3.0f, 4.0f};

    PropertyView<glm::vec4> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.noData() == glm::vec4(0.0f));
    REQUIRE(view.defaultValue() == glm::vec4(1.0f, 2.0f, 3.0f, 4.0f));
  }

  SUBCASE("Reports errors for incorrectly defined properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.required = true;
    classProperty.defaultProperty = {1, 2};

    PropertyView<glm::i8vec2> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    classProperty.noData = {0, 0};
    view = PropertyView<glm::i8vec2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);

    classProperty.scale = {3, 2};
    view = PropertyView<glm::i8vec2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    classProperty.offset = {12, 8};
    view = PropertyView<glm::i8vec2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }

  SUBCASE("Reports errors for out-of-bounds values") {
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

    classProperty.max = {0, 500};
    view = PropertyView<glm::i8vec2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMax);
  }

  SUBCASE("Reports errors for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC2;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.defaultProperty = true;

    PropertyView<glm::vec2> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    classProperty.noData = "0";
    view = PropertyView<glm::vec2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);

    classProperty.min = JsonValue::Array{-10};
    view = PropertyView<glm::vec2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMin);

    classProperty.max = {10, 20, 30, 40};
    view = PropertyView<glm::vec2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMax);

    classProperty.scale = 1;
    view = PropertyView<glm::vec2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    classProperty.offset = "(1, 2, 3)";
    view = PropertyView<glm::vec2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }
}

TEST_CASE("VecN PropertyView (normalized)") {
  SUBCASE("Constructs empty PropertyView") {
    PropertyView<glm::i8vec2, true> view;
    REQUIRE(view.status() == PropertyViewStatus::ErrorNonexistentProperty);
    REQUIRE(view.arrayCount() == 0);
    REQUIRE(view.normalized());
    REQUIRE(!view.offset());
    REQUIRE(!view.scale());
    REQUIRE(!view.max());
    REQUIRE(!view.min());
    REQUIRE(!view.required());
    REQUIRE(!view.noData());
    REQUIRE(!view.defaultValue());
  }

  SUBCASE("Reports type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC2;

    PropertyView<glm::u8vec3, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Reports component type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::INT8;

    PropertyView<glm::u8vec3, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Reports array type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.array = true;

    PropertyView<glm::u8vec3, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Reports invalid normalization") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.normalized = false;

    PropertyView<glm::u8vec3, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Constructs with offset, scale, max, and min") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.normalized = true;
    classProperty.offset = {-1, 1, 2};
    classProperty.scale = {2, 1, 3};
    classProperty.max = {10, 5, 6};
    classProperty.min = {-11, -12, -13};

    PropertyView<glm::ivec3, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.offset() == glm::dvec3(-1, 1, 2));
    REQUIRE(view.scale() == glm::dvec3(2, 1, 3));
    REQUIRE(view.max() == glm::dvec3(10, 5, 6));
    REQUIRE(view.min() == glm::dvec3(-11, -12, -13));
  }

  SUBCASE("Constructs with noData and defaultProperty") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC4;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.normalized = true;
    classProperty.required = false;
    classProperty.noData = {0, 0, -1, -1};
    classProperty.defaultProperty = {1.0, 2.0, 3.0, 4.5};

    PropertyView<glm::i8vec4, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(!view.required());
    REQUIRE(view.noData() == glm::i8vec4(0, 0, -1, -1));
    REQUIRE(view.defaultValue() == glm::dvec4(1.0, 2.0, 3.0, 4.5));
  }

  SUBCASE("Reports errors for incorrectly defined properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.normalized = true;
    classProperty.required = true;
    classProperty.defaultProperty = {1, 2};

    PropertyView<glm::i8vec2, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    classProperty.noData = {0, 0};
    view = PropertyView<glm::i8vec2, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);
  }

  SUBCASE("Reports errors for out-of-bounds values") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.normalized = true;
    classProperty.noData = {-128, -129};

    PropertyView<glm::i8vec2, true> view(classProperty);
    view = PropertyView<glm::i8vec2, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);
  }

  SUBCASE("Reports errors for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC2;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.normalized = true;
    classProperty.defaultProperty = true;

    PropertyView<glm::ivec2, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    classProperty.noData = "0";
    view = PropertyView<glm::ivec2, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);

    classProperty.min = JsonValue::Array{-10};
    view = PropertyView<glm::ivec2, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMin);

    classProperty.max = {10, 20, 30, 40};
    view = PropertyView<glm::ivec2, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMax);

    classProperty.scale = 1;
    view = PropertyView<glm::ivec2, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    classProperty.offset = "(1, 2, 3)";
    view = PropertyView<glm::ivec2, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }
}

TEST_CASE("MatN PropertyView") {
  SUBCASE("Constructs empty PropertyView") {
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

  SUBCASE("Reports type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT4;

    PropertyView<glm::mat2> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Reports component type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;

    PropertyView<glm::mat2> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Reports array type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.array = true;

    PropertyView<glm::mat2> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Reports invalid normalization") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.normalized = true;

    PropertyView<glm::mat2> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Constructs with offset, scale, max, and min") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT3;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
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

    PropertyView<glm::mat3> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);

    // clang-format off
    glm::mat3 expectedOffset(
      -1,  1, 2,
       3, -1, 4,
      -5, -5, 0);
    REQUIRE(view.offset() == expectedOffset);

    glm::mat3 expectedScale(
      1, 1, 1,
      2, 2, 3,
      3, 4, 5);
    REQUIRE(view.scale() == expectedScale);

    glm::mat3 expectedMax(
      20,  5, 20,
      30, 22, 43,
      37,  1,  8);
    REQUIRE(view.max() == expectedMax);

    glm::mat3 expectedMin(
      -10, -2, -3,
        0, 20,  4,
        9,  4,  5);
    REQUIRE(view.min() == expectedMin);
    // clang-format on
  }

  SUBCASE("Constructs with noData and defaultProperty") {
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
      3.0f, 4.5f};
    // clang-format on

    PropertyView<glm::mat2> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(!view.required());

    // clang-format off
    glm::mat2 expectedNoData(
      0.0f, 0.0f,
      0.0f, 0.0f);
    REQUIRE(view.noData() == expectedNoData);

    glm::mat2 expectedDefaultValue(
      1.0f, 2.0f,
      3.0f, 4.5f);
    REQUIRE(view.defaultValue() == expectedDefaultValue);
  }

  SUBCASE("Reports errors for incorrectly defined properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.required = true;
    
    // clang-format off
    classProperty.defaultProperty = {
      1, 2,
      3, 4};
    // clang-format on
    PropertyView<glm::i8mat2x2> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    // clang-format off
    classProperty.noData = {
      0, 0,
      0, 0};
    // clang-format on
    view = PropertyView<glm::i8mat2x2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);

    // clang-format off
    classProperty.scale = {
      1, 1,
      -1, 1};
    // clang-format on
    view = PropertyView<glm::i8mat2x2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    // clang-format off
    classProperty.offset = {
      0, 0,
      2, 1};
    // clang-format on
    view = PropertyView<glm::i8mat2x2>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }

  SUBCASE("Reports errors for out-of-bounds values") {
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
      -29, -240,
      -155, -43};
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

  SUBCASE("Reports errors for invalid types") {
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

TEST_CASE("MatN PropertyView (normalized)") {
  SUBCASE("Constructs empty PropertyView") {
    PropertyView<glm::imat2x2, true> view;
    REQUIRE(view.status() == PropertyViewStatus::ErrorNonexistentProperty);
    REQUIRE(view.arrayCount() == 0);
    REQUIRE(view.normalized());
    REQUIRE(!view.offset());
    REQUIRE(!view.scale());
    REQUIRE(!view.max());
    REQUIRE(!view.min());
    REQUIRE(!view.required());
    REQUIRE(!view.noData());
    REQUIRE(!view.defaultValue());
  }

  SUBCASE("Reports type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT4;

    PropertyView<glm::imat2x2, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Reports component type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;

    PropertyView<glm::imat2x2, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Reports array type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.array = true;

    PropertyView<glm::imat2x2, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Reports invalid normalization") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.normalized = false;

    PropertyView<glm::imat2x2, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Constructs with offset, scale, max, and min") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT3;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.normalized = true;
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

    PropertyView<glm::imat3x3, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    // clang-format off
    glm::dmat3 expectedOffset(
      -1,  1, 2,
       3, -1, 4,
      -5, -5, 0);
    REQUIRE(view.offset() == expectedOffset);

    glm::dmat3 expectedScale(
      1, 1, 1,
      2, 2, 3,
      3, 4, 5);
    REQUIRE(view.scale() == expectedScale);

    glm::dmat3 expectedMax(
      20,  5, 20,
      30, 22, 43,
      37,  1,  8);
    REQUIRE(view.max() == expectedMax);

    glm::dmat3 expectedMin(
      -10, -2, -3,
        0, 20,  4,
        9,  4,  5);
    REQUIRE(view.min() == expectedMin);
    // clang-format on
  }

  SUBCASE("Constructs with noData and defaultProperty") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.normalized = true;
    classProperty.required = false;
    // clang-format off
    classProperty.noData = {
      0, 0,
      0, 0};
    classProperty.defaultProperty = {
      1.0, 2.0,
      3.0, 4.5};
    // clang-format on

    PropertyView<glm::imat2x2, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(!view.required());

    glm::imat2x2 expectedNoData(0);
    REQUIRE(view.noData() == expectedNoData);

    // clang-format off
    glm::dmat2 expectedDefaultValue(
      1.0, 2.0,
      3.0, 4.5);
    REQUIRE(view.defaultValue() == expectedDefaultValue);
  }

  SUBCASE("Reports errors for incorrectly defined properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.normalized = true;
    classProperty.required = true;
    // clang-format off
    classProperty.defaultProperty = {
      1.0, 2.0,
      3.0, 4.5};
    // clang-format on

    PropertyView<glm::imat2x2, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    // clang-format off
    classProperty.noData = {
      0, 0,
      0, 0};
    // clang-format on
    view = PropertyView<glm::imat2x2, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);
  }

  SUBCASE("Reports errors for out-of-bounds values") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.normalized = true;
    // clang-format off
    classProperty.noData = {
      0, 0,
       1, -129};
    // clang-format on

    PropertyView<glm::i8mat2x2, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);
  }

  SUBCASE("Reports errors for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.normalized = true;

    // clang-format off
    classProperty.defaultProperty = JsonValue::Array{
      JsonValue::Array{4, 1, 2, 0},
      JsonValue::Array{2, 3, 1, 1}
    };
    // clang-format on

    PropertyView<glm::i8mat2x2, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    // clang-format off
    classProperty.noData = {
      0.45, 0.0,
       1.0, -1.4};
    // clang-format on
    view = PropertyView<glm::i8mat2x2, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);

    classProperty.min = {0, 0};
    view = PropertyView<glm::i8mat2x2, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMin);

    classProperty.max = {10, 20, 30, 40, 50};
    view = PropertyView<glm::i8mat2x2, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMax);

    classProperty.scale = 1;
    view = PropertyView<glm::i8mat2x2, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    classProperty.offset = "(1, 2, 3, 4)";
    view = PropertyView<glm::i8mat2x2, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }
}

TEST_CASE("String PropertyView") {
  SUBCASE("Constructs empty PropertyView") {
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

  SUBCASE("Reports type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;

    PropertyView<std::string_view> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Reports array type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.array = true;

    PropertyView<std::string_view> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Constructs with noData and defaultProperty") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.required = false;
    classProperty.noData = "null";
    classProperty.defaultProperty = "default";

    PropertyView<std::string_view> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(!view.required());

    REQUIRE(view.noData() == "null");
    REQUIRE(view.defaultValue() == "default");
  }

  SUBCASE("Reports errors for incorrectly defined properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.required = true;
    classProperty.defaultProperty = "default";

    PropertyView<std::string_view> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    classProperty.noData = "null";
    view = PropertyView<std::string_view>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);
  }

  SUBCASE("Reports errors for invalid types") {
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
  SUBCASE("Constructs empty PropertyView") {
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

  SUBCASE("Reports type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.array = true;

    PropertyView<PropertyArrayView<bool>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Reports array type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;
    classProperty.array = false;

    PropertyView<PropertyArrayView<bool>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Constructs with count") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;
    classProperty.array = true;
    classProperty.count = 5;

    PropertyView<PropertyArrayView<bool>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.arrayCount() == 5);
  }

  SUBCASE("Constructs with defaultProperty") {
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

  SUBCASE("Reports errors for incorrectly defined properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;
    classProperty.array = true;
    classProperty.required = true;
    classProperty.defaultProperty = JsonValue::Array{false, true};

    PropertyView<PropertyArrayView<bool>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);
  }

  SUBCASE("Reports errors for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;
    classProperty.array = true;
    classProperty.defaultProperty = true;

    PropertyView<PropertyArrayView<bool>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);
  }
}

TEST_CASE("Scalar Array PropertyView") {
  SUBCASE("Constructs empty PropertyView") {
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

  SUBCASE("Reports type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.array = true;

    PropertyView<PropertyArrayView<uint8_t>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Reports component type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.array = true;

    PropertyView<PropertyArrayView<uint8_t>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Reports array type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.array = false;

    PropertyView<PropertyArrayView<uint8_t>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Reports invalid normalization") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.array = true;
    classProperty.normalized = true;

    PropertyView<PropertyArrayView<int32_t>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Constructs with count") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.array = true;
    classProperty.count = 5;

    PropertyView<PropertyArrayView<uint8_t>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.arrayCount() == *classProperty.count);
  }

  SUBCASE("Constructs with offset, scale, max, and min") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.array = true;
    classProperty.count = 2;
    classProperty.offset = JsonValue::Array{5.0f, 10.0f};
    classProperty.scale = JsonValue::Array{2.0f, 1.0f};
    classProperty.max = JsonValue::Array{10.0f, 20.0f};
    classProperty.min = JsonValue::Array{-10.0f, -1.0f};

    PropertyView<PropertyArrayView<float>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.offset());
    REQUIRE(view.scale());
    REQUIRE(view.max());
    REQUIRE(view.min());

    PropertyArrayView<float> value = *view.offset();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == 5.0f);
    REQUIRE(value[1] == 10.0f);

    value = *view.scale();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == 2.0f);
    REQUIRE(value[1] == 1.0f);

    value = *view.max();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == 10.0f);
    REQUIRE(value[1] == 20.0f);

    value = *view.min();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == -10.0f);
    REQUIRE(value[1] == -1.0f);
  }

  SUBCASE("Constructs with noData and defaultProperty") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.array = true;
    classProperty.count = 2;
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

  SUBCASE("Reports errors for defined properties on variable-length arrays") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.array = true;
    classProperty.count = 0;
    classProperty.min = {0, 0};

    PropertyView<PropertyArrayView<float>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMin);

    classProperty.max = {5, 4};
    view = PropertyView<PropertyArrayView<float>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMax);

    classProperty.scale = {1, 1};
    view = PropertyView<PropertyArrayView<float>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    classProperty.offset = {0, 2};
    view = PropertyView<PropertyArrayView<float>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }

  SUBCASE("Reports errors for incorrectly defined properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.array = true;
    classProperty.count = 2;
    classProperty.required = true;
    classProperty.defaultProperty = {2, 3};

    PropertyView<PropertyArrayView<uint8_t>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    classProperty.noData = {0, 1};
    view = PropertyView<PropertyArrayView<uint8_t>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);

    classProperty.scale = {1, 1};
    view = PropertyView<PropertyArrayView<uint8_t>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    classProperty.offset = {0, 2};
    view = PropertyView<PropertyArrayView<uint8_t>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }

  SUBCASE("Reports errors for out-of-bounds values") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.array = true;
    classProperty.count = 2;
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

  SUBCASE("Reports errors for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.array = true;
    classProperty.count = 2;
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

TEST_CASE("Scalar Array PropertyView (normalized)") {
  SUBCASE("Constructs empty PropertyView") {
    PropertyView<PropertyArrayView<uint8_t>, true> view;
    REQUIRE(view.status() == PropertyViewStatus::ErrorNonexistentProperty);
    REQUIRE(view.arrayCount() == 0);
    REQUIRE(view.normalized());
    REQUIRE(!view.offset());
    REQUIRE(!view.scale());
    REQUIRE(!view.max());
    REQUIRE(!view.min());
    REQUIRE(!view.required());
    REQUIRE(!view.noData());
    REQUIRE(!view.defaultValue());
  }

  SUBCASE("Reports type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.array = true;

    PropertyView<PropertyArrayView<uint8_t>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Reports component type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.array = true;

    PropertyView<PropertyArrayView<uint8_t>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Reports array type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.array = false;

    PropertyView<PropertyArrayView<uint8_t>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Reports invalid normalization") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.array = true;
    classProperty.normalized = false;

    PropertyView<PropertyArrayView<int32_t>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Constructs with count") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.array = true;
    classProperty.normalized = true;
    classProperty.count = 5;

    PropertyView<PropertyArrayView<uint8_t>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.arrayCount() == *classProperty.count);
  }

  SUBCASE("Constructs with offset, scale, max, and min") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::INT16;
    classProperty.array = true;
    classProperty.count = 2;
    classProperty.normalized = true;
    classProperty.offset = JsonValue::Array{5.0, 10.0};
    classProperty.scale = JsonValue::Array{2.0, 1.0};
    classProperty.max = JsonValue::Array{10.0, 20.0};
    classProperty.min = JsonValue::Array{-10.0, -1.0};

    PropertyView<PropertyArrayView<int16_t>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.offset());
    REQUIRE(view.scale());
    REQUIRE(view.max());
    REQUIRE(view.min());

    PropertyArrayView<double> value = *view.offset();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == 5.0);
    REQUIRE(value[1] == 10.0);

    value = *view.scale();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == 2.0);
    REQUIRE(value[1] == 1.0);

    value = *view.max();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == 10.0);
    REQUIRE(value[1] == 20.0);

    value = *view.min();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == -10.0);
    REQUIRE(value[1] == -1.0);
  }

  SUBCASE("Constructs with noData and defaultProperty") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.array = true;
    classProperty.normalized = true;
    classProperty.required = false;
    classProperty.noData = {0, 1};
    classProperty.defaultProperty = {2.5, 3.5};

    PropertyView<PropertyArrayView<uint8_t>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(!view.required());
    REQUIRE(view.noData());
    REQUIRE(view.defaultValue());

    PropertyArrayView<uint8_t> noData(*view.noData());
    REQUIRE(noData.size() == 2);
    REQUIRE(noData[0] == 0);
    REQUIRE(noData[1] == 1);

    PropertyArrayView<double> defaultValue(*view.defaultValue());
    REQUIRE(defaultValue.size() == 2);
    REQUIRE(defaultValue[0] == 2.5);
    REQUIRE(defaultValue[1] == 3.5);
  }

  SUBCASE("Reports errors for defined properties on variable-length arrays") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.array = true;
    classProperty.count = 0;
    classProperty.normalized = true;
    classProperty.min = {0, 0};

    PropertyView<PropertyArrayView<uint8_t>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMin);

    classProperty.max = {5, 4};
    view = PropertyView<PropertyArrayView<uint8_t>, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMax);

    classProperty.scale = {1, 1};
    view = PropertyView<PropertyArrayView<uint8_t>, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    classProperty.offset = {0, 2};
    classProperty.offset = {0, 2};
    view = PropertyView<PropertyArrayView<uint8_t>, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }

  SUBCASE("Reports errors for incorrectly defined properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.array = true;
    classProperty.count = 2;
    classProperty.normalized = true;
    classProperty.required = true;
    classProperty.defaultProperty = {2, 3};

    PropertyView<PropertyArrayView<uint8_t>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    classProperty.noData = {0, 1};
    view = PropertyView<PropertyArrayView<uint8_t>, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);
  }

  SUBCASE("Reports errors for out-of-bounds values") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.array = true;
    classProperty.count = 2;
    classProperty.normalized = true;
    classProperty.noData = {-1, 0};

    PropertyView<PropertyArrayView<uint8_t>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);
  }

  SUBCASE("Reports errors for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.array = true;
    classProperty.count = 2;
    classProperty.normalized = true;
    classProperty.defaultProperty = "[256, 256]";

    PropertyView<PropertyArrayView<uint8_t>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    classProperty.noData = 0;
    view = PropertyView<PropertyArrayView<uint8_t>, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);

    classProperty.min = false;
    view = PropertyView<PropertyArrayView<uint8_t>, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMin);

    classProperty.max = JsonValue::Array{10.4, "30.0"};
    view = PropertyView<PropertyArrayView<uint8_t>, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMax);

    classProperty.scale =
        JsonValue::Array{JsonValue::Array{2.3}, JsonValue::Array{1.3}};
    view = PropertyView<PropertyArrayView<uint8_t>, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    classProperty.offset = "10";
    view = PropertyView<PropertyArrayView<uint8_t>, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }
}

TEST_CASE("VecN Array PropertyView") {
  SUBCASE("Constructs empty PropertyView") {
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

  SUBCASE("Reports type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC2;
    classProperty.array = true;

    PropertyView<PropertyArrayView<glm::vec3>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Reports component type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.array = true;

    PropertyView<PropertyArrayView<glm::vec3>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Reports array type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.array = false;

    PropertyView<PropertyArrayView<glm::vec3>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Reports invalid normalization") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.array = true;
    classProperty.normalized = true;

    PropertyView<PropertyArrayView<glm::vec3>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Constructs with count") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::INT16;
    classProperty.array = true;
    classProperty.count = 5;

    PropertyView<PropertyArrayView<glm::i16vec3>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.arrayCount() == classProperty.count);
  }

  SUBCASE("Constructs with offset, scale, max, and min") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.array = true;
    classProperty.count = 2;
    classProperty.offset = {{-1, 1, 2}, {4, 4, 0}};
    classProperty.scale = {{2, 1, 3}, {8, 2, 3}};
    classProperty.max = {{14, 28, 12}, {10, 5, 6}};
    classProperty.min = {{-11, -12, -13}, {-2, -4, 6}};

    PropertyView<PropertyArrayView<glm::vec3>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.offset());
    REQUIRE(view.scale());
    REQUIRE(view.max());
    REQUIRE(view.min());

    PropertyArrayView<glm::vec3> value = *view.offset();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == glm::vec3(-1, 1, 2));
    REQUIRE(value[1] == glm::vec3(4, 4, 0));

    value = *view.scale();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == glm::vec3(2, 1, 3));
    REQUIRE(value[1] == glm::vec3(8, 2, 3));

    value = *view.max();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == glm::vec3(14, 28, 12));
    REQUIRE(value[1] == glm::vec3(10, 5, 6));

    value = *view.min();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == glm::vec3(-11, -12, -13));
    REQUIRE(value[1] == glm::vec3(-2, -4, 6));
  }

  SUBCASE("Constructs with noData and defaultProperty") {
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

  SUBCASE("Reports errors for defined properties on variable-length arrays") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.array = true;
    classProperty.count = 0;
    classProperty.min = {{-11, -12, -13}, {-2, -4, 6}};

    PropertyView<PropertyArrayView<glm::vec3>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMin);

    classProperty.max = {{14, 28, 12}, {10, 5, 6}};
    view = PropertyView<PropertyArrayView<glm::vec3>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMax);

    classProperty.scale = {{2, 1, 3}, {8, 2, 3}};
    view = PropertyView<PropertyArrayView<glm::vec3>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    classProperty.offset = {{-1, 1, 2}, {4, 4, 0}};
    view = PropertyView<PropertyArrayView<glm::vec3>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }

  SUBCASE("Reports errors for incorrectly defined properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC2;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.array = true;
    classProperty.required = true;
    classProperty.defaultProperty = {{3, 4}, {5, 6}};

    PropertyView<PropertyArrayView<glm::ivec2>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    classProperty.noData = {{0, 0}, {1, 2}};
    view = PropertyView<PropertyArrayView<glm::ivec2>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);

    classProperty.scale = {{1, 1}, {-1, -1}};
    view = PropertyView<PropertyArrayView<glm::ivec2>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    classProperty.offset = {{0, 0}, {-4, 7}};
    view = PropertyView<PropertyArrayView<glm::ivec2>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }

  SUBCASE("Reports errors for out-of-bounds values") {
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

  SUBCASE("Reports errors for invalid types") {
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

TEST_CASE("VecN Array PropertyView (normalized)") {
  SUBCASE("Constructs empty PropertyView") {
    PropertyView<PropertyArrayView<glm::ivec2>, true> view;
    REQUIRE(view.status() == PropertyViewStatus::ErrorNonexistentProperty);
    REQUIRE(view.arrayCount() == 0);
    REQUIRE(view.normalized());
    REQUIRE(!view.offset());
    REQUIRE(!view.scale());
    REQUIRE(!view.max());
    REQUIRE(!view.min());
    REQUIRE(!view.required());
    REQUIRE(!view.noData());
    REQUIRE(!view.defaultValue());
  }

  SUBCASE("Reports type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC2;
    classProperty.array = true;

    PropertyView<PropertyArrayView<glm::ivec3>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Reports component type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.array = true;

    PropertyView<PropertyArrayView<glm::ivec3>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Reports array type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.array = false;

    PropertyView<PropertyArrayView<glm::ivec3>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Reports invalid normalization") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.array = true;
    classProperty.normalized = false;

    PropertyView<PropertyArrayView<glm::ivec3>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Constructs with count") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.array = true;
    classProperty.count = 5;
    classProperty.normalized = true;

    PropertyView<PropertyArrayView<glm::ivec3>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.arrayCount() == classProperty.count);
  }

  SUBCASE("Constructs with offset, scale, max, and min") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.array = true;
    classProperty.count = 2;
    classProperty.normalized = true;
    classProperty.offset = {{-1, 1, 2}, {4, 4, 0}};
    classProperty.scale = {{2, 1, 3}, {8, 2, 3}};
    classProperty.max = {{14, 28, 12}, {10, 5, 6}};
    classProperty.min = {{-11, -12, -13}, {-2, -4, 6}};

    PropertyView<PropertyArrayView<glm::ivec3>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.offset());
    REQUIRE(view.scale());
    REQUIRE(view.max());
    REQUIRE(view.min());

    PropertyArrayView<glm::dvec3> value = *view.offset();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == glm::dvec3(-1, 1, 2));
    REQUIRE(value[1] == glm::dvec3(4, 4, 0));

    value = *view.scale();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == glm::dvec3(2, 1, 3));
    REQUIRE(value[1] == glm::dvec3(8, 2, 3));

    value = *view.max();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == glm::dvec3(14, 28, 12));
    REQUIRE(value[1] == glm::dvec3(10, 5, 6));

    value = *view.min();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == glm::dvec3(-11, -12, -13));
    REQUIRE(value[1] == glm::dvec3(-2, -4, 6));
  }

  SUBCASE("Constructs with noData and defaultProperty") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC2;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.array = true;
    classProperty.normalized = true;
    classProperty.required = false;
    classProperty.noData = {{0, 0}, {1, 2}};
    classProperty.defaultProperty = {{3.5, 4.5}, {5.0, 6.0}};

    PropertyView<PropertyArrayView<glm::ivec2>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(!view.required());
    REQUIRE(view.noData());
    REQUIRE(view.defaultValue());

    PropertyArrayView<glm::ivec2> noData = *view.noData();
    REQUIRE(noData.size() == 2);
    REQUIRE(noData[0] == glm::ivec2(0, 0));
    REQUIRE(noData[1] == glm::ivec2(1, 2));

    PropertyArrayView<glm::dvec2> defaultValue = *view.defaultValue();
    REQUIRE(defaultValue.size() == 2);
    REQUIRE(defaultValue[0] == glm::dvec2(3.5, 4.5));
    REQUIRE(defaultValue[1] == glm::dvec2(5.0, 6.0));
  }

  SUBCASE("Reports errors for defined properties on variable-length arrays") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.array = true;
    classProperty.count = 0;
    classProperty.normalized = true;
    classProperty.min = {{-11, -12, -13}, {-2, -4, 6}};

    PropertyView<PropertyArrayView<glm::ivec3>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMin);

    classProperty.max = {{14, 28, 12}, {10, 5, 6}};
    view = PropertyView<PropertyArrayView<glm::ivec3>, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMax);

    classProperty.scale = {{2, 1, 3}, {8, 2, 3}};
    view = PropertyView<PropertyArrayView<glm::ivec3>, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    classProperty.offset = {{-1, 1, 2}, {4, 4, 0}};
    view = PropertyView<PropertyArrayView<glm::ivec3>, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }

  SUBCASE("Reports errors for incorrectly defined properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC2;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.array = true;
    classProperty.count = 2;
    classProperty.normalized = true;
    classProperty.required = true;
    classProperty.defaultProperty = {{3, 4}, {5, 6}};

    PropertyView<PropertyArrayView<glm::ivec2>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    classProperty.noData = {{0, 0}, {1, 2}};
    view = PropertyView<PropertyArrayView<glm::ivec2>, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);
  }

  SUBCASE("Reports errors for out-of-bounds values") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.normalized = true;
    classProperty.array = true;
    classProperty.noData = {{0, 0}, {-128, -129}};

    PropertyView<PropertyArrayView<glm::i8vec2>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);
  }

  SUBCASE("Reports errors for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.array = true;
    classProperty.normalized = true;
    classProperty.defaultProperty = {1, 20};

    PropertyView<PropertyArrayView<glm::i8vec2>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    classProperty.noData = JsonValue::Array{{2.0f, 5.4f}, "not a vec2"};
    view = PropertyView<PropertyArrayView<glm::i8vec2>, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);

    classProperty.min = {{-10, -1, 4}, {0, 0, 0}};
    view = PropertyView<PropertyArrayView<glm::i8vec2>, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMin);

    classProperty.max = {{10, 20, 30, 40}, {1, 2, 3, 4}};
    view = PropertyView<PropertyArrayView<glm::i8vec2>, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMax);

    classProperty.scale = 2;
    view = PropertyView<PropertyArrayView<glm::i8vec2>, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    classProperty.offset = "(1, 2)";
    view = PropertyView<PropertyArrayView<glm::i8vec2>, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }
}

TEST_CASE("MatN Array PropertyView") {
  SUBCASE("Constructs empty PropertyView") {
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

  SUBCASE("Reports type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT4;
    classProperty.array = true;

    PropertyView<PropertyArrayView<glm::mat2>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Reports component type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.array = true;

    PropertyView<PropertyArrayView<glm::mat2>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Reports array type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.array = false;

    PropertyView<PropertyArrayView<glm::mat2>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Reports invalid normalization") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.array = true;
    classProperty.normalized = true;

    PropertyView<PropertyArrayView<glm::mat2>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Constructs with count") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT3;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.array = true;
    classProperty.count = 5;

    PropertyView<PropertyArrayView<glm::imat3x3>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.arrayCount() == classProperty.count);
  }

  SUBCASE("Constructs with offset, scale, max, and min") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.array = true;
    classProperty.count = 2;
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

    PropertyView<PropertyArrayView<glm::mat2>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.offset());
    REQUIRE(view.scale());
    REQUIRE(view.max());
    REQUIRE(view.min());

    PropertyArrayView<glm::mat2> value = *view.offset();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == glm::mat2(-1, 1, 0, 2));
    REQUIRE(value[1] == glm::mat2(2, 40, 6, -8));

    value = *view.scale();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == glm::mat2(1, 1, 1, 0));
    REQUIRE(value[1] == glm::mat2(-2, 5, 7, 1));

    value = *view.max();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == glm::mat2(2, 4, 8, 0));
    REQUIRE(value[1] == glm::mat2(-7, 8, 4, 4));

    value = *view.min();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == glm::mat2(-1, -6, -1, 2));
    REQUIRE(value[1] == glm::mat2(0, 1, 2, 3));
  }

  SUBCASE("Constructs with noData and defaultProperty") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.array = true;
    classProperty.count = 2;
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

  SUBCASE("Reports errors for defined properties on variable-length arrays") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.array = true;
    classProperty.count = 0;
    // clang-format off
    classProperty.min = {
      {0, 0,
       0, 0},
      {-1, -1,
       -1, -1},
    };
    // clang-format on
    PropertyView<PropertyArrayView<glm::mat2>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMin);

    // clang-format off
    classProperty.max = {
      {1, 1,
       1, 1},
      {2, 2,
       2, 2},
    };
    // clang-format on
    view = PropertyView<PropertyArrayView<glm::mat2>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMax);

    // clang-format off
    classProperty.scale = {
      {1, 0,
       0, 1},
      {-1, 0,
       0, -1},
    };
    // clang-format on
    view = PropertyView<PropertyArrayView<glm::mat2>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    // clang-format off
    classProperty.offset = {
      {2, 2,
       1, 1},
      {0, 2,
       1, 2},
    };
    // clang-format on
    view = PropertyView<PropertyArrayView<glm::mat2>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }

  SUBCASE("Reports errors for incorrectly defined properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.array = true;
    classProperty.count = 2;
    classProperty.required = true;
    // clang-format off
    classProperty.defaultProperty = {
      {1, 1,
       1, 1},
      {2, 2,
       2, 2},
    };
    // clang-format on

    PropertyView<PropertyArrayView<glm::imat2x2>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    // clang-format off
    classProperty.noData = {
      {0, 0,
       0, 0},
      {-1, -1,
       -1, -1},
    };
    // clang-format on
    view = PropertyView<PropertyArrayView<glm::imat2x2>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);

    // clang-format off
    classProperty.scale = {
      {1, 0,
       0, 1},
      {-1, 0,
       0, -1},
    };
    // clang-format on
    view = PropertyView<PropertyArrayView<glm::imat2x2>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    // clang-format off
    classProperty.offset = {
      {2, 2,
       1, 1},
      {0, 2,
       1, 2},
    };
    // clang-format on
    view = PropertyView<PropertyArrayView<glm::imat2x2>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }

  SUBCASE("Reports errors for out-of-bounds values") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.array = true;
    classProperty.count = 2;

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

  SUBCASE("Reports errors for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.array = true;
    classProperty.count = 2;
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

TEST_CASE("MatN Array PropertyView (normalized)") {
  SUBCASE("Constructs empty PropertyView") {
    PropertyView<PropertyArrayView<glm::imat2x2>, true> view;
    REQUIRE(view.status() == PropertyViewStatus::ErrorNonexistentProperty);
    REQUIRE(view.arrayCount() == 0);
    REQUIRE(view.normalized());
    REQUIRE(!view.offset());
    REQUIRE(!view.scale());
    REQUIRE(!view.max());
    REQUIRE(!view.min());
    REQUIRE(!view.required());
    REQUIRE(!view.noData());
    REQUIRE(!view.defaultValue());
  }

  SUBCASE("Reports type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT4;
    classProperty.array = true;

    PropertyView<PropertyArrayView<glm::imat2x2>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Reports component type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.array = true;

    PropertyView<PropertyArrayView<glm::imat2x2>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Reports array type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.array = false;

    PropertyView<PropertyArrayView<glm::imat2x2>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Reports invalid normalization") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.array = true;
    classProperty.normalized = false;

    PropertyView<PropertyArrayView<glm::imat2x2>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Constructs with count") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT3;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.array = true;
    classProperty.count = 5;
    classProperty.normalized = true;

    PropertyView<PropertyArrayView<glm::imat3x3>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.arrayCount() == classProperty.count);
  }

  SUBCASE("Constructs with offset, scale, max, and min") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.array = true;
    classProperty.count = 2;
    classProperty.normalized = true;

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

    PropertyView<PropertyArrayView<glm::imat2x2>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.offset());
    REQUIRE(view.scale());
    REQUIRE(view.max());
    REQUIRE(view.min());

    PropertyArrayView<glm::dmat2> value = *view.offset();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == glm::dmat2(-1, 1, 0, 2));
    REQUIRE(value[1] == glm::dmat2(2, 40, 6, -8));

    value = *view.scale();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == glm::dmat2(1, 1, 1, 0));
    REQUIRE(value[1] == glm::dmat2(-2, 5, 7, 1));

    value = *view.max();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == glm::dmat2(2, 4, 8, 0));
    REQUIRE(value[1] == glm::dmat2(-7, 8, 4, 4));

    value = *view.min();
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == glm::dmat2(-1, -6, -1, 2));
    REQUIRE(value[1] == glm::dmat2(0, 1, 2, 3));
  }

  SUBCASE("Constructs with noData and defaultProperty") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.array = true;
    classProperty.count = 2;
    classProperty.normalized = true;
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

    PropertyView<PropertyArrayView<glm::i8mat2x2>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(!view.required());
    REQUIRE(view.noData());
    REQUIRE(view.defaultValue());

    PropertyArrayView<glm::i8mat2x2> noData = *view.noData();
    REQUIRE(noData.size() == 2);
    REQUIRE(noData[0] == glm::i8mat2x2(0, 0, 0, 0));
    REQUIRE(noData[1] == glm::i8mat2x2(-1, -1, -1, -1));

    PropertyArrayView<glm::dmat2> defaultValue = *view.defaultValue();
    REQUIRE(defaultValue.size() == 2);
    REQUIRE(defaultValue[0] == glm::dmat2(1, 1, 1, 1));
    REQUIRE(defaultValue[1] == glm::dmat2(2, 2, 2, 2));
  }
  SUBCASE("Reports errors for defined properties on variable-length arrays") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.array = true;
    classProperty.count = 0;
    classProperty.normalized = true;
    // clang-format off
    classProperty.min = {
      {0, 0,
       0, 0},
      {-1, -1,
       -1, -1},
    };

    // clang-format on
    PropertyView<PropertyArrayView<glm::imat2x2>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMin);

    // clang-format off
    classProperty.max = {
      {1, 1,
       1, 1},
      {2, 2,
       2, 2},
    };
    // clang-format on
    view = PropertyView<PropertyArrayView<glm::imat2x2>, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMax);

    // clang-format off
    classProperty.scale = {
      {1, 0,
       0, 1},
      {-1, 0,
       0, -1},
    };
    // clang-format on
    view = PropertyView<PropertyArrayView<glm::imat2x2>, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    // clang-format off
    classProperty.offset = {
      {2, 2,
       1, 1},
      {0, 2,
       1, 2},
    };
    // clang-format on
    view = PropertyView<PropertyArrayView<glm::imat2x2>, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }
  SUBCASE("Reports errors for incorrectly defined properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT32;
    classProperty.array = true;
    classProperty.count = 2;
    classProperty.normalized = true;
    classProperty.required = true;
    // clang-format off
    classProperty.defaultProperty = {
      {1, 1,
       1, 1},
      {2, 2,
       2, 2},
    };
    // clang-format on

    PropertyView<PropertyArrayView<glm::imat2x2>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    // clang-format off
    classProperty.noData = {
      {0, 0,
       0, 0},
      {-1, -1,
       -1, -1},
    };
    // clang-format on
    view = PropertyView<PropertyArrayView<glm::imat2x2>, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);
    ;
  }

  SUBCASE("Reports errors for out-of-bounds values") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.array = true;
    classProperty.normalized = true;

    // clang-format off
    classProperty.noData = {
      {0, 0,
       0, 0},
      {-140, -1,
       -1, -1},
    };
    // clang-format on
    PropertyView<PropertyArrayView<glm::i8mat2x2>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);
  }

  SUBCASE("Reports errors for invalid types") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::INT8;
    classProperty.array = true;
    classProperty.normalized = true;
    classProperty.defaultProperty = {4, 1, 2, 0};

    PropertyView<PropertyArrayView<glm::i8mat2x2>, true> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    // clang-format off
    classProperty.noData = {
      {0.45, 0.0,
       1.0, -1.4},
       "not a matrix"
    };
    // clang-format on
    view = PropertyView<PropertyArrayView<glm::i8mat2x2>, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);

    classProperty.min = {{0, 1, 2, 3, 4, 5, 6}};
    view = PropertyView<PropertyArrayView<glm::i8mat2x2>, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMin);

    classProperty.max = {{0, 1, 2, 3}, false};
    view = PropertyView<PropertyArrayView<glm::i8mat2x2>, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidMax);

    classProperty.scale = 1;
    view = PropertyView<PropertyArrayView<glm::i8mat2x2>, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidScale);

    classProperty.offset = "[(1, 2, 3, 4)]";
    view = PropertyView<PropertyArrayView<glm::i8mat2x2>, true>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidOffset);
  }
}

TEST_CASE("String Array PropertyView") {
  SUBCASE("Constructs empty PropertyView") {
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

  SUBCASE("Reports type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::BOOLEAN;
    classProperty.array = true;

    PropertyView<PropertyArrayView<std::string_view>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Reports array type mismatch") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.array = false;

    PropertyView<PropertyArrayView<std::string_view>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Constructs with count") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.array = true;
    classProperty.count = 5;

    PropertyView<PropertyArrayView<std::string_view>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::Valid);
    REQUIRE(view.arrayCount() == classProperty.count);
  }

  SUBCASE("Constructs with noData and defaultProperty") {
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

  SUBCASE("Reports errors for incorrectly defined properties") {
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.array = true;
    classProperty.required = true;
    classProperty.defaultProperty = {"default1", "default2"};

    PropertyView<PropertyArrayView<std::string_view>> view(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidDefaultValue);

    classProperty.noData = {"null", "0"};
    view = PropertyView<PropertyArrayView<std::string_view>>(classProperty);
    REQUIRE(view.status() == PropertyViewStatus::ErrorInvalidNoDataValue);
  }

  SUBCASE("Reports errors for invalid types") {
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
