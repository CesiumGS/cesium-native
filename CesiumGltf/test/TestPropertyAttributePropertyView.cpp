#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/ClassProperty.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/PropertyAttributeProperty.h>
#include <CesiumGltf/PropertyTransformations.h>
#include <CesiumGltf/PropertyType.h>
#include <CesiumGltf/PropertyTypeTraits.h>
#include <CesiumUtility/JsonValue.h>

#include <glm/ext/matrix_double2x2.hpp>
#include <glm/ext/matrix_float2x2.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_int2_sized.hpp>
#include <glm/ext/vector_uint3_sized.hpp>
#include <glm/fwd.hpp>

#include <cstdint>
#include <cstring>
#include <optional>
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

#include <CesiumGltf/PropertyAttributePropertyView.h>
#include <CesiumUtility/Assert.h>

#include <doctest/doctest.h>

#include <cstddef>
#include <vector>

using namespace CesiumGltf;
using namespace CesiumUtility;

namespace {

template <typename T, bool Normalized>
const Accessor& addValuesToModel(Model& model, const std::vector<T>& values) {
  Buffer& buffer = model.buffers.emplace_back();
  buffer.cesium.data.resize(values.size() * sizeof(T));
  buffer.byteLength = static_cast<int64_t>(buffer.cesium.data.size());
  std::memcpy(
      buffer.cesium.data.data(),
      values.data(),
      buffer.cesium.data.size());

  BufferView& bufferView = model.bufferViews.emplace_back();
  bufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
  bufferView.byteOffset = 0;
  bufferView.byteLength = buffer.byteLength;

  Accessor& accessor = model.accessors.emplace_back();
  accessor.bufferView = static_cast<int32_t>(model.bufferViews.size() - 1);
  accessor.count = static_cast<int64_t>(values.size());
  accessor.byteOffset = 0;

  PropertyType type = TypeToPropertyType<T>::value;
  switch (type) {
  case PropertyType::Scalar:
    accessor.type = Accessor::Type::SCALAR;
    break;
  case PropertyType::Vec2:
    accessor.type = Accessor::Type::VEC2;
    break;
  case PropertyType::Vec3:
    accessor.type = Accessor::Type::VEC3;
    break;
  case PropertyType::Vec4:
    accessor.type = Accessor::Type::VEC4;
    break;
  case PropertyType::Mat2:
    accessor.type = Accessor::Type::MAT2;
    break;
  case PropertyType::Mat3:
    accessor.type = Accessor::Type::MAT3;
    break;
  case PropertyType::Mat4:
    accessor.type = Accessor::Type::MAT4;
    break;
  default:
    CESIUM_ASSERT(false && "Input type is not supported as an accessor type");
    break;
  }

  PropertyComponentType componentType = TypeToPropertyType<T>::component;
  switch (componentType) {
  case PropertyComponentType::Int8:
    accessor.componentType = Accessor::ComponentType::BYTE;
    break;
  case PropertyComponentType::Uint8:
    accessor.componentType = Accessor::ComponentType::UNSIGNED_BYTE;
    break;
  case PropertyComponentType::Int16:
    accessor.componentType = Accessor::ComponentType::SHORT;
    break;
  case PropertyComponentType::Uint16:
    accessor.componentType = Accessor::ComponentType::UNSIGNED_SHORT;
    break;
  case PropertyComponentType::Float32:
    accessor.componentType = Accessor::ComponentType::FLOAT;
    break;
  default:
    CESIUM_ASSERT(
        false &&
        "Input component type is not supported as an accessor component type");
    break;
  }

  accessor.normalized = Normalized;

  return accessor;
}

template <typename T> void checkAttributeValues(const std::vector<T>& values) {
  Model model;
  const Accessor& accessor = addValuesToModel<T, false>(model, values);
  AccessorView accessorView = AccessorView<T>(model, accessor);

  PropertyAttributeProperty property;
  ClassProperty classProperty;
  classProperty.type =
      convertPropertyTypeToString(TypeToPropertyType<T>::value);

  PropertyComponentType componentType = TypeToPropertyType<T>::component;
  classProperty.componentType =
      convertPropertyComponentTypeToString(componentType);

  PropertyAttributePropertyView<T> view(property, classProperty, accessorView);

  REQUIRE(view.size() == static_cast<int64_t>(values.size()));
  REQUIRE(!view.normalized());

  for (int64_t i = 0; i < view.size(); i++) {
    REQUIRE(view.getRaw(i) == values[static_cast<size_t>(i)]);
    REQUIRE(view.get(i) == view.getRaw(i));
  }
}

template <typename T>
void checkAttributeValues(
    const std::vector<T>& values,
    const std::vector<std::optional<T>>& expected,
    const std::optional<JsonValue> offset = std::nullopt,
    const std::optional<JsonValue> scale = std::nullopt,
    const std::optional<JsonValue> noData = std::nullopt,
    const std::optional<JsonValue> defaultValue = std::nullopt) {
  Model model;
  const Accessor& accessor = addValuesToModel<T, false>(model, values);
  AccessorView accessorView = AccessorView<T>(model, accessor);

  PropertyAttributeProperty property;
  ClassProperty classProperty;
  classProperty.type =
      convertPropertyTypeToString(TypeToPropertyType<T>::value);

  PropertyComponentType componentType = TypeToPropertyType<T>::component;
  classProperty.componentType =
      convertPropertyComponentTypeToString(componentType);

  classProperty.offset = offset;
  classProperty.scale = scale;
  classProperty.noData = noData;
  classProperty.defaultProperty = defaultValue;

  PropertyAttributePropertyView<T> view(property, classProperty, accessorView);

  REQUIRE(view.size() == static_cast<int64_t>(values.size()));
  REQUIRE(!view.normalized());

  for (int64_t i = 0; i < view.size(); i++) {
    REQUIRE(view.getRaw(i) == values[static_cast<size_t>(i)]);
    REQUIRE(view.get(i) == expected[static_cast<size_t>(i)]);
  }
}

template <typename T, typename D = typename TypeToNormalizedType<T>::type>
void checkNormalizedAttributeValues(
    const std::vector<T>& values,
    const std::vector<std::optional<D>>& expected,
    const std::optional<JsonValue> offset = std::nullopt,
    const std::optional<JsonValue> scale = std::nullopt,
    const std::optional<JsonValue> noData = std::nullopt,
    const std::optional<JsonValue> defaultValue = std::nullopt) {
  Model model;
  const Accessor& accessor = addValuesToModel<T, true>(model, values);
  AccessorView accessorView = AccessorView<T>(model, accessor);

  PropertyAttributeProperty property;
  ClassProperty classProperty;
  classProperty.type =
      convertPropertyTypeToString(TypeToPropertyType<T>::value);

  PropertyComponentType componentType = TypeToPropertyType<T>::component;
  classProperty.componentType =
      convertPropertyComponentTypeToString(componentType);

  classProperty.normalized = true;

  classProperty.offset = offset;
  classProperty.scale = scale;
  classProperty.noData = noData;
  classProperty.defaultProperty = defaultValue;

  PropertyAttributePropertyView<T, true> view(
      property,
      classProperty,
      accessorView);

  REQUIRE(view.size() == static_cast<int64_t>(values.size()));
  REQUIRE(view.normalized());

  for (int64_t i = 0; i < view.size(); i++) {
    REQUIRE(view.getRaw(i) == values[static_cast<size_t>(i)]);
    REQUIRE(view.get(i) == expected[static_cast<size_t>(i)]);
  }
}
} // namespace

TEST_CASE("Check scalar PropertyAttributePropertyView") {
  SUBCASE("Uint8") {
    std::vector<uint8_t> data{12, 33, 56, 67};
    checkAttributeValues(data);
  }

  SUBCASE("Int16") {
    std::vector<int16_t> data{-1, -32511, 768, 438};
    checkAttributeValues(data);
  }

  SUBCASE("float") {
    std::vector<float> data{12.3333f, -12.44555f, -5.6111f, 6.7421f};
    checkAttributeValues(data);
  }

  SUBCASE("float with offset / scale") {
    std::vector<float> data{1.0f, 2.0f, 3.0f, 4.0f};

    const float offset = 1.0f;
    const float scale = 2.0f;

    std::vector<std::optional<float>> expected{3.0f, 5.0f, 7.0f, 9.0f};
    checkAttributeValues(data, expected, offset, scale);
  }

  SUBCASE("uint8_t with noData") {
    std::vector<uint8_t> data{12, 33, 0, 128, 0, 56, 67};
    const uint8_t noData = 0;
    std::vector<std::optional<uint8_t>> expected{
        data[0],
        data[1],
        std::nullopt,
        data[3],
        std::nullopt,
        data[5],
        data[6]};
    checkAttributeValues(data, expected, std::nullopt, std::nullopt, noData);
  }

  SUBCASE("uint8_t with noData and defaultValue") {
    std::vector<uint8_t> data{12, 33, 0, 128, 0, 56, 67};
    const uint8_t noData = 0;
    const uint8_t defaultValue = 255;
    std::vector<std::optional<uint8_t>> expected{
        data[0],
        data[1],
        defaultValue,
        data[3],
        defaultValue,
        data[5],
        data[6]};
    checkAttributeValues(
        data,
        expected,
        std::nullopt,
        std::nullopt,
        noData,
        defaultValue);
  }
}

TEST_CASE("Check scalar PropertyAttributePropertyView (normalized)") {
  SUBCASE("Uint8") {
    std::vector<uint8_t> data{12, 33, 56, 67};
    std::vector<std::optional<double>> expected{
        12.0 / 255.0,
        33 / 255.0,
        56 / 255.0,
        67 / 255.0};
    checkNormalizedAttributeValues(data, expected);
  }

  SUBCASE("Int16") {
    std::vector<int16_t> data{-1, -32511, 768, 438};
    std::vector<std::optional<double>> expected{
        normalize(data[0]),
        normalize(data[1]),
        normalize(data[2]),
        normalize(data[3])};
    checkNormalizedAttributeValues(data, expected);
  }

  SUBCASE("Uint8 with offset / scale") {
    std::vector<uint8_t> data{12, 33, 56, 67};
    const double offset = 1.0;
    const double scale = 2.0;
    std::vector<std::optional<double>> expected{
        normalize(data[0]) * scale + offset,
        normalize(data[1]) * scale + offset,
        normalize(data[2]) * scale + offset,
        normalize(data[3]) * scale + offset,
    };
    checkNormalizedAttributeValues(data, expected, offset, scale);
  }

  SUBCASE("Uint8 with all properties") {
    std::vector<uint8_t> data{12, 33, 56, 0, 67};
    const double offset = 1.0;
    const double scale = 2.0;
    const uint8_t noData = 0;
    const double defaultValue = 10.0;
    std::vector<std::optional<double>> expected{
        normalize(data[0]) * scale + offset,
        normalize(data[1]) * scale + offset,
        normalize(data[2]) * scale + offset,
        10.0,
        normalize(data[4]) * scale + offset,
    };
    checkNormalizedAttributeValues(
        data,
        expected,
        offset,
        scale,
        noData,
        defaultValue);
  }
}

TEST_CASE("Check vecN PropertyAttributePropertyView") {
  SUBCASE("glm::i8vec2") {
    std::vector<glm::i8vec2> data{
        glm::i8vec2(28, -1),
        glm::i8vec2(-2, 1),
        glm::i8vec2(0, 3),
        glm::i8vec2(-74, 1)};
    checkAttributeValues(data);
  }

  SUBCASE("glm::vec3") {
    std::vector<glm::vec3> data{
        glm::vec3(1.5f, 2.0f, -3.3f),
        glm::vec3(4.12f, -5.008f, 6.0f),
        glm::vec3(7.0f, 8.0f, 9.01f),
        glm::vec3(-0.28f, 5.0f, 1.2f)};
    checkAttributeValues(data);
  }

  SUBCASE("glm::vec3 with offset / scale") {
    std::vector<glm::vec3> data{
        glm::vec3(1.0f, 2.0f, 3.0f),
        glm::vec3(-1.0f, -2.0f, -3.0f),
        glm::vec3(0.0f),
        glm::vec3(1.0f)};

    JsonValue::Array offset{1.0f, 0.0f, -1.0f};
    JsonValue::Array scale{2.0f, 2.0f, 2.0f};

    std::vector<std::optional<glm::vec3>> expected{
        glm::vec3(3.0f, 4.0f, 5.0f),
        glm::vec3(-1.0f, -4.0f, -7.0f),
        glm::vec3(1.0f, 0.0f, -1.0f),
        glm::vec3(3.0f, 2.0f, 1.0f)};

    checkAttributeValues(data, expected, offset, scale);
  }

  SUBCASE("glm::i8vec2 with noData") {
    std::vector<glm::i8vec2> data{
        glm::i8vec2(28, -1),
        glm::i8vec2(-2, 1),
        glm::i8vec2(0, 3),
        glm::i8vec2(0),
        glm::i8vec2(-74, 1)};

    JsonValue::Array noData{0, 0};

    std::vector<std::optional<glm::i8vec2>> expected{
        glm::i8vec2(28, -1),
        glm::i8vec2(-2, 1),
        glm::i8vec2(0, 3),
        std::nullopt,
        glm::i8vec2(-74, 1)};
    checkAttributeValues(data, expected, std::nullopt, std::nullopt, noData);
  }

  SUBCASE("glm::i8vec2 with defaultValue") {
    std::vector<glm::i8vec2> data{
        glm::i8vec2(28, -1),
        glm::i8vec2(-2, 1),
        glm::i8vec2(0, 3),
        glm::i8vec2(0, 0),
        glm::i8vec2(-74, 1)};

    JsonValue::Array noData{0, 0};
    JsonValue::Array defaultValue{127, 127};

    std::vector<std::optional<glm::i8vec2>> expected{
        glm::i8vec2(28, -1),
        glm::i8vec2(-2, 1),
        glm::i8vec2(0, 3),
        glm::i8vec2(127, 127),
        glm::i8vec2(-74, 1)};
    checkAttributeValues(
        data,
        expected,
        std::nullopt,
        std::nullopt,
        noData,
        defaultValue);
  }
}

TEST_CASE("Check vecN PropertyAttributePropertyView (normalized)") {
  SUBCASE("glm::i8vec2") {
    std::vector<glm::i8vec2> data{
        glm::i8vec2(28, -1),
        glm::i8vec2(-2, 1),
        glm::i8vec2(0, 3),
        glm::i8vec2(-74, 1)};
    std::vector<std::optional<glm::dvec2>> expected{
        normalize(data[0]),
        normalize(data[1]),
        normalize(data[2]),
        normalize(data[3])};
    checkNormalizedAttributeValues(data, expected);
  }

  SUBCASE("glm::u8vec3") {
    std::vector<glm::u8vec3> data{
        glm::u8vec3(1, 2, 3),
        glm::u8vec3(4, 5, 6),
        glm::u8vec3(7, 8, 9),
        glm::u8vec3(0, 5, 2)};
    std::vector<std::optional<glm::dvec3>> expected{
        normalize(data[0]),
        normalize(data[1]),
        normalize(data[2]),
        normalize(data[3])};
    checkNormalizedAttributeValues(data, expected);
  }

  SUBCASE("glm::i8vec2 with offset / scale") {
    std::vector<glm::i8vec2> data{
        glm::i8vec2(28, -1),
        glm::i8vec2(-2, 1),
        glm::i8vec2(0, 3),
        glm::i8vec2(0),
        glm::i8vec2(-74, 1)};

    const glm::dvec2 offset(-1.0, 4.0);
    const glm::dvec2 scale(2.0, 1.0);

    std::vector<std::optional<glm::dvec2>> expected{
        normalize(data[0]) * scale + offset,
        normalize(data[1]) * scale + offset,
        normalize(data[2]) * scale + offset,
        normalize(data[3]) * scale + offset,
        normalize(data[4]) * scale + offset,
    };
    checkNormalizedAttributeValues(
        data,
        expected,
        JsonValue::Array{offset[0], offset[1]},
        JsonValue::Array{scale[0], scale[1]});
  }

  SUBCASE("glm::i8vec2 with all properties") {
    const glm::dvec2 offset(-1.0, 4.0);
    const glm::dvec2 scale(2.0, 1.0);
    const glm::i8vec2 noData(0);
    const glm::dvec2 defaultValue(100.0, 5.5);

    std::vector<glm::i8vec2> data{
        glm::i8vec2(28, -1),
        glm::i8vec2(-2, 1),
        glm::i8vec2(0, 3),
        glm::i8vec2(0),
        glm::i8vec2(-74, 1)};
    std::vector<std::optional<glm::dvec2>> expected{
        normalize(data[0]) * scale + offset,
        normalize(data[1]) * scale + offset,
        normalize(data[2]) * scale + offset,
        defaultValue,
        normalize(data[4]) * scale + offset,
    };
    checkNormalizedAttributeValues(
        data,
        expected,
        JsonValue::Array{offset[0], offset[1]},
        JsonValue::Array{scale[0], scale[1]},
        JsonValue::Array{noData[0], noData[1]},
        JsonValue::Array{defaultValue[0], defaultValue[1]});
  }
}

TEST_CASE("Check matN PropertyAttributePropertyView") {
  SUBCASE("Float Mat2") {
    // clang-format off
    std::vector<glm::mat2> data{
        glm::mat2(
          1.0f, 2.0f,
          3.0f, 4.0f),
        glm::mat2(
          -10.0f, 40.0f,
           0.08f, 5.4f),
        glm::mat2(
          9.99f, -2.0f,
          -0.4f, 0.23f)
    };
    // clang-format on
    checkAttributeValues(data);
  }

  SUBCASE("Int16 Mat3") {
    // clang-format off
    std::vector<glm::i16mat3x3> data{
        glm::i16mat3x3(
          1, 2, 3,
          4, 5, 6,
          7, 8, 9),
        glm::i16mat3x3(
           10,  0,  5,
          -14, 35, 16,
           -2,  3,  4),
        glm::i16mat3x3(
          -6,  5,   2,
          14,  4, -33,
           2,  1,   0)
    };
    // clang-format on
    checkAttributeValues(data);
  }

  SUBCASE("Float Mat4") {
    // clang-format off
    std::vector<glm::mat4> data{
        glm::mat4(
           1.0f,  2.0f,  3.0f,  4.0f,
           5.0f,  6.0f,  7.0f,  8.0f,
           9.0f, 10.0f, 11.0f, 12.0f,
          13.0f, 14.0f, 15.0f, 16.0f),
        glm::mat4(
           0.1f,   0.2f,   0.3f,   0.4f,
           0.5f,   0.6f,   0.7f,   0.8f,
          -9.0f, -10.0f, -11.0f, -12.0f,
          13.0f,  14.0f,  15.0f,  16.0f),
        glm::mat4(
          1.0f, 0.0f,  0.0f, 10.0f,
          0.0f, 0.0f, -1.0f, -3.5f,
          0.0f, 1.0f,  0.0f, 20.4f,
          0.0f, 0.0f,  0.0f,  1.0f)
    };
    // clang-format on
    checkAttributeValues(data);
  }

  SUBCASE("Float Mat2 with Offset / Scale") {
    // clang-format off
    std::vector<glm::mat2> data{
        glm::mat2(
          1.0f, 3.0f,
          4.0f, 2.0f),
        glm::mat2(
          6.5f, 2.0f,
          -2.0f, 0.0f),
        glm::mat2(
          8.0f, -1.0f,
          -3.0f, 1.0f),
    };
    const JsonValue::Array offset{
      1.0f, 2.0f,
      3.0f, 1.0f};
    const JsonValue::Array scale {
      2.0f, 0.0f,
      0.0f, 2.0f};

    std::vector<std::optional<glm::mat2>> expected{
        glm::mat2(
          3.0f, 2.0f,
          3.0f, 5.0f),
        glm::mat2(
          14.0f, 2.0f,
          3.0f, 1.0f),
        glm::mat2(
          17.0f, 2.0f,
          3.0f, 3.0f),
    };
    // clang-format on
    checkAttributeValues(data, expected, offset, scale);
  }

  SUBCASE("Int16 Mat3 with NoData") {
    // clang-format off
    std::vector<glm::i16mat3x3> data{
        glm::i16mat3x3(
           1,  2,  3,
          -1, -2, -3,
           0,  1,  0),
        glm::i16mat3x3(
          1, -1, 0,
          0,  1, 2,
          0,  4, 5),
        glm::i16mat3x3(
          -1, -1, -1,
           0,  0,  0,
           1,  1,  1)};
    JsonValue::Array noData{
          -1, -1, -1,
          0, 0, 0,
          1, 1, 1};
    // clang-format on
    std::vector<std::optional<glm::i16mat3x3>> expected{
        data[0],
        data[1],
        std::nullopt};
    checkAttributeValues(
        data,
        expected,
        std::nullopt,
        std::nullopt,
        noData,
        std::nullopt);
  };

  SUBCASE("Int16 Mat3 with NoData and DefaultValue") {
    // clang-format off
    std::vector<glm::i16mat3x3> data{
        glm::i16mat3x3(
           1,  2,  3,
          -1, -2, -3,
           0,  1,  0),
        glm::i16mat3x3(
          1, -1, 0,
          0,  1, 2,
          0,  4, 5),
        glm::i16mat3x3(
          -1, -1, -1,
           0,  0,  0,
           1,  1,  1)};
    JsonValue::Array noData{
      -1, -1, -1,
       0,  0,  0,
       1,  1,  1};
    JsonValue::Array defaultValue{
      1, 0, 0,
      0, 1, 0,
      0, 0, 1};
    // clang-format on
    std::vector<std::optional<glm::i16mat3x3>> expected{
        data[0],
        data[1],
        glm::i16mat3x3(1)};
    checkAttributeValues(
        data,
        expected,
        std::nullopt,
        std::nullopt,
        noData,
        defaultValue);
  };
}

TEST_CASE("Check matN PropertyAttributePropertyView (normalized)") {
  SUBCASE("Normalized Uint8 Mat2") {
    // clang-format off
    std::vector<glm::u8mat2x2> data{
        glm::u8mat2x2(
          0, 64,
          255, 255),
        glm::u8mat2x2(
          255, 0,
          128, 0)};
    std::vector<std::optional<glm::dmat2>> expected{
        glm::dmat2(
          0.0, 64.0 / 255.0,
          1.0, 1.0),
        glm::dmat2(
          1.0, 0.0,
          128.0 / 255.0, 0.0)};
    // clang-format on
    checkNormalizedAttributeValues(data, expected);
  }

  SUBCASE("Normalized Int16 Mat2") {
    // clang-format off
    std::vector<glm::i16mat2x2> data{
        glm::i16mat2x2(
          -32768, 0,
          16384, 32767),
        glm::i16mat2x2(
          0, 32767,
          32767, -32768)};
    std::vector<std::optional<glm::dmat2>> expected{
        glm::dmat2(
          -1.0, 0.0,
          16384.0 / 32767.0, 1.0),
        glm::dmat2(
          0.0, 1.0,
          1.0, -1.0),
    };
    // clang-format on
    checkNormalizedAttributeValues(data, expected);
  }

  SUBCASE("Normalized Uint8 Mat2 with Offset and Scale") {
    // clang-format off
    std::vector<glm::u8mat2x2> values{
        glm::u8mat2x2(
          0, 64,
          255, 255),
        glm::u8mat2x2(
          255, 0,
          128, 0)};
    JsonValue::Array offset{
      0.0, 1.0,
      1.0, 0.0};
    JsonValue::Array scale{
      2.0, 1.0,
      0.0, 2.0};
    std::vector<std::optional<glm::dmat2>> expected{
        glm::dmat2(
          0.0, 1 + 64.0 / 255.0,
          1.0, 2.0),
        glm::dmat2(
          2.0, 1.0,
          1.0, 0.0)};
    // clang-format on
    checkNormalizedAttributeValues(values, expected, offset, scale);
  }

  SUBCASE("Normalized Uint8 Mat2 with all properties") {
    // clang-format off
    std::vector<glm::u8mat2x2> values{
        glm::u8mat2x2(
          0, 64,
          255, 255),
        glm::u8mat2x2(0),
        glm::u8mat2x2(
          255, 0,
          128, 0)};
    JsonValue::Array offset{
      0.0, 1.0,
      1.0, 0.0};
    JsonValue::Array scale{
      2.0, 1.0,
      0.0, 2.0};
    JsonValue::Array noData{
      0, 0,
      0, 0};
    JsonValue::Array defaultValue{
      1.0, 0.0,
      0.0, 1.0};

    std::vector<std::optional<glm::dmat2>> expected{
        glm::dmat2(
          0.0, 1 + 64.0 / 255.0,
          1.0, 2.0),
        glm::dmat2(1.0),
        glm::dmat2(
          2.0, 1.0,
          1.0, 0.0)};
    // clang-format on
    checkNormalizedAttributeValues(
        values,
        expected,
        offset,
        scale,
        noData,
        defaultValue);
  }
}

TEST_CASE("Check that PropertyAttributeProperty values override class property "
          "values") {
  Model model;
  std::vector<float> data{1.0f, 2.0f, 3.0f, 4.0f};

  Buffer& buffer = model.buffers.emplace_back();
  buffer.cesium.data.resize(data.size() * sizeof(float));
  buffer.byteLength = static_cast<int64_t>(buffer.cesium.data.size());
  std::memcpy(
      buffer.cesium.data.data(),
      data.data(),
      buffer.cesium.data.size());

  BufferView& bufferView = model.bufferViews.emplace_back();
  bufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
  bufferView.byteOffset = 0;
  bufferView.byteLength = buffer.byteLength;

  Accessor& accessor = model.accessors.emplace_back();
  accessor.bufferView = static_cast<int32_t>(model.bufferViews.size() - 1);
  accessor.count = static_cast<int64_t>(data.size());
  accessor.byteOffset = 0;
  accessor.type = Accessor::Type::SCALAR;
  accessor.componentType = Accessor::ComponentType::FLOAT;

  AccessorView accessorView = AccessorView<float>(model, accessor);

  ClassProperty classProperty;
  classProperty.type = ClassProperty::Type::SCALAR;
  classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

  classProperty.offset = 0.0f;
  classProperty.scale = 1.0f;
  classProperty.min = -10.0f;
  classProperty.max = 10.0f;

  const float offset = 1.0f;
  const float scale = 2.0f;
  const float min = 3.0f;
  const float max = 9.0f;

  std::vector<std::optional<float>> expected{3.0f, 5.0f, 7.0f, 9.0f};

  PropertyAttributeProperty property;
  property.offset = offset;
  property.scale = scale;
  property.min = min;
  property.max = max;

  PropertyAttributePropertyView<float> view(
      property,
      classProperty,
      accessorView);
  REQUIRE(view.offset() == offset);
  REQUIRE(view.scale() == scale);
  REQUIRE(view.min() == min);
  REQUIRE(view.max() == max);

  REQUIRE(view.size() == static_cast<int64_t>(data.size()));
  for (int64_t i = 0; i < view.size(); i++) {
    REQUIRE(view.getRaw(i) == data[static_cast<size_t>(i)]);
    REQUIRE(view.get(i) == expected[static_cast<size_t>(i)]);
  }
}

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
