#include <CesiumGltf/ClassProperty.h>
#include <CesiumGltf/PropertyArrayView.h>
#include <CesiumGltf/PropertyTableProperty.h>
#include <CesiumGltf/PropertyType.h>
#include <CesiumGltf/PropertyTypeTraits.h>
#include <CesiumUtility/JsonValue.h>

#include <glm/ext/matrix_double2x2.hpp>
#include <glm/ext/matrix_double3x3.hpp>
#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/matrix_float2x2.hpp>
#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/ext/vector_int2_sized.hpp>
#include <glm/ext/vector_int3.hpp>
#include <glm/ext/vector_int3_sized.hpp>
#include <glm/ext/vector_uint2_sized.hpp>
#include <glm/ext/vector_uint4_sized.hpp>
#include <glm/fwd.hpp>

#include <cstdint>
#include <optional>
#include <string_view>
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

#include <CesiumGltf/PropertyTablePropertyView.h>

#include <doctest/doctest.h>

#include <bitset>
#include <climits>
#include <cstddef>
#include <cstring>
#include <ostream>
#include <span>
#include <string>
#include <vector>

using namespace doctest;
using namespace CesiumGltf;
using namespace CesiumUtility;

namespace {

template <typename T>
static void
checkArrayEqual(PropertyArrayView<T> arrayView, std::vector<T> expected) {
  REQUIRE(arrayView.size() == static_cast<int64_t>(expected.size()));
  for (int64_t i = 0; i < arrayView.size(); i++) {
    REQUIRE(arrayView[i] == expected[static_cast<size_t>(i)]);
  }
}

template <typename T> static void checkNumeric(const std::vector<T>& expected) {
  std::vector<std::byte> data;
  data.resize(expected.size() * sizeof(T));
  std::memcpy(data.data(), expected.data(), data.size());

  PropertyTableProperty propertyTableProperty;
  ClassProperty classProperty;
  classProperty.type =
      convertPropertyTypeToString(TypeToPropertyType<T>::value);

  PropertyComponentType componentType = TypeToPropertyType<T>::component;
  if (componentType != PropertyComponentType::None) {
    classProperty.componentType =
        convertPropertyComponentTypeToString(componentType);
  }

  PropertyTablePropertyView<T> property(
      propertyTableProperty,
      classProperty,
      static_cast<int64_t>(expected.size()),
      std::span<const std::byte>(data.data(), data.size()));

  REQUIRE(property.size() == static_cast<int64_t>(expected.size()));

  for (int64_t i = 0; i < property.size(); ++i) {
    REQUIRE(property.getRaw(i) == expected[static_cast<size_t>(i)]);
    REQUIRE(property.get(i) == property.getRaw(i));
  }
}

template <typename T>
static void checkNumeric(
    const std::vector<T>& values,
    const std::vector<std::optional<T>>& expected,
    const std::optional<JsonValue>& offset = std::nullopt,
    const std::optional<JsonValue>& scale = std::nullopt,
    const std::optional<JsonValue>& noData = std::nullopt,
    const std::optional<JsonValue>& defaultValue = std::nullopt) {
  std::vector<std::byte> data;
  data.resize(values.size() * sizeof(T));
  std::memcpy(data.data(), values.data(), data.size());

  PropertyTableProperty propertyTableProperty;
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

  PropertyTablePropertyView<T> property(
      propertyTableProperty,
      classProperty,
      static_cast<int64_t>(expected.size()),
      std::span<const std::byte>(data.data(), data.size()));

  REQUIRE(property.size() == static_cast<int64_t>(expected.size()));
  REQUIRE(!property.normalized());

  for (int64_t i = 0; i < property.size(); ++i) {
    REQUIRE(property.getRaw(i) == values[static_cast<size_t>(i)]);
    if constexpr (IsMetadataFloating<T>::value) {
      REQUIRE(property.get(i) == Approx(*expected[static_cast<size_t>(i)]));
    } else {
      REQUIRE(property.get(i) == expected[static_cast<size_t>(i)]);
    }
  }
}

template <typename T, typename D = typename TypeToNormalizedType<T>::type>
static void checkNormalizedNumeric(
    const std::vector<T>& values,
    const std::vector<std::optional<D>>& expected,
    const std::optional<JsonValue>& offset = std::nullopt,
    const std::optional<JsonValue>& scale = std::nullopt,
    const std::optional<JsonValue>& noData = std::nullopt,
    const std::optional<JsonValue>& defaultValue = std::nullopt) {
  std::vector<std::byte> data;
  data.resize(values.size() * sizeof(T));
  std::memcpy(data.data(), values.data(), data.size());

  PropertyTableProperty propertyTableProperty;
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

  PropertyTablePropertyView<T, true> property(
      propertyTableProperty,
      classProperty,
      static_cast<int64_t>(expected.size()),
      std::span<const std::byte>(data.data(), data.size()));

  REQUIRE(property.size() == static_cast<int64_t>(expected.size()));
  REQUIRE(property.normalized());

  for (int64_t i = 0; i < property.size(); ++i) {
    REQUIRE(property.getRaw(i) == values[static_cast<size_t>(i)]);
    REQUIRE(property.get(i) == expected[static_cast<size_t>(i)]);
  }
}

template <typename DataType, typename OffsetType>
static void checkVariableLengthArray(
    const std::vector<DataType>& data,
    const std::vector<OffsetType>& offsets,
    PropertyComponentType offsetType,
    int64_t instanceCount) {
  // copy data to buffer
  std::vector<std::byte> buffer;
  buffer.resize(data.size() * sizeof(DataType));
  std::memcpy(buffer.data(), data.data(), data.size() * sizeof(DataType));

  // copy offset to buffer
  std::vector<std::byte> offsetBuffer;
  offsetBuffer.resize(offsets.size() * sizeof(OffsetType));
  std::memcpy(
      offsetBuffer.data(),
      offsets.data(),
      offsets.size() * sizeof(OffsetType));

  PropertyTableProperty propertyTableProperty;
  ClassProperty classProperty;
  classProperty.type =
      convertPropertyTypeToString(TypeToPropertyType<DataType>::value);

  PropertyComponentType componentType = TypeToPropertyType<DataType>::component;
  if (componentType != PropertyComponentType::None) {
    classProperty.componentType =
        convertPropertyComponentTypeToString(componentType);
  }

  classProperty.array = true;

  PropertyTablePropertyView<PropertyArrayView<DataType>> property(
      propertyTableProperty,
      classProperty,
      instanceCount,
      std::span<const std::byte>(buffer.data(), buffer.size()),
      std::span<const std::byte>(offsetBuffer.data(), offsetBuffer.size()),
      std::span<const std::byte>(),
      offsetType,
      PropertyComponentType::None);

  REQUIRE(property.arrayCount() == 0);

  size_t expectedIdx = 0;
  for (int64_t i = 0; i < property.size(); ++i) {
    PropertyArrayView<DataType> values = property.getRaw(i);
    for (int64_t j = 0; j < values.size(); ++j) {
      REQUIRE(values[j] == data[expectedIdx]);
      ++expectedIdx;
    }

    auto maybeValues = property.get(i);
    REQUIRE(maybeValues);
    for (int64_t j = 0; j < values.size(); ++j) {
      REQUIRE((*maybeValues)[j] == values[j]);
    }
  }

  REQUIRE(expectedIdx == data.size());
}

template <typename DataType, typename OffsetType>
static void checkVariableLengthArray(
    const std::vector<DataType>& data,
    const std::vector<OffsetType>& offsets,
    PropertyComponentType offsetType,
    int64_t instanceCount,
    const std::vector<std::optional<std::vector<DataType>>>& expected,
    const std::optional<JsonValue::Array>& noData = std::nullopt,
    const std::optional<JsonValue::Array>& defaultValue = std::nullopt) {
  // copy data to buffer
  std::vector<std::byte> buffer;
  buffer.resize(data.size() * sizeof(DataType));
  std::memcpy(buffer.data(), data.data(), data.size() * sizeof(DataType));

  // copy offset to buffer
  std::vector<std::byte> offsetBuffer;
  offsetBuffer.resize(offsets.size() * sizeof(OffsetType));
  std::memcpy(
      offsetBuffer.data(),
      offsets.data(),
      offsets.size() * sizeof(OffsetType));

  PropertyTableProperty propertyTableProperty;
  ClassProperty classProperty;
  classProperty.type =
      convertPropertyTypeToString(TypeToPropertyType<DataType>::value);

  PropertyComponentType componentType = TypeToPropertyType<DataType>::component;
  if (componentType != PropertyComponentType::None) {
    classProperty.componentType =
        convertPropertyComponentTypeToString(componentType);
  }

  classProperty.array = true;
  classProperty.noData = noData;
  classProperty.defaultProperty = defaultValue;

  PropertyTablePropertyView<PropertyArrayView<DataType>> property(
      propertyTableProperty,
      classProperty,
      instanceCount,
      std::span<const std::byte>(buffer.data(), buffer.size()),
      std::span<const std::byte>(offsetBuffer.data(), offsetBuffer.size()),
      std::span<const std::byte>(),
      offsetType,
      PropertyComponentType::None);

  REQUIRE(property.arrayCount() == 0);

  // Check raw values first
  size_t expectedIdx = 0;
  for (int64_t i = 0; i < property.size(); ++i) {
    PropertyArrayView<DataType> values = property.getRaw(i);
    for (int64_t j = 0; j < values.size(); ++j) {
      REQUIRE(values[j] == data[expectedIdx]);
      ++expectedIdx;
    }
  }

  REQUIRE(expectedIdx == data.size());

  // Check values with properties applied
  for (int64_t i = 0; i < property.size(); ++i) {
    std::optional<PropertyArrayCopy<DataType>> maybeValues = property.get(i);
    if (!maybeValues) {
      REQUIRE(!expected[static_cast<size_t>(i)]);
      continue;
    }

    auto values = *maybeValues;
    auto expectedValues = *expected[static_cast<size_t>(i)];

    for (int64_t j = 0; j < values.size(); ++j) {
      REQUIRE(values[j] == expectedValues[static_cast<size_t>(j)]);
    }
  }
}

template <
    typename DataType,
    typename OffsetType,
    typename NormalizedType = typename TypeToNormalizedType<DataType>::type>
static void checkNormalizedVariableLengthArray(
    const std::vector<DataType>& data,
    const std::vector<OffsetType> offsets,
    PropertyComponentType offsetType,
    int64_t instanceCount,
    const std::vector<std::optional<std::vector<NormalizedType>>>& expected,
    const std::optional<JsonValue::Array>& noData = std::nullopt,
    const std::optional<JsonValue::Array>& defaultValue = std::nullopt) {
  // copy data to buffer
  std::vector<std::byte> buffer;
  buffer.resize(data.size() * sizeof(DataType));
  std::memcpy(buffer.data(), data.data(), data.size() * sizeof(DataType));

  // copy offset to buffer
  std::vector<std::byte> offsetBuffer;
  offsetBuffer.resize(offsets.size() * sizeof(OffsetType));
  std::memcpy(
      offsetBuffer.data(),
      offsets.data(),
      offsets.size() * sizeof(OffsetType));

  PropertyTableProperty propertyTableProperty;
  ClassProperty classProperty;
  classProperty.type =
      convertPropertyTypeToString(TypeToPropertyType<DataType>::value);

  PropertyComponentType componentType = TypeToPropertyType<DataType>::component;
  classProperty.componentType =
      convertPropertyComponentTypeToString(componentType);

  classProperty.array = true;
  classProperty.normalized = true;
  classProperty.noData = noData;
  classProperty.defaultProperty = defaultValue;

  PropertyTablePropertyView<PropertyArrayView<DataType>, true> property(
      propertyTableProperty,
      classProperty,
      instanceCount,
      std::span<const std::byte>(buffer.data(), buffer.size()),
      std::span<const std::byte>(offsetBuffer.data(), offsetBuffer.size()),
      offsetType);

  REQUIRE(property.arrayCount() == 0);

  // Check raw values first
  size_t expectedIdx = 0;
  for (int64_t i = 0; i < property.size(); ++i) {
    PropertyArrayView<DataType> values = property.getRaw(i);
    for (int64_t j = 0; j < values.size(); ++j) {
      REQUIRE(values[j] == data[expectedIdx]);
      ++expectedIdx;
    }
  }

  REQUIRE(expectedIdx == data.size());

  // Check values with properties applied
  for (int64_t i = 0; i < property.size(); ++i) {
    std::optional<PropertyArrayCopy<NormalizedType>> maybeValues =
        property.get(i);
    if (!maybeValues) {
      REQUIRE(!expected[static_cast<size_t>(i)]);
      continue;
    }

    auto values = *maybeValues;
    auto expectedValues = *expected[static_cast<size_t>(i)];

    for (int64_t j = 0; j < values.size(); ++j) {
      REQUIRE(values[j] == expectedValues[static_cast<size_t>(j)]);
    }
  }
}

template <typename T>
static void checkFixedLengthArray(
    const std::vector<T>& data,
    int64_t fixedLengthArrayCount) {
  int64_t instanceCount =
      static_cast<int64_t>(data.size()) / fixedLengthArrayCount;

  std::vector<std::byte> buffer;
  buffer.resize(data.size() * sizeof(T));
  std::memcpy(buffer.data(), data.data(), buffer.size());

  PropertyTableProperty propertyTableProperty;
  ClassProperty classProperty;
  classProperty.type =
      convertPropertyTypeToString(TypeToPropertyType<T>::value);

  PropertyComponentType componentType = TypeToPropertyType<T>::component;
  if (componentType != PropertyComponentType::None) {
    classProperty.componentType =
        convertPropertyComponentTypeToString(componentType);
  }

  classProperty.array = true;
  classProperty.count = fixedLengthArrayCount;

  PropertyTablePropertyView<PropertyArrayView<T>> property(
      propertyTableProperty,
      classProperty,
      instanceCount,
      std::span<const std::byte>(buffer.data(), buffer.size()),
      std::span<const std::byte>(),
      std::span<const std::byte>(),
      PropertyComponentType::None,
      PropertyComponentType::None);

  REQUIRE(property.arrayCount() == fixedLengthArrayCount);

  size_t expectedIdx = 0;
  for (int64_t i = 0; i < property.size(); ++i) {
    PropertyArrayView<T> values = property.getRaw(i);
    for (int64_t j = 0; j < values.size(); ++j) {
      REQUIRE(values[j] == data[expectedIdx]);
      ++expectedIdx;
    }

    auto maybeValues = property.get(i);
    REQUIRE(maybeValues);
    for (int64_t j = 0; j < values.size(); ++j) {
      REQUIRE((*maybeValues)[j] == values[j]);
    }
  }

  REQUIRE(expectedIdx == data.size());
}

template <typename T>
static void checkFixedLengthArray(
    const std::vector<T>& data,
    int64_t fixedLengthArrayCount,
    const std::vector<std::optional<std::vector<T>>>& expected,
    const std::optional<JsonValue::Array>& offset = std::nullopt,
    const std::optional<JsonValue::Array>& scale = std::nullopt,
    const std::optional<JsonValue::Array>& noData = std::nullopt,
    const std::optional<JsonValue::Array>& defaultValue = std::nullopt) {
  int64_t instanceCount =
      static_cast<int64_t>(data.size()) / fixedLengthArrayCount;

  std::vector<std::byte> buffer;
  buffer.resize(data.size() * sizeof(T));
  std::memcpy(buffer.data(), data.data(), buffer.size());

  PropertyTableProperty propertyTableProperty;
  ClassProperty classProperty;
  classProperty.type =
      convertPropertyTypeToString(TypeToPropertyType<T>::value);

  PropertyComponentType componentType = TypeToPropertyType<T>::component;
  if (componentType != PropertyComponentType::None) {
    classProperty.componentType =
        convertPropertyComponentTypeToString(componentType);
  }

  classProperty.array = true;
  classProperty.count = fixedLengthArrayCount;
  classProperty.offset = offset;
  classProperty.scale = scale;
  classProperty.noData = noData;
  classProperty.defaultProperty = defaultValue;

  PropertyTablePropertyView<PropertyArrayView<T>> property(
      propertyTableProperty,
      classProperty,
      instanceCount,
      std::span<const std::byte>(buffer.data(), buffer.size()),
      std::span<const std::byte>(),
      std::span<const std::byte>(),
      PropertyComponentType::None,
      PropertyComponentType::None);

  REQUIRE(property.arrayCount() == fixedLengthArrayCount);

  // Check raw values first
  size_t expectedIdx = 0;
  for (int64_t i = 0; i < property.size(); ++i) {
    PropertyArrayView<T> rawValues = property.getRaw(i);
    for (int64_t j = 0; j < rawValues.size(); ++j) {
      REQUIRE(rawValues[j] == data[expectedIdx]);
      ++expectedIdx;
    }
  }

  REQUIRE(expectedIdx == data.size());

  // Check values with properties applied
  for (int64_t i = 0; i < property.size(); ++i) {
    std::optional<PropertyArrayCopy<T>> maybeValues = property.get(i);
    if (!maybeValues) {
      REQUIRE(!expected[static_cast<size_t>(i)]);
      continue;
    }

    auto values = *maybeValues;
    auto expectedValues = *expected[static_cast<size_t>(i)];

    for (int64_t j = 0; j < values.size(); ++j) {
      REQUIRE(values[j] == expectedValues[static_cast<size_t>(j)]);
    }
  }
}

template <typename T, typename D = typename TypeToNormalizedType<T>::type>
static void checkNormalizedFixedLengthArray(
    const std::vector<T>& data,
    int64_t fixedLengthArrayCount,
    const std::vector<std::optional<std::vector<D>>>& expected,
    const std::optional<JsonValue::Array>& offset = std::nullopt,
    const std::optional<JsonValue::Array>& scale = std::nullopt,
    const std::optional<JsonValue::Array>& noData = std::nullopt,
    const std::optional<JsonValue::Array>& defaultValue = std::nullopt) {
  int64_t instanceCount =
      static_cast<int64_t>(data.size()) / fixedLengthArrayCount;

  std::vector<std::byte> buffer;
  buffer.resize(data.size() * sizeof(T));
  std::memcpy(buffer.data(), data.data(), buffer.size());

  PropertyTableProperty propertyTableProperty;
  ClassProperty classProperty;
  classProperty.type =
      convertPropertyTypeToString(TypeToPropertyType<T>::value);

  PropertyComponentType componentType = TypeToPropertyType<T>::component;
  classProperty.componentType =
      convertPropertyComponentTypeToString(componentType);

  classProperty.array = true;
  classProperty.count = fixedLengthArrayCount;
  classProperty.normalized = true;

  classProperty.offset = offset;
  classProperty.scale = scale;
  classProperty.noData = noData;
  classProperty.defaultProperty = defaultValue;

  PropertyTablePropertyView<PropertyArrayView<T>, true> property(
      propertyTableProperty,
      classProperty,
      instanceCount,
      std::span<const std::byte>(buffer.data(), buffer.size()),
      std::span<const std::byte>(),
      PropertyComponentType::None);

  REQUIRE(property.arrayCount() == fixedLengthArrayCount);

  // Check raw values first
  size_t expectedIdx = 0;
  for (int64_t i = 0; i < property.size(); ++i) {
    PropertyArrayView<T> rawValues = property.getRaw(i);
    for (int64_t j = 0; j < rawValues.size(); ++j) {
      REQUIRE(rawValues[j] == data[expectedIdx]);
      ++expectedIdx;
    }
  }

  REQUIRE(expectedIdx == data.size());

  // Check values with properties applied
  for (int64_t i = 0; i < property.size(); ++i) {
    std::optional<PropertyArrayCopy<D>> maybeValues = property.get(i);
    if (!maybeValues) {
      REQUIRE(!expected[static_cast<size_t>(i)]);
      continue;
    }

    auto values = *maybeValues;
    auto expectedValues = *expected[static_cast<size_t>(i)];

    for (int64_t j = 0; j < values.size(); ++j) {
      REQUIRE(values[j] == expectedValues[static_cast<size_t>(j)]);
    }
  }
}
} // namespace

TEST_CASE("Check scalar PropertyTablePropertyView") {
  SUBCASE("Uint8") {
    std::vector<uint8_t> data{12, 33, 56, 67};
    checkNumeric(data);
  }

  SUBCASE("Int32") {
    std::vector<int32_t> data{111222, -11133, -56000, 670000};
    checkNumeric(data);
  }

  SUBCASE("Float") {
    std::vector<float> data{12.3333f, -12.44555f, -5.6111f, 6.7421f};
    checkNumeric(data);
  }

  SUBCASE("Double") {
    std::vector<double> data{
        12222.3302121,
        -12000.44555,
        -5000.6113111,
        6.7421};
    checkNumeric(data);
  }

  SUBCASE("Float with Offset / Scale") {
    std::vector<float> values{12.5f, -12.5f, -5.0f, 6.75f};
    const float offset = 1.0f;
    const float scale = 2.0f;
    std::vector<std::optional<float>> expected{26.0f, -24.0f, -9.0f, 14.5f};
    checkNumeric(values, expected, offset, scale);
  }

  SUBCASE("Int16 with NoData") {
    std::vector<int16_t> values{-1, 3, 7, -1};
    const int16_t noData = -1;
    std::vector<std::optional<int16_t>> expected{
        std::nullopt,
        static_cast<int16_t>(3),
        static_cast<int16_t>(7),
        std::nullopt};
    checkNumeric<int16_t>(
        values,
        expected,
        std::nullopt,
        std::nullopt,
        noData,
        std::nullopt);
  };

  SUBCASE("Int16 with NoData and Default") {
    std::vector<int16_t> values{-1, 3, 7, -1};
    const int16_t noData = -1;
    const int16_t defaultValue = 0;
    std::vector<std::optional<int16_t>> expected{
        static_cast<int16_t>(0),
        static_cast<int16_t>(3),
        static_cast<int16_t>(7),
        static_cast<int16_t>(0)};
    checkNumeric<int16_t>(
        values,
        expected,
        std::nullopt,
        std::nullopt,
        noData,
        defaultValue);
  }

  SUBCASE("Overrides class property values") {
    std::vector<float> values{1.0f, 3.0f, 2.0f, 4.0f};
    std::vector<std::byte> data;
    data.resize(values.size() * sizeof(float));
    std::memcpy(data.data(), values.data(), data.size());

    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.offset = 0.0f;
    classProperty.scale = 1.0f;
    classProperty.min = 1.0f;
    classProperty.max = 4.0f;

    PropertyTableProperty propertyTableProperty;
    propertyTableProperty.offset = 1.0f;
    propertyTableProperty.scale = 2.0f;
    propertyTableProperty.min = 3.0f;
    propertyTableProperty.max = 9.0f;

    PropertyTablePropertyView<float> property(
        propertyTableProperty,
        classProperty,
        static_cast<int64_t>(values.size()),
        std::span<const std::byte>(data.data(), data.size()));

    REQUIRE(property.offset() == 1.0f);
    REQUIRE(property.scale() == 2.0f);
    REQUIRE(property.min() == 3.0f);
    REQUIRE(property.max() == 9.0f);

    std::vector<float> expected{3.0, 7.0f, 5.0f, 9.0f};
    for (int64_t i = 0; i < property.size(); ++i) {
      REQUIRE(property.getRaw(i) == values[static_cast<size_t>(i)]);
      REQUIRE(property.get(i) == Approx(expected[static_cast<size_t>(i)]));
    }
  }
}

TEST_CASE("Check scalar PropertyTablePropertyView (normalized)") {
  SUBCASE("Uint8") {
    std::vector<uint8_t> values{0, 64, 128, 255};
    std::vector<std::optional<double>> expected{
        0.0,
        64.0 / 255.0,
        128.0 / 255.0,
        1.0};
    checkNormalizedNumeric(values, expected);
  }

  SUBCASE("Int16") {
    std::vector<int16_t> values{-32768, 0, 16384, 32767};
    std::vector<std::optional<double>> expected{
        -1.0,
        0.0,
        16384.0 / 32767.0,
        1.0};
    checkNormalizedNumeric(values, expected);
  }

  SUBCASE("Uint8 with Offset and Scale") {
    std::vector<uint8_t> values{0, 64, 128, 255};
    const double offset = 1.0;
    const double scale = 2.0;
    std::vector<std::optional<double>> expected{
        1.0,
        1 + 2 * (64.0 / 255.0),
        1 + 2 * (128.0 / 255.0),
        3.0};
    checkNormalizedNumeric(values, expected, offset, scale);
  }

  SUBCASE("Uint8 with all properties") {
    std::vector<uint8_t> values{0, 64, 128, 255};
    const double offset = 1.0;
    const double scale = 2.0;
    const uint8_t noData = 0;
    const double defaultValue = 10.0;
    std::vector<std::optional<double>> expected{
        10.0,
        1 + 2 * (64.0 / 255.0),
        1 + 2 * (128.0 / 255.0),
        3.0};
    checkNormalizedNumeric(
        values,
        expected,
        offset,
        scale,
        noData,
        defaultValue);
  }
}

TEST_CASE("Check vecN PropertyTablePropertyView") {
  SUBCASE("Float Vec2") {
    std::vector<glm::vec2> data{
        glm::vec2(10.001f, 0.005f),
        glm::vec2(-20.8f, 50.0f),
        glm::vec2(99.9f, -9.9f),
        glm::vec2(-64.456f, 12.01f)};
    checkNumeric(data);
  }

  SUBCASE("Int32 Vec3") {
    std::vector<glm::ivec3> data{
        glm::ivec3(10, 0, -3),
        glm::ivec3(-20, 10, 52),
        glm::ivec3(9, 9, -9),
        glm::ivec3(8, -40, 2)};
    checkNumeric(data);
  }

  SUBCASE("Uint8 Vec4") {
    std::vector<glm::u8vec4> data{
        glm::u8vec4(1, 2, 3, 0),
        glm::u8vec4(4, 5, 6, 1),
        glm::u8vec4(7, 8, 9, 0),
        glm::u8vec4(0, 0, 0, 1)};
    checkNumeric(data);
  }

  SUBCASE("Float Vec3 with Offset / Scale") {
    std::vector<glm::vec3> values{
        glm::vec3(0.0f, -1.5f, -5.0f),
        glm::vec3(6.5f, 2.0f, 4.0f),
        glm::vec3(8.0f, -3.0f, 1.0f),
    };
    JsonValue::Array offset{1.0f, 2.0f, 3.0f};
    JsonValue::Array scale{2.0f, 1.0f, 2.0f};
    std::vector<std::optional<glm::vec3>> expected{
        glm::vec3(1.0f, 0.5f, -7.0f),
        glm::vec3(14.0f, 4.0f, 11.0f),
        glm::vec3(17.0f, -1.0f, 5.0f),
    };
    checkNumeric(values, expected, offset, scale);
  }

  SUBCASE("Int16 Vec2 with NoData") {
    std::vector<glm::i16vec2> values{
        glm::i16vec2(-1, 3),
        glm::i16vec2(-1, -1),
        glm::i16vec2(7, 0)};
    JsonValue::Array noData{-1, -1};
    std::vector<std::optional<glm::i16vec2>> expected{
        glm::i16vec2(-1, 3),
        std::nullopt,
        glm::i16vec2(7, 0)};
    checkNumeric(
        values,
        expected,
        std::nullopt,
        std::nullopt,
        noData,
        std::nullopt);
  };

  SUBCASE("Int16 Vec2 with NoData and Default") {
    std::vector<glm::i16vec2> values{
        glm::i16vec2(-1, 3),
        glm::i16vec2(-1, -1),
        glm::i16vec2(7, 0)};
    JsonValue::Array noData{-1, -1};
    JsonValue::Array defaultValue{0, 1};
    std::vector<std::optional<glm::i16vec2>> expected{
        glm::i16vec2(-1, 3),
        glm::i16vec2(0, 1),
        glm::i16vec2(7, 0)};
    checkNumeric(
        values,
        expected,
        std::nullopt,
        std::nullopt,
        noData,
        defaultValue);
  };

  SUBCASE("Overrides class property values") {
    std::vector<glm::vec2> values{
        glm::vec2(1.0f, 3.0f),
        glm::vec2(2.5f, 2.5f),
        glm::vec2(2.0f, 4.0f)};
    std::vector<std::byte> data;
    data.resize(values.size() * sizeof(glm::vec2));
    std::memcpy(data.data(), values.data(), data.size());

    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC2;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    classProperty.offset = {0.0f, 0.0f};
    classProperty.scale = {1.0f, 1.0f};
    classProperty.min = {1.0f, 2.5f};
    classProperty.max = {2.5f, 4.0f};

    PropertyTableProperty propertyTableProperty;
    propertyTableProperty.offset = {1.0f, 0.5f};
    propertyTableProperty.scale = {2.0f, 1.0f};
    propertyTableProperty.min = {3.0f, 3.0f};
    propertyTableProperty.max = {6.0f, 4.5f};

    PropertyTablePropertyView<glm::vec2> property(
        propertyTableProperty,
        classProperty,
        static_cast<int64_t>(values.size()),
        std::span<const std::byte>(data.data(), data.size()));

    REQUIRE(property.offset() == glm::vec2(1.0f, 0.5f));
    REQUIRE(property.scale() == glm::vec2(2.0f, 1.0f));
    REQUIRE(property.min() == glm::vec2(3.0f, 3.0f));
    REQUIRE(property.max() == glm::vec2(6.0f, 4.5f));

    std::vector<glm::vec2> expected{
        glm::vec2(3.0f, 3.5f),
        glm::vec2(6.0f, 3.0f),
        glm::vec2(5.0f, 4.5f)};
    for (int64_t i = 0; i < property.size(); ++i) {
      REQUIRE(property.getRaw(i) == values[static_cast<size_t>(i)]);
      REQUIRE(property.get(i) == expected[static_cast<size_t>(i)]);
    }
  }
}

TEST_CASE("Check vecN PropertyTablePropertyView (normalized)") {
  SUBCASE("Uint8 Vec2") {
    std::vector<glm::u8vec2> values{
        glm::u8vec2(0, 64),
        glm::u8vec2(128, 255),
        glm::u8vec2(255, 0)};
    std::vector<std::optional<glm::dvec2>> expected{
        glm::dvec2(0.0, 64.0 / 255.0),
        glm::dvec2(128.0 / 255.0, 1.0),
        glm::dvec2(1.0, 0.0)};
    checkNormalizedNumeric(values, expected);
  }

  SUBCASE("Int16 Vec2") {
    std::vector<glm::i16vec2> values{
        glm::i16vec2(-32768, 0),
        glm::i16vec2(16384, 32767),
        glm::i16vec2(32767, -32768)};
    std::vector<std::optional<glm::dvec2>> expected{
        glm::dvec2(-1.0, 0.0),
        glm::dvec2(16384.0 / 32767.0, 1.0),
        glm::dvec2(1.0, -1.0),
    };
    checkNormalizedNumeric(values, expected);
  }

  SUBCASE("Uint8 Vec2 with Offset and Scale") {
    std::vector<glm::u8vec2> values{
        glm::u8vec2(0, 64),
        glm::u8vec2(128, 255),
        glm::u8vec2(255, 0)};
    JsonValue::Array offset{0.0, 1.0};
    JsonValue::Array scale{2.0, 1.0};
    std::vector<std::optional<glm::dvec2>> expected{
        glm::dvec2(0.0, 1 + (64.0 / 255.0)),
        glm::dvec2(2 * (128.0 / 255.0), 2.0),
        glm::dvec2(2.0, 1.0)};
    checkNormalizedNumeric(values, expected, offset, scale);
  }

  SUBCASE("Uint8 Vec2 with all properties") {
    std::vector<glm::u8vec2> values{
        glm::u8vec2(0, 64),
        glm::u8vec2(128, 255),
        glm::u8vec2(255, 0),
        glm::u8vec2(0, 0)};
    JsonValue::Array offset{0.0, 1.0};
    JsonValue::Array scale{2.0, 1.0};
    JsonValue::Array noData{0, 0};
    JsonValue::Array defaultValue{5.0, 15.0};
    std::vector<std::optional<glm::dvec2>> expected{
        glm::dvec2(0.0, 1 + (64.0 / 255.0)),
        glm::dvec2(2 * (128.0 / 255.0), 2.0),
        glm::dvec2(2.0, 1.0),
        glm::dvec2(5.0, 15.0)};
    checkNormalizedNumeric(
        values,
        expected,
        offset,
        scale,
        noData,
        defaultValue);
  }
}

TEST_CASE("Check matN PropertyTablePropertyView") {
  SUBCASE("Float Mat2") {
    // clang-format off
    std::vector<glm::mat2> data{
        glm::mat2(
          1.0f,  2.0f,
          3.0f,  4.0f),
        glm::mat2(
          -10.0f, 40.0f,
           0.08f,  5.4f),
        glm::mat2(
          9.99f, -2.0f,
          -0.4f, 0.23f)
    };
    // clang-format on
    checkNumeric(data);
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
    checkNumeric(data);
  }

  SUBCASE("Double Mat4") {
    // clang-format off
    std::vector<glm::dmat4> data{
        glm::dmat4(
           1.0,  2.0,  3.0,  4.0,
           5.0,  6.0,  7.0,  8.0,
           9.0, 10.0, 11.0, 12.0,
          13.0, 14.0, 15.0, 16.0),
        glm::dmat4(
           0.1,   0.2,   0.3,   0.4,
           0.5,   0.6,   0.7,   0.8,
          -9.0, -10.0, -11.0, -12.0,
          13.0,  14.0,  15.0,  16.0),
        glm::dmat4(
          1.0, 0.0,  0.0, 10.0,
          0.0, 0.0, -1.0, -3.5,
          0.0, 1.0,  0.0, 20.4,
          0.0, 0.0,  0.0,  1.0)
    };
    // clang-format on
    checkNumeric(data);
  }

  SUBCASE("Float Mat2 with Offset / Scale") {
    // clang-format off
    std::vector<glm::mat2> values{
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
    JsonValue::Array offset{
      1.0f, 2.0f,
      3.0f, 1.0f};
    JsonValue::Array scale {
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
    checkNumeric(values, expected, offset, scale);
  }

  SUBCASE("Int16 Mat3 with NoData") {
    // clang-format off
    std::vector<glm::i16mat3x3> values{
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
        values[0],
        values[1],
        std::nullopt};
    checkNumeric(
        values,
        expected,
        std::nullopt,
        std::nullopt,
        noData,
        std::nullopt);
  };

  SUBCASE("Int16 Mat3 with NoData and DefaultValue") {
    // clang-format off
    std::vector<glm::i16mat3x3> values{
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
        values[0],
        values[1],
        glm::i16mat3x3(1)};
    checkNumeric(
        values,
        expected,
        std::nullopt,
        std::nullopt,
        noData,
        defaultValue);
  };

  SUBCASE("Overrides class property values") {
    // clang-fomrat off
    std::vector<glm::mat2> values{
        glm::mat2(1.0f),
        glm::mat2(2.5f, 1.0f, 1.0f, 2.5f),
        glm::mat2(3.0f)};
    // clang-format on
    std::vector<std::byte> data;
    data.resize(values.size() * sizeof(glm::mat2));
    std::memcpy(data.data(), values.data(), data.size());

    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;
    // clang-format off
    classProperty.offset = {
      0.0f, 0.0f,
      0.0f, 0.0f};
    classProperty.scale = {
      1.0f, 1.0f,
      1.0f, 1.0f};
    classProperty.min = {
      1.0f, 0.0f,
      0.0f, 1.0f};
    classProperty.max = {
      3.0f, 0.0f,
      0.0f, 3.0f};
    // clang-format on

    PropertyTableProperty propertyTableProperty;
    // clang-format off
    propertyTableProperty.offset = {
      1.0f, 0.5f,
      0.5f, 1.0f};
    propertyTableProperty.scale = {
      2.0f, 1.0f,
      0.0f, 1.0f};
    propertyTableProperty.min = {
      3.0f, 0.5f,
      0.5f, 2.0f};
    propertyTableProperty.max = {
      7.0f, 1.5f,
      0.5f, 4.0f};
    // clang-format on

    PropertyTablePropertyView<glm::mat2> property(
        propertyTableProperty,
        classProperty,
        static_cast<int64_t>(values.size()),
        std::span<const std::byte>(data.data(), data.size()));

    // clang-format off
    REQUIRE(property.offset() == glm::mat2(
      1.0f, 0.5f,
      0.5f, 1.0f));
    REQUIRE(property.scale() == glm::mat2(
      2.0f, 1.0f,
      0.0f, 1.0f));
    REQUIRE(property.min() == glm::mat2(
      3.0f, 0.5f,
      0.5f, 2.0f));
    REQUIRE(property.max() == glm::mat2(
      7.0f, 1.5f,
      0.5f, 4.0f));

    std::vector<glm::mat2> expected{
        glm::mat2(
          3.0f, 0.5f,
          0.5f, 2.0f),
        glm::mat2(
          6.0f, 1.5f,
          0.5f, 3.5f),
        glm::mat2(
          7.0f, 0.5f,
          0.5f, 4.0f)};
    // clang-format on
    for (int64_t i = 0; i < property.size(); ++i) {
      REQUIRE(property.getRaw(i) == values[static_cast<size_t>(i)]);
      REQUIRE(property.get(i) == expected[static_cast<size_t>(i)]);
    }
  }
}

TEST_CASE("Check matN PropertyTablePropertyView (normalized)") {
  SUBCASE("Uint8 Mat2") {
    // clang-format off
    std::vector<glm::u8mat2x2> values{
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
    checkNormalizedNumeric(values, expected);
  }

  SUBCASE("Int16 Mat2") {
    // clang-format off
    std::vector<glm::i16mat2x2> values{
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
    checkNormalizedNumeric(values, expected);
  }

  SUBCASE("Uint8 Mat2 with Offset and Scale") {
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
    checkNormalizedNumeric(values, expected, offset, scale);
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
    checkNormalizedNumeric(
        values,
        expected,
        offset,
        scale,
        noData,
        defaultValue);
  }
}

TEST_CASE("Check boolean PropertyTablePropertyView") {
  std::bitset<sizeof(unsigned long)* CHAR_BIT> bits = 0b11110101;
  unsigned long val = bits.to_ulong();
  std::vector<std::byte> data(sizeof(val));
  std::memcpy(data.data(), &val, sizeof(val));

  PropertyTableProperty propertyTableProperty;
  ClassProperty classProperty;
  classProperty.type = ClassProperty::Type::BOOLEAN;

  size_t instanceCount = sizeof(unsigned long) * CHAR_BIT;
  PropertyTablePropertyView<bool> property(
      propertyTableProperty,
      classProperty,
      static_cast<int64_t>(instanceCount),
      std::span<const std::byte>(data.data(), data.size()));

  for (int64_t i = 0; i < property.size(); ++i) {
    REQUIRE(property.getRaw(i) == bits[static_cast<size_t>(i)]);
    REQUIRE(property.get(i) == property.getRaw(i));
  }
}

TEST_CASE("Check string PropertyTablePropertyView") {
  std::vector<std::string> strings{
      "This is a fine test",
      "What's going on",
      "Good morning"};
  size_t totalSize = 0;
  for (const auto& s : strings) {
    totalSize += s.size();
  }

  uint32_t currentOffset = 0;
  std::vector<std::byte> buffer;
  buffer.resize(totalSize);
  for (size_t i = 0; i < strings.size(); ++i) {
    std::memcpy(
        buffer.data() + currentOffset,
        strings[i].data(),
        strings[i].size());
    currentOffset += static_cast<uint32_t>(strings[i].size());
  }

  // copy offset to buffer
  std::vector<std::byte> offsetBuffer;
  offsetBuffer.resize((strings.size() + 1) * sizeof(uint32_t));
  currentOffset = 0;
  for (size_t i = 0; i < strings.size(); ++i) {
    std::memcpy(
        offsetBuffer.data() + i * sizeof(uint32_t),
        &currentOffset,
        sizeof(uint32_t));
    currentOffset += static_cast<uint32_t>(strings[i].size());
  }
  std::memcpy(
      offsetBuffer.data() + strings.size() * sizeof(uint32_t),
      &currentOffset,
      sizeof(uint32_t));

  SUBCASE("Returns correct values") {
    PropertyTableProperty propertyTableProperty;
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;

    PropertyTablePropertyView<std::string_view> property(
        propertyTableProperty,
        classProperty,
        static_cast<int64_t>(strings.size()),
        std::span<const std::byte>(buffer.data(), buffer.size()),
        std::span<const std::byte>(),
        std::span<const std::byte>(offsetBuffer.data(), offsetBuffer.size()),
        PropertyComponentType::None,
        PropertyComponentType::Uint32);

    for (int64_t i = 0; i < property.size(); ++i) {
      REQUIRE(property.getRaw(i) == strings[static_cast<size_t>(i)]);
      REQUIRE(property.get(i) == strings[static_cast<size_t>(i)]);
    }
  }

  SUBCASE("Uses NoData value") {
    PropertyTableProperty propertyTableProperty;
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.noData = "What's going on";

    PropertyTablePropertyView<std::string_view> property(
        propertyTableProperty,
        classProperty,
        static_cast<int64_t>(strings.size()),
        std::span<const std::byte>(buffer.data(), buffer.size()),
        std::span<const std::byte>(),
        std::span<const std::byte>(offsetBuffer.data(), offsetBuffer.size()),
        PropertyComponentType::None,
        PropertyComponentType::Uint32);

    std::vector<std::optional<std::string_view>> expected{
        strings[0],
        std::nullopt,
        strings[2]};

    for (int64_t i = 0; i < property.size(); ++i) {
      REQUIRE(property.getRaw(i) == strings[static_cast<size_t>(i)]);
      REQUIRE(property.get(i) == expected[static_cast<size_t>(i)]);
    }
  }

  SUBCASE("Uses NoData and Default value") {
    PropertyTableProperty propertyTableProperty;
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.noData = "What's going on";
    classProperty.defaultProperty = "Hello";

    PropertyTablePropertyView<std::string_view> property(
        propertyTableProperty,
        classProperty,
        static_cast<int64_t>(strings.size()),
        std::span<const std::byte>(buffer.data(), buffer.size()),
        std::span<const std::byte>(),
        std::span<const std::byte>(offsetBuffer.data(), offsetBuffer.size()),
        PropertyComponentType::None,
        PropertyComponentType::Uint32);

    std::vector<std::optional<std::string_view>> expected{
        strings[0],
        "Hello",
        strings[2]};

    for (int64_t i = 0; i < property.size(); ++i) {
      REQUIRE(property.getRaw(i) == strings[static_cast<size_t>(i)]);
      REQUIRE(property.get(i) == expected[static_cast<size_t>(i)]);
    }
  }
}

TEST_CASE("Check fixed-length scalar array PropertyTablePropertyView") {
  SUBCASE("Array of 4 uint8_ts") {
    // clang-format off
    std::vector<uint8_t> data{
        210, 211, 3, 42,
        122, 22, 1, 45};
    // clang-format on
    checkFixedLengthArray(data, 4);
  }

  SUBCASE("Array of 3 int8_ts") {
    // clang-format off
    std::vector<int8_t> data{
        122, -12, 3,
        44, 11, -2,
        5, 6, -22,
        5, 6, 1};
    // clang-format on
    checkFixedLengthArray(data, 3);
  }

  SUBCASE("Array of 4 int16_ts") {
    // clang-format off
    std::vector<int16_t> data{
        -122, 12, 3, 44,
        11, 2, 5, -6000,
        119, 30, 51, 200,
        22000, -500, 6000, 1};
    // clang-format on
    checkFixedLengthArray(data, 4);
  }

  SUBCASE("Array of 6 uint32_ts") {
    // clang-format off
    std::vector<uint32_t> data{
        122, 12, 3, 44, 34444, 2222,
        11, 2, 5, 6000, 1111, 2222,
        119, 30, 51, 200, 12534, 11,
        22000, 500, 6000, 1, 3, 7};
    // clang-format on
    checkFixedLengthArray(data, 6);
  }

  SUBCASE("Array of 2 int32_ts") {
    // clang-format off
    std::vector<int32_t> data{
        122, 12,
        -3, 44};
    // clang-format on
    checkFixedLengthArray(data, 2);
  }

  SUBCASE("Array of 4 uint64_ts") {
    // clang-format off
    std::vector<uint64_t> data{
        10022, 120000, 2422, 1111,
        3, 440000, 333, 1455};
    // clang-format on
    checkFixedLengthArray(data, 4);
  }

  SUBCASE("Array of 4 int64_ts") {
    // clang-format off
    std::vector<int64_t> data{
        10022, -120000, 2422, 1111,
        3, 440000, -333, 1455};
    // clang-format on
    checkFixedLengthArray(data, 4);
  }

  SUBCASE("Array of 4 floats") {
    // clang-format off
    std::vector<float> data{
        10.022f, -12.43f, 242.2f, 1.111f,
        3.333f, 440000.1f, -33.3f, 14.55f};
    // clang-format on
    checkFixedLengthArray(data, 4);
  }

  SUBCASE("Array of 4 double") {
    // clang-format off
    std::vector<double> data{
        10.022, -12.43, 242.2, 1.111,
        3.333, 440000.1, -33.3, 14.55};
    // clang-format on
    checkFixedLengthArray(data, 4);
  }

  SUBCASE("Array of 4 uint8_ts") {
    // clang-format off
    std::vector<uint8_t> data{
        210, 211, 3, 42,
        122, 22, 1, 45};
    // clang-format on
    checkFixedLengthArray(data, 4);
  }

  SUBCASE("Array of 4 floats with offset / scale") {
    // clang-format off
    std::vector<float> data{
      1.0f, 2.0f, 3.0f, 4.0f,
      5.0f, -1.0f, 0.0f, 2.0f
    };
    // clang-format on

    JsonValue::Array offset{1.0f, 0.0f, -1.0f, 0.0f};
    JsonValue::Array scale{1.0f, 2.0f, 1.0f, 2.0f};

    std::vector<std::optional<std::vector<float>>> expected{
        std::vector<float>{2.0f, 4.0f, 2.0f, 8.0f},
        std::vector<float>{6.0f, -2.0f, -1.0f, 4.0f}};
    checkFixedLengthArray(data, 4, expected, offset, scale);
  }

  SUBCASE("Array of 2 int32_ts with noData value") {
    // clang-format off
    std::vector<int32_t> data{
        122, 12,
        -1, -1,
        -3, 44};
    // clang-format on
    JsonValue::Array noData{-1, -1};
    std::vector<std::optional<std::vector<int32_t>>> expected{
        std::vector<int32_t>{122, 12},
        std::nullopt,
        std::vector<int32_t>{-3, 44}};
    checkFixedLengthArray<int32_t>(
        data,
        2,
        expected,
        std::nullopt,
        std::nullopt,
        noData,
        std::nullopt);
  }

  SUBCASE("Array of 2 int32_ts with noData and default value") {
    // clang-format off
    std::vector<int32_t> data{
        122, 12,
        -1, -1,
        -3, 44};
    // clang-format on
    JsonValue::Array noData{-1, -1};
    JsonValue::Array defaultValue{0, 1};
    std::vector<std::optional<std::vector<int32_t>>> expected{
        std::vector<int32_t>{122, 12},
        std::vector<int32_t>{0, 1},
        std::vector<int32_t>{-3, 44}};
    checkFixedLengthArray<int32_t>(
        data,
        2,
        expected,
        std::nullopt,
        std::nullopt,
        noData,
        defaultValue);
  }

  SUBCASE("Overrides class property values") {
    // clang-format off
    std::vector<float> data{
        1.0f, 2.0f, 3.0f, 4.0f,
        0.0f, -1.0f, 1.0f, -2.0f};
    // clang-format on
    const int64_t count = 4;
    const int64_t instanceCount = 2;

    std::vector<std::byte> buffer;
    buffer.resize(data.size() * sizeof(float));
    std::memcpy(buffer.data(), data.data(), buffer.size());

    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

    classProperty.array = true;
    classProperty.count = count;
    classProperty.offset = {0, 0, 0, 0};
    classProperty.scale = {1, 1, 1, 1};
    classProperty.min = {0.0f, -1.0f, 1.0f, -2.0f};
    classProperty.max = {1.0f, 2.0f, 3.0f, 4.0f};

    PropertyTableProperty propertyTableProperty;
    propertyTableProperty.offset = {2, 1, 0, -1};
    propertyTableProperty.scale = {1, 0, 1, -1};
    propertyTableProperty.min = {0.0f, 1.0f, 1.0f, -2.0f};
    propertyTableProperty.max = {1.0f, 1.0f, 3.0f, 4.0f};

    PropertyTablePropertyView<PropertyArrayView<float>> property(
        propertyTableProperty,
        classProperty,
        instanceCount,
        std::span<const std::byte>(buffer.data(), buffer.size()),
        std::span<const std::byte>(),
        std::span<const std::byte>(),
        PropertyComponentType::None,
        PropertyComponentType::None);

    REQUIRE(property.arrayCount() == count);
    REQUIRE(property.size() == instanceCount);

    REQUIRE(property.offset());
    checkArrayEqual(*property.offset(), {2, 1, 0, -1});

    REQUIRE(property.scale());
    checkArrayEqual(*property.scale(), {1, 0, 1, -1});

    REQUIRE(property.min());
    checkArrayEqual(*property.min(), {0.0f, 1.0f, 1.0f, -2.0f});

    REQUIRE(property.max());
    checkArrayEqual(*property.max(), {1.0f, 1.0f, 3.0f, 4.0f});

    std::vector<std::vector<float>> expected{
        {3.0f, 1.0, 3.0f, -5.0f},
        {2.0f, 1.0f, 1.0f, 1.0f}};
    size_t expectedIdx = 0;
    for (int64_t i = 0; i < property.size(); ++i) {
      PropertyArrayView<float> rawValues = property.getRaw(i);
      auto values = property.get(i);
      REQUIRE(values);
      for (int64_t j = 0; j < rawValues.size(); ++j) {
        REQUIRE(rawValues[j] == data[expectedIdx]);
        REQUIRE(
            (*values)[j] ==
            expected[static_cast<size_t>(i)][static_cast<size_t>(j)]);
        ++expectedIdx;
      }
    }
  }
}

TEST_CASE(
    "Check fixed-length scalar array PropertyTablePropertyView (normalized)") {
  SUBCASE("Array of 4 uint8_ts") {
    // clang-format off
    std::vector<uint8_t> data{
        255, 64, 0, 255,
        128, 0, 255, 0};
    // clang-format on
    std::vector<std::optional<std::vector<double>>> expected{
        std::vector<double>{1.0, 64.0 / 255.0, 0.0, 1.0},
        std::vector<double>{128.0 / 255.0, 0.0, 1.0, 0.0}};
    checkNormalizedFixedLengthArray(data, 4, expected);
  }

  SUBCASE("Array of 3 normalized int8_ts with all properties") {
    // clang-format off
    std::vector<int8_t> data{
        -128, 0, 64,
        -64, 127, -128,
         0, 0, 0};
    // clang-format on
    JsonValue::Array offset{0, 1, 1};
    JsonValue::Array scale{1, -1, 2};
    JsonValue::Array noData{0, 0, 0};
    JsonValue::Array defaultValue{10, 8, 2};

    std::vector<std::optional<std::vector<double>>> expected{
        std::vector<double>{-1.0, 1.0, 1 + 2 * (64.0 / 127.0)},
        std::vector<double>{-64.0 / 127.0, 0.0, -1.0},
        std::vector<double>{10.0, 8.0, 2.0},
    };
    checkNormalizedFixedLengthArray(
        data,
        3,
        expected,
        offset,
        scale,
        noData,
        defaultValue);
  }
}

TEST_CASE("Check fixed-length vecN array PropertyTablePropertyView") {
  SUBCASE("Array of 4 u8vec2s") {
    std::vector<glm::u8vec2> data{
        glm::u8vec2(10, 21),
        glm::u8vec2(3, 42),
        glm::u8vec2(122, 22),
        glm::u8vec2(1, 45),
        glm::u8vec2(0, 0),
        glm::u8vec2(32, 12),
        glm::u8vec2(8, 19),
        glm::u8vec2(6, 5)};
    checkFixedLengthArray(data, 4);
  }

  SUBCASE("Array of 2 i8vec3s") {
    std::vector<glm::i8vec3> data{
        glm::i8vec3(122, -12, 3),
        glm::i8vec3(44, 11, -2),
        glm::i8vec3(5, 6, -22),
        glm::i8vec3(5, 6, 1),
        glm::i8vec3(8, -7, 7),
        glm::i8vec3(-4, 36, 17)};
    checkFixedLengthArray(data, 2);
  }

  SUBCASE("Array of 3 vec4s") {
    std::vector<glm::vec4> data{
        glm::vec4(40.2f, -1.2f, 8.8f, 1.0f),
        glm::vec4(1.4f, 0.11, 34.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        glm::vec4(1.0f, 2.0f, 3.0f, 6.0f),
        glm::vec4(1.08f, -3.71f, 18.0f, -7.0f),
        glm::vec4(-17.0f, 33.0f, 8.0f, -3.0f)};
    checkFixedLengthArray(data, 3);
  }

  SUBCASE("Array of 2 vec2s with offset / scale") {
    // clang-format off
    std::vector<glm::vec2> data{
      glm::vec2(1.0f, 2.0f), glm::vec2(3.0f, 4.0f),
      glm::vec2(5.0f, -1.0f), glm::vec2(0.0f, 2.0f)
    };
    // clang-format on

    JsonValue::Array offset{{1.0f, 0.0f}, {-1.0f, 0.0f}};
    JsonValue::Array scale{{1.0f, 2.0f}, {1.0f, 2.0f}};

    std::vector<std::optional<std::vector<glm::vec2>>> expected{
        std::vector<glm::vec2>{glm::vec2(2.0f, 4.0f), glm::vec2(2.0f, 8.0f)},
        std::vector<glm::vec2>{glm::vec2(6.0f, -2.0f), glm::vec2(-1.0f, 4.0f)}};
    checkFixedLengthArray(data, 2, expected, offset, scale);
  }

  SUBCASE("Array of 2 ivec2 with noData value") {
    // clang-format off
    std::vector<glm::ivec2> data{
        glm::ivec2(122, 12), glm::ivec2(-1, -1),
        glm::ivec2( -3, 44), glm::ivec2(0, 7),
        glm::ivec2(-1, -1), glm::ivec2(0, 0)};
    // clang-format on
    JsonValue::Array noData{{-1, -1}, {0, 0}};
    std::vector<std::optional<std::vector<glm::ivec2>>> expected{
        std::vector<glm::ivec2>{glm::ivec2(122, 12), glm::ivec2(-1, -1)},
        std::vector<glm::ivec2>{glm::ivec2(-3, 44), glm::ivec2(0, 7)},
        std::nullopt};
    checkFixedLengthArray<glm::ivec2>(
        data,
        2,
        expected,
        std::nullopt,
        std::nullopt,
        noData,
        std::nullopt);
  }

  SUBCASE("Array of 2 ivec2 with noData and default value") {
    // clang-format off
    std::vector<glm::ivec2> data{
        glm::ivec2(122, 12), glm::ivec2(-1, -1),
        glm::ivec2( -3, 44), glm::ivec2(0, 7),
        glm::ivec2(-1, -1), glm::ivec2(0, 0)};
    // clang-format on
    JsonValue::Array noData{{-1, -1}, {0, 0}};
    JsonValue::Array defaultValue{{1, 1}, {1, 2}};
    std::vector<std::optional<std::vector<glm::ivec2>>> expected{
        std::vector<glm::ivec2>{glm::ivec2(122, 12), glm::ivec2(-1, -1)},
        std::vector<glm::ivec2>{glm::ivec2(-3, 44), glm::ivec2(0, 7)},
        std::vector<glm::ivec2>{glm::ivec2(1, 1), glm::ivec2(1, 2)}};
    checkFixedLengthArray<glm::ivec2>(
        data,
        2,
        expected,
        std::nullopt,
        std::nullopt,
        noData,
        defaultValue);
  }

  SUBCASE("Overrides class property values") {
    // clang-format off
    std::vector<glm::vec2> data{
        glm::vec2(1.0f, 2.0f), glm::vec2(3.0f, 4.0f),
        glm::vec2(0.0f, -1.0f), glm::vec2(1.0f, -2.0f)};
    // clang-format on
    const int64_t count = 2;
    const int64_t instanceCount = 2;

    std::vector<std::byte> buffer;
    buffer.resize(data.size() * sizeof(glm::vec2));
    std::memcpy(buffer.data(), data.data(), buffer.size());

    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC2;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

    classProperty.array = true;
    classProperty.count = count;
    classProperty.offset = JsonValue::Array{{0, 0}, {0, 0}};
    classProperty.scale = JsonValue::Array{{1, 1}, {1, 1}};
    classProperty.min = JsonValue::Array{{0.0f, -1.0f}, {1.0f, -2.0f}};
    classProperty.max = JsonValue::Array{{1.0f, 2.0f}, {3.0f, 4.0f}};

    PropertyTableProperty propertyTableProperty;
    propertyTableProperty.offset = JsonValue::Array{{2, 1}, {0, -1}};
    propertyTableProperty.scale = JsonValue::Array{{1, 0}, {1, -1}};
    propertyTableProperty.min = JsonValue::Array{{0.0f, 1.0f}, {1.0f, -2.0f}};
    propertyTableProperty.max = JsonValue::Array{{1.0f, 1.0f}, {3.0f, 4.0f}};

    PropertyTablePropertyView<PropertyArrayView<glm::vec2>> property(
        propertyTableProperty,
        classProperty,
        instanceCount,
        std::span<const std::byte>(buffer.data(), buffer.size()),
        std::span<const std::byte>(),
        std::span<const std::byte>(),
        PropertyComponentType::None,
        PropertyComponentType::None);

    REQUIRE(property.arrayCount() == count);
    REQUIRE(property.size() == instanceCount);

    REQUIRE(property.offset());
    checkArrayEqual(*property.offset(), {glm::vec2{2, 1}, glm::vec2{0, -1}});

    REQUIRE(property.scale());
    checkArrayEqual(*property.scale(), {glm::vec2{1, 0}, glm::vec2{1, -1}});

    REQUIRE(property.min());
    checkArrayEqual(
        *property.min(),
        {glm::vec2{0.0f, 1.0f}, glm::vec2{1.0f, -2.0f}});

    REQUIRE(property.max());
    checkArrayEqual(
        *property.max(),
        {glm::vec2(1.0f, 1.0f), glm::vec2(3.0f, 4.0f)});

    std::vector<std::vector<glm::vec2>> expected{
        {glm::vec2(3.0f, 1.0f), glm::vec2(3.0f, -5.0f)},
        {glm::vec2(2.0f, 1.0f), glm::vec2(1.0f, 1.0f)}};
    size_t expectedIdx = 0;
    for (int64_t i = 0; i < property.size(); ++i) {
      PropertyArrayView<glm::vec2> rawValues = property.getRaw(i);
      auto values = property.get(i);
      REQUIRE(values);
      for (int64_t j = 0; j < rawValues.size(); ++j) {
        REQUIRE(rawValues[j] == data[expectedIdx]);
        REQUIRE(
            (*values)[j] ==
            expected[static_cast<size_t>(i)][static_cast<size_t>(j)]);
        ++expectedIdx;
      }
    }
  }
}

TEST_CASE(
    "Check fixed-length vecN array PropertyTablePropertyView (normalized)") {
  SUBCASE("Array of 2 u8vec2s") {
    std::vector<glm::u8vec2> data{
        glm::u8vec2(255, 64),
        glm::u8vec2(0, 255),
        glm::u8vec2(128, 0),
        glm::u8vec2(255, 0)};
    std::vector<std::optional<std::vector<glm::dvec2>>> expected{
        std::vector<glm::dvec2>{
            glm::dvec2(1.0, 64.0 / 255.0),
            glm::dvec2(0.0, 1.0)},
        std::vector<glm::dvec2>{
            glm::dvec2(128.0 / 255.0, 0.0),
            glm::dvec2(1.0, 0.0)}};
    checkNormalizedFixedLengthArray(data, 2, expected);
  }

  SUBCASE("Array of 2 i8vec2 with all properties") {
    // clang-format off
    std::vector<glm::i8vec2> data{
      glm::i8vec2(-128, 0), glm::i8vec2(64, -64),
      glm::i8vec2(127, -128), glm::i8vec2(0, 0),
      glm::i8vec2(0), glm::i8vec2(0)};
    // clang-format on
    JsonValue::Array offset{{0, 1}, {1, 2}};
    JsonValue::Array scale{{1, -1}, {2, 1}};
    JsonValue::Array noData{{0, 0}, {0, 0}};
    JsonValue::Array defaultValue{{10, 2}, {4, 8}};

    std::vector<std::optional<std::vector<glm::dvec2>>> expected{
        std::vector<glm::dvec2>{
            glm::dvec2(-1.0, 1.0),
            glm::dvec2(1 + 2 * (64.0 / 127.0), 2 - (64.0 / 127.0))},
        std::vector<glm::dvec2>{glm::dvec2(1, 2), glm::dvec2(1, 2)},
        std::vector<glm::dvec2>{glm::dvec2(10, 2), glm::dvec2(4, 8)},
    };
    checkNormalizedFixedLengthArray(
        data,
        2,
        expected,
        offset,
        scale,
        noData,
        defaultValue);
  }

  SUBCASE("Overrides class property values") {
    // clang-format off
    std::vector<glm::vec2> data{
        glm::vec2(1.0f, 2.0f), glm::vec2(3.0f, 4.0f),
        glm::vec2(0.0f, -1.0f), glm::vec2(1.0f, -2.0f)};
    // clang-format on
    const int64_t count = 2;
    const int64_t instanceCount = 2;

    std::vector<std::byte> buffer;
    buffer.resize(data.size() * sizeof(glm::vec2));
    std::memcpy(buffer.data(), data.data(), buffer.size());

    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC2;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

    classProperty.array = true;
    classProperty.count = count;
    classProperty.offset = JsonValue::Array{{0, 0}, {0, 0}};
    classProperty.scale = JsonValue::Array{{1, 1}, {1, 1}};
    classProperty.min = JsonValue::Array{{0.0f, -1.0f}, {1.0f, -2.0f}};
    classProperty.max = JsonValue::Array{{1.0f, 2.0f}, {3.0f, 4.0f}};

    PropertyTableProperty propertyTableProperty;
    propertyTableProperty.offset = JsonValue::Array{{2, 1}, {0, -1}};
    propertyTableProperty.scale = JsonValue::Array{{1, 0}, {1, -1}};
    propertyTableProperty.min = JsonValue::Array{{0.0f, 1.0f}, {1.0f, -2.0f}};
    propertyTableProperty.max = JsonValue::Array{{1.0f, 1.0f}, {3.0f, 4.0f}};

    PropertyTablePropertyView<PropertyArrayView<glm::vec2>> property(
        propertyTableProperty,
        classProperty,
        instanceCount,
        std::span<const std::byte>(buffer.data(), buffer.size()),
        std::span<const std::byte>(),
        std::span<const std::byte>(),
        PropertyComponentType::None,
        PropertyComponentType::None);

    REQUIRE(property.arrayCount() == count);
    REQUIRE(property.size() == instanceCount);

    REQUIRE(property.offset());
    checkArrayEqual(*property.offset(), {glm::vec2{2, 1}, glm::vec2{0, -1}});

    REQUIRE(property.scale());
    checkArrayEqual(*property.scale(), {glm::vec2{1, 0}, glm::vec2{1, -1}});

    REQUIRE(property.min());
    checkArrayEqual(
        *property.min(),
        {glm::vec2{0.0f, 1.0f}, glm::vec2{1.0f, -2.0f}});

    REQUIRE(property.max());
    checkArrayEqual(
        *property.max(),
        {glm::vec2(1.0f, 1.0f), glm::vec2(3.0f, 4.0f)});

    std::vector<std::vector<glm::vec2>> expected{
        {glm::vec2(3.0f, 1.0f), glm::vec2(3.0f, -5.0f)},
        {glm::vec2(2.0f, 1.0f), glm::vec2(1.0f, 1.0f)}};
    size_t expectedIdx = 0;
    for (int64_t i = 0; i < property.size(); ++i) {
      PropertyArrayView<glm::vec2> rawValues = property.getRaw(i);
      auto values = property.get(i);
      REQUIRE(values);
      for (int64_t j = 0; j < rawValues.size(); ++j) {
        REQUIRE(rawValues[j] == data[expectedIdx]);
        REQUIRE(
            (*values)[j] ==
            expected[static_cast<size_t>(i)][static_cast<size_t>(j)]);
        ++expectedIdx;
      }
    }
  }
}

TEST_CASE("Check fixed-length matN array PropertyTablePropertyView") {
  SUBCASE("Array of 4 i8mat2x2") {
    // clang-format off
    std::vector<glm::i8mat2x2> data{
        glm::i8mat2x2(
          10, 21,
          1, -2),
        glm::i8mat2x2(
          3, 42,
          -10, 12),
        glm::i8mat2x2(
          122, 22,
          80, -4),
        glm::i8mat2x2(
          15, -2,
          17, 6),
        glm::i8mat2x2(
          0, 0,
          -1, 1),
        glm::i8mat2x2(
          32, -12,
          20, 4),
        glm::i8mat2x2(
          8, 19,
          -7, 1),
        glm::i8mat2x2(
          6, 16,
          2, 5)};
    // clang-format on
    checkFixedLengthArray(data, 4);
  }

  SUBCASE("Array of 2 dmat3s") {
    // clang-format off
    std::vector<glm::dmat3> data{
        glm::dmat3(
           1.0, 2.0, 3.0,
          0.01, 0.02, 0.03,
           4.0, 5.0, 6.0),
        glm::dmat3(
          0.2, -1.0, 8.0,
          40.0, -8.0, 9.0,
          10.0, 0.2, 0.34),
        glm::dmat3(
          7.2, 16.5, 4.2,
          33.0, 3.5, -20.0,
          1.22, 1.02, 30.34),
        glm::dmat3(
          1.2, 0.5, 0.0,
          0.0, 3.5, 0.0,
          0.76, 0.9, 1.1),
        glm::dmat3(
          25.0, 50.4, 8.8,
          16.1, 23.0, 40.0,
          0.8, 8.9, 5.0),
        glm::dmat3(
          -4.0, 9.4, 11.2,
          5.5, 3.09, 0.301,
          4.5, 52.4, 1.05)};
    // clang-format on
    checkFixedLengthArray(data, 2);
  }

  SUBCASE("Array of 3 u8mat4x4") {
    // clang-format off
    std::vector<glm::u8mat4x4> data{
        glm::u8mat4x4(
          1, 2, 3, 4,
          5, 6, 7, 8,
          9, 10, 11, 12,
          13, 14, 15, 16),
        glm::u8mat4x4(
          0, 4, 2, 19,
          8, 7, 3, 5,
          43, 21, 10, 9,
          3, 10, 8, 6),
         glm::u8mat4x4(
          1, 0, 0, 19,
          0, 1, 0, 2,
          0, 0, 4, 0,
          0, 0, 0, 1),
         glm::u8mat4x4(
          6, 2, 7, 8,
          50, 11, 18, 2,
          3, 12, 6, 9,
          4, 20, 10, 4),
         glm::u8mat4x4(
          10, 2, 46, 5,
          8, 7, 20, 13,
          24, 8, 6, 9,
          2, 15, 4, 3),
         glm::u8mat4x4(
          3, 2, 1, 0,
          0, 1, 2, 3,
          8, 7, 6, 5,
          4, 3, 2, 1),};
    // clang-format on
    checkFixedLengthArray(data, 3);
  }

  SUBCASE("Array of 2 mat2s with offset / scale") {
    // clang-format off
    std::vector<glm::mat2> data{
      glm::mat2(
        1.0f, 2.0f,
        3.0f, 4.0f),
      glm::mat2(
        5.0f, -1.0f,
        0.0f, 2.0f),
      glm::mat2(
        -1.0f, -1.0f,
        0.0f, -2.0f),
      glm::mat2(
        0.0f, -2.0f,
        4.0f, 3.0f)};

    JsonValue::Array offset{
      {1.0f, 0.0f,
       2.0f, 3.0f},
      {-1.0f, 0.0f,
        0.0f, 2.0f}};
    JsonValue::Array scale{
      {1.0f, 2.0f,
       1.0f, 0.0f},
      {1.0f, -1.0f,
      -1.0f, 2.0f}};

    std::vector<std::optional<std::vector<glm::mat2>>> expected{
      std::vector<glm::mat2>{
        glm::mat2(
          2.0f, 4.0f,
          5.0f, 3.0f),
        glm::mat2(
          4.0f, 1.0f,
          0.0f, 6.0f)},
      std::vector<glm::mat2>{
        glm::mat2(
          0.0f, -2.0f,
          2.0f, 3.0f),
        glm::mat2(
          -1.0f, 2.0f,
          -4.0f, 8.0f)}};
    // clang-format on
    checkFixedLengthArray(data, 2, expected, offset, scale);
  }

  SUBCASE("Array of 2 imat2x2 with noData value") {
    // clang-format off
    std::vector<glm::imat2x2> data{
        glm::imat2x2(
          122, 12,
          4, 6),
        glm::imat2x2(-1),
        glm::imat2x2(
          -3, 44,
          7, 8),
        glm::imat2x2(
          0, 7,
          -1, 0),
        glm::imat2x2(-1),
        glm::imat2x2(0)};
    JsonValue::Array noData{
      {-1, 0,
       0, -1},
      {0, 0,
       0, 0}};
    std::vector<std::optional<std::vector<glm::imat2x2>>> expected{
        std::vector<glm::imat2x2>{
          glm::imat2x2(
            122, 12,
            4, 6),
          glm::imat2x2(-1)},
        std::vector<glm::imat2x2>{
          glm::imat2x2(
            -3, 44,
            7, 8),
          glm::imat2x2(
            0, 7,
            -1, 0)},
        std::nullopt};
    // clang-format on
    checkFixedLengthArray<glm::imat2x2>(
        data,
        2,
        expected,
        std::nullopt,
        std::nullopt,
        noData,
        std::nullopt);
  }

  SUBCASE("Array of 2 imat2x2 with noData and default value") {
    // clang-format off
    std::vector<glm::imat2x2> data{
        glm::imat2x2(
          122, 12,
          4, 6),
        glm::imat2x2(-1),
        glm::imat2x2(
          -3, 44,
          7, 8),
        glm::imat2x2(
          0, 7,
          -1, 0),
        glm::imat2x2(-1),
        glm::imat2x2(0)};
    JsonValue::Array noData{
      {-1, 0,
       0, -1},
      {0, 0,
       0, 0}};
    JsonValue::Array defaultValue{
      {2, 0,
       0, 2},
      {1, 0,
       0, 1}
    };
    std::vector<std::optional<std::vector<glm::imat2x2>>> expected{
        std::vector<glm::imat2x2>{
          glm::imat2x2(
            122, 12,
            4, 6),
          glm::imat2x2(-1)},
        std::vector<glm::imat2x2>{
          glm::imat2x2(
            -3, 44,
            7, 8),
          glm::imat2x2(
            0, 7,
            -1, 0)},
        std::vector<glm::imat2x2>{
          glm::imat2x2(2),
          glm::imat2x2(1)
        }};
    // clang-format on
    checkFixedLengthArray<glm::imat2x2>(
        data,
        2,
        expected,
        std::nullopt,
        std::nullopt,
        noData,
        defaultValue);
  }

  SUBCASE("Overrides class property values") {
    // clang-format off
    std::vector<glm::mat2> data{
        glm::mat2(1.0f),
        glm::mat2(2.0f),
        glm::mat2(3.0f),
        glm::mat2(4.0f)};
    // clang-format on
    const int64_t count = 2;
    const int64_t instanceCount = 2;

    std::vector<std::byte> buffer;
    buffer.resize(data.size() * sizeof(glm::mat2));
    std::memcpy(buffer.data(), data.data(), buffer.size());

    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::MAT2;
    classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

    classProperty.array = true;
    classProperty.count = count;
    // clang-format off
    classProperty.offset = JsonValue::Array{
      {0, 0,
       0, 0},
      {0, 0,
       0, 0}};
    classProperty.scale = JsonValue::Array{
      {1, 1,
       1, 1},
      {1, 1,
       1, 1}};
    classProperty.min = JsonValue::Array{
      {1.0f, 0.0f,
       0.0f, 1.0f},
      {2.0f, 0.0f,
       0.0f, 2.0f}};
    classProperty.max = JsonValue::Array{
      {3.0f, 0.0f,
       0.0f, 3.0f},
      {4.0f, 0.0f,
       0.0f, 4.0f}};
    // clang-format on

    PropertyTableProperty propertyTableProperty;
    // clang-format off
    propertyTableProperty.offset = JsonValue::Array{
      {2, 1,
      -1, -2},
      {0, -1,
       4, 0}};
    propertyTableProperty.scale = JsonValue::Array{
      {1, 0,
       1, 2},
      {1, -1,
       3, 2}};
    propertyTableProperty.min = JsonValue::Array{
      {2.0f, 1.0f,
      -1.0f, 0.0f},
      {2.0f, -1.0f,
      -4.0f, 4.0f}};
    propertyTableProperty.max = JsonValue::Array{
      {5.0f, 1.0f,
      -1.0f, 4.0f},
      {4.0f, -1.0f,
       4.0f, 8.0f}};
    // clang-format on

    PropertyTablePropertyView<PropertyArrayView<glm::mat2>> property(
        propertyTableProperty,
        classProperty,
        instanceCount,
        std::span<const std::byte>(buffer.data(), buffer.size()),
        std::span<const std::byte>(),
        std::span<const std::byte>(),
        PropertyComponentType::None,
        PropertyComponentType::None);

    REQUIRE(property.arrayCount() == count);
    REQUIRE(property.size() == instanceCount);

    // clang-format off
    REQUIRE(property.offset());
    checkArrayEqual(*property.offset(), {
      glm::mat2(
        2, 1,
        -1, -2),
      glm::mat2(
        0, -1,
        4, 0)});

    REQUIRE(property.scale());
    checkArrayEqual(*property.scale(), {
      glm::mat2(
        1, 0,
        1, 2),
      glm::mat2(
        1, -1,
        3, 2)});

    REQUIRE(property.min());
    checkArrayEqual(*property.min(), {
      glm::mat2{
        2.0f, 1.0f,
        -1.0f, 0.0f},
      glm::mat2{
        2.0f, -1.0f,
        -4.0f, 4.0f}});

    REQUIRE(property.max());
    checkArrayEqual( *property.max(), {
      glm::mat2(
        5.0f, 1.0f,
        -1.0f, 4.0f),
      glm::mat2(
        4.0f, -1.0f,
        4.0f, 8.0f)});

    std::vector<std::vector<glm::mat2>> expected{
      {glm::mat2(
        3.0f, 1.0f,
        -1.0f, 0.0f),
       glm::mat2(
        2.0f, -1.0f,
        4.0f, 4.0f)},
      {glm::mat2(
        5.0f, 1.0f,
        -1.0f, 4.0f),
       glm::mat2(
        4.0f, -1.0f,
        4.0f, 8.0f)}};
    // clang-format on
    size_t expectedIdx = 0;
    for (int64_t i = 0; i < property.size(); ++i) {
      PropertyArrayView<glm::mat2> rawValues = property.getRaw(i);
      auto values = property.get(i);
      REQUIRE(values);
      for (int64_t j = 0; j < rawValues.size(); ++j) {
        REQUIRE(rawValues[j] == data[expectedIdx]);
        REQUIRE(
            (*values)[j] ==
            expected[static_cast<size_t>(i)][static_cast<size_t>(j)]);
        ++expectedIdx;
      }
    }
  }
}

TEST_CASE(
    "Check fixed-length matN array PropertyTablePropertyView (normalized)") {
  SUBCASE("Array of 2 u8mat2x2s") {
    // clang-format off
    std::vector<glm::u8mat2x2> data{
        glm::u8mat2x2(
          255, 64,
          0, 255),
        glm::u8mat2x2(
          0, 255,
          64, 128),
        glm::u8mat2x2(
          128, 0,
          0, 255),
        glm::u8mat2x2(
          255, 32,
          255, 0)};

    std::vector<std::optional<std::vector<glm::dmat2>>> expected{
        std::vector<glm::dmat2>{
            glm::dmat2(
              1.0, 64.0 / 255.0,
              0.0, 1.0),
            glm::dmat2(
              0.0, 1.0,
              64.0 / 255.0, 128.0 / 255.0),
        },
        std::vector<glm::dmat2>{
            glm::dmat2(
              128.0 / 255.0, 0.0,
              0.0, 1.0),
            glm::dmat2(
              1.0, 32.0 / 255.0,
              1.0, 0.0),
        }};
    // clang-format on

    checkNormalizedFixedLengthArray(data, 2, expected);
  }

  SUBCASE("Array of 2 i8mat2x2 with all properties") {
    // clang-format off
    std::vector<glm::i8mat2x2> data{
      glm::i8mat2x2(-128),
      glm::i8mat2x2(
        64, -64,
        0, 255),
      glm::i8mat2x2(
        127, -128,
        -128, 0),
      glm::i8mat2x2(0),
      glm::i8mat2x2(0),
      glm::i8mat2x2(
        -128, -128,
        -128, -128)};
    JsonValue::Array offset{
      {0, 1,
      2, 3},
      {1, 2,
       0, -2}};
    JsonValue::Array scale{
      {1, -1,
       0, 1},
      {2, 1,
      -1, 0}};
    JsonValue::Array noData{
      {0, 0,
       0, 0},
      {-128, -128,
       -128, -128}};
    JsonValue::Array defaultValue{
      {1, 0,
      0, 1},
      {2, 0,
      0, 2}};

    std::vector<std::optional<std::vector<glm::dmat2>>> expected{
      std::vector<glm::dmat2>{
        glm::dmat2(
          -1.0, 1.0,
           2.0, 2.0),
        glm::dmat2(
           1 + 2 * (64.0 / 127.0), 2 - (64.0 / 127.0),
           0, -2)},
      std::vector<glm::dmat2>{
        glm::dmat2(
          1.0, 2.0,
          2.0, 3.0),
        glm::dmat2(
          1.0, 2.0,
          0.0, -2.0)},
      std::vector<glm::dmat2>{
        glm::dmat2(1),
        glm::dmat2(2)},
    };
    // clang-format on
    checkNormalizedFixedLengthArray(
        data,
        2,
        expected,
        offset,
        scale,
        noData,
        defaultValue);
  }
}

TEST_CASE("Check variable-length scalar array PropertyTablePropertyView") {
  SUBCASE("Array of uint8_t") {
    // clang-format off
    std::vector<uint8_t> data{
        3, 2,
        0, 45, 2, 1, 4,
        1, 3, 2,
        1, 3, 4, 1
    };
    // clang-format on
    std::vector<uint32_t> offsets{0, 2, 7, 10, 14};
    checkVariableLengthArray(data, offsets, PropertyComponentType::Uint32, 4);
  }

  SUBCASE("Array of int32_t") {
    // clang-format off
    std::vector<int32_t> data{
        3, 200,
        0, 450, 200, 1, 4,
        1, 3, 2,
        1, 3, 4, 1
    };
    // clang-format on
    std::vector<uint32_t> offsets{0, 2, 7, 10, 14};
    checkVariableLengthArray(data, offsets, PropertyComponentType::Uint32, 4);
  }

  SUBCASE("Array of double") {
    // clang-format off
    std::vector<double> data{
        3.333, 200.2,
        0.1122, 4.50, 2.30, 1.22, 4.444,
        1.4, 3.3, 2.2,
        1.11, 3.2, 4.111, 1.44
    };
    // clang-format on
    std::vector<uint32_t> offsets{0, 2, 7, 10, 14};
    checkVariableLengthArray(data, offsets, PropertyComponentType::Uint32, 4);
  }

  SUBCASE("Array of normalized uint8_t") {
    // clang-format off
    std::vector<uint8_t> data{
        255, 0,
        0, 255, 128,
        64,
    };
    // clang-format on
    std::vector<uint32_t> offsets{0, 2, 5, 6};

    std::vector<std::optional<std::vector<double>>> expected{
        std::vector<double>{1.0, 0.0},
        std::vector<double>{0.0, 1.0, 128.0 / 255.0},
        std::vector<double>{64.0 / 255.0},
    };
    checkNormalizedVariableLengthArray(
        data,
        offsets,
        PropertyComponentType::Uint32,
        3,
        expected);
  }

  SUBCASE("Array of int32_t with NoData") {
    // clang-format off
    std::vector<int32_t> data{
        3, 200,
        0, 450, 200, 1, 4,
        1, 3, 2,
        0,
        1, 3, 4, 1
    };
    // clang-format on
    std::vector<uint32_t> offsets{0, 2, 7, 10, 11, 15};

    JsonValue::Array noData{0};

    std::vector<std::optional<std::vector<int32_t>>> expected{
        std::vector<int32_t>{3, 200},
        std::vector<int32_t>{0, 450, 200, 1, 4},
        std::vector<int32_t>{1, 3, 2},
        std::nullopt,
        std::vector<int32_t>{1, 3, 4, 1}};
    checkVariableLengthArray(
        data,
        offsets,
        PropertyComponentType::Uint32,
        5,
        expected,
        noData);
  }

  SUBCASE("Array of int32_t with NoData and DefaultValue") {
    // clang-format off
    std::vector<int32_t> data{
        3, 200,
        0, 450, 200, 1, 4,
        1, 3, 2,
        0,
        1, 3, 4, 1
    };
    // clang-format on
    std::vector<uint32_t> offsets{0, 2, 7, 10, 11, 15};

    JsonValue::Array noData{0};
    JsonValue::Array defaultValue{1};

    std::vector<std::optional<std::vector<int32_t>>> expected{
        std::vector<int32_t>{3, 200},
        std::vector<int32_t>{0, 450, 200, 1, 4},
        std::vector<int32_t>{1, 3, 2},
        std::vector<int32_t>{1},
        std::vector<int32_t>{1, 3, 4, 1}};
    checkVariableLengthArray(
        data,
        offsets,
        PropertyComponentType::Uint32,
        5,
        expected,
        noData,
        defaultValue);
  }

  SUBCASE("Array of normalized uint8_t with NoData and DefaultValue") {
    // clang-format off
    std::vector<uint8_t> data{
        255, 0,
        0, 255, 128,
        64,
        255, 255
    };
    // clang-format on
    std::vector<uint32_t> offsets{0, 2, 5, 6, 8};

    JsonValue::Array noData{255, 255};
    JsonValue::Array defaultValue{-1.0};

    std::vector<std::optional<std::vector<double>>> expected{
        std::vector<double>{1.0, 0.0},
        std::vector<double>{0.0, 1.0, 128.0 / 255.0},
        std::vector<double>{64.0 / 255},
        std::vector<double>{-1.0},
    };

    checkNormalizedVariableLengthArray(
        data,
        offsets,
        PropertyComponentType::Uint32,
        4,
        expected,
        noData,
        defaultValue);
  }
}

TEST_CASE("Check variable-length scalar array PropertyTablePropertyView "
          "(normalized)") {
  SUBCASE("Array of uint8_t") {
    // clang-format off
    std::vector<uint8_t> data{
        255, 0,
        0, 255, 128,
        64,
    };
    // clang-format on
    std::vector<uint32_t> offsets{0, 2, 5, 6};

    std::vector<std::optional<std::vector<double>>> expected{
        std::vector<double>{1.0, 0.0},
        std::vector<double>{0.0, 1.0, 128.0 / 255.0},
        std::vector<double>{64.0 / 255.0},
    };
    checkNormalizedVariableLengthArray(
        data,
        offsets,
        PropertyComponentType::Uint32,
        3,
        expected);
  }

  SUBCASE("Array of uint8_t with NoData and DefaultValue") {
    // clang-format off
    std::vector<uint8_t> data{
        255, 0,
        0, 255, 128,
        64,
        255, 255
    };
    // clang-format on
    std::vector<uint32_t> offsets{0, 2, 5, 6, 8};

    JsonValue::Array noData{255, 255};
    JsonValue::Array defaultValue{-1.0};

    std::vector<std::optional<std::vector<double>>> expected{
        std::vector<double>{1.0, 0.0},
        std::vector<double>{0.0, 1.0, 128.0 / 255.0},
        std::vector<double>{64.0 / 255},
        std::vector<double>{-1.0},
    };

    checkNormalizedVariableLengthArray(
        data,
        offsets,
        PropertyComponentType::Uint32,
        4,
        expected,
        noData,
        defaultValue);
  }
}

TEST_CASE("Check variable-length vecN array PropertyTablePropertyView") {
  SUBCASE("Array of ivec2") {
    // clang-format off
    std::vector<glm::ivec2> data{
        glm::ivec2(3, -2), glm::ivec2(20, 3),
        glm::ivec2(0, 45), glm::ivec2(-10, 2), glm::ivec2(4, 4), glm::ivec2(1, -1),
        glm::ivec2(3, 1), glm::ivec2(3, 2), glm::ivec2(0, -5),
        glm::ivec2(-9, 10), glm::ivec2(8, -2)
    };
    // clang-format on
    std::vector<uint32_t> offsets{0, 2, 6, 9, 11};
    checkVariableLengthArray(data, offsets, PropertyComponentType::Uint32, 4);
  }

  SUBCASE("Array of dvec3") {
    // clang-format off
    std::vector<glm::dvec3> data{
        glm::dvec3(-0.02, 2.0, 1.0), glm::dvec3(9.92, 9.0, -8.0),
        glm::dvec3(22.0, 5.5, -3.7), glm::dvec3(1.02, 9.0, -8.0), glm::dvec3(0.0, 0.5, 1.0),
        glm::dvec3(-1.3, -5.0, -90.0),
        glm::dvec3(4.4, 1.0, 2.3), glm::dvec3(5.8, 7.07, -4.0),
        glm::dvec3(-2.0, 0.85, 0.22), glm::dvec3(-8.8, 5.1, 0.0), glm::dvec3(12.0, 8.0, -2.2),
    };
    // clang-format on
    std::vector<uint32_t> offsets{0, 2, 5, 6, 8, 11};
    checkVariableLengthArray(data, offsets, PropertyComponentType::Uint32, 5);
  }

  SUBCASE("Array of u8vec4") {
    // clang-format off
     std::vector<glm::u8vec4> data{
         glm::u8vec4(1, 2, 3, 4), glm::u8vec4(5, 6, 7, 8),
         glm::u8vec4(9, 2, 1, 0),
         glm::u8vec4(8, 7, 10, 21), glm::u8vec4(3, 6, 8, 0), glm::u8vec4(0, 0, 0, 1),
         glm::u8vec4(64, 8, 17, 5), glm::u8vec4(35, 23, 10, 0),
         glm::u8vec4(99, 8, 1, 2)
     };
    // clang-format on
    std::vector<uint32_t> offsets{0, 2, 3, 6, 8, 9};
    checkVariableLengthArray(data, offsets, PropertyComponentType::Uint32, 5);
  }

  SUBCASE("Array of ivec3 with NoData") {
    // clang-format off
    std::vector<glm::ivec3> data{
        glm::ivec3(3, 200, 1), glm::ivec3(2, 4, 6),
        glm::ivec3(-1, 0, -450),
        glm::ivec3(200, 1, 4), glm::ivec3(1, 3, 2), glm::ivec3(0),
        glm::ivec3(-1),
        glm::ivec3(1, 3, 4), glm::ivec3(1, 0, 1)
    };
    // clang-format on
    std::vector<uint32_t> offsets{0, 2, 3, 6, 7, 9};

    JsonValue::Array noData{};
    noData.push_back(JsonValue::Array{-1, -1, -1});

    std::vector<std::optional<std::vector<glm::ivec3>>> expected{
        std::vector<glm::ivec3>{glm::ivec3(3, 200, 1), glm::ivec3(2, 4, 6)},
        std::vector<glm::ivec3>{glm::ivec3(-1, 0, -450)},
        std::vector<glm::ivec3>{
            glm::ivec3(200, 1, 4),
            glm::ivec3(1, 3, 2),
            glm::ivec3(0)},
        std::nullopt,
        std::vector<glm::ivec3>{glm::ivec3(1, 3, 4), glm::ivec3(1, 0, 1)}};
    checkVariableLengthArray(
        data,
        offsets,
        PropertyComponentType::Uint32,
        5,
        expected,
        noData);
  }

  SUBCASE("Array of ivec3 with NoData and DefaultValue") {
    // clang-format off
    std::vector<glm::ivec3> data{
        glm::ivec3(3, 200, 1), glm::ivec3(2, 4, 6),
        glm::ivec3(-1, 0, -450),
        glm::ivec3(200, 1, 4), glm::ivec3(1, 3, 2), glm::ivec3(0),
        glm::ivec3(-1),
        glm::ivec3(1, 3, 4), glm::ivec3(1, 0, 1)
    };
    // clang-format on
    std::vector<uint32_t> offsets{0, 2, 3, 6, 7, 9};

    JsonValue::Array noData{};
    noData.push_back(JsonValue::Array{-1, -1, -1});
    JsonValue::Array defaultValue{};
    defaultValue.push_back(JsonValue::Array{0, 0, 0});

    std::vector<std::optional<std::vector<glm::ivec3>>> expected{
        std::vector<glm::ivec3>{glm::ivec3(3, 200, 1), glm::ivec3(2, 4, 6)},
        std::vector<glm::ivec3>{glm::ivec3(-1, 0, -450)},
        std::vector<glm::ivec3>{
            glm::ivec3(200, 1, 4),
            glm::ivec3(1, 3, 2),
            glm::ivec3(0)},
        std::vector<glm::ivec3>{glm::ivec3(0)},
        std::vector<glm::ivec3>{glm::ivec3(1, 3, 4), glm::ivec3(1, 0, 1)}};
    checkVariableLengthArray(
        data,
        offsets,
        PropertyComponentType::Uint32,
        5,
        expected,
        noData,
        defaultValue);
  }
}

TEST_CASE(
    "Check variable-length vecN array PropertyTablePropertyView (normalized)") {
  SUBCASE("Array of u8vec2") {
    // clang-format off
    std::vector<glm::u8vec2> data{
        glm::u8vec2(255, 0), glm::u8vec2(0, 64),
        glm::u8vec2(0, 0),
        glm::u8vec2(128, 255), glm::u8vec2(255, 255), glm::u8vec2(32, 64)
    };
    // clang-format on
    std::vector<uint32_t> offsets{0, 2, 3, 6};

    std::vector<std::optional<std::vector<glm::dvec2>>> expected{
        std::vector<glm::dvec2>{
            glm::dvec2(1.0, 0.0),
            glm::dvec2(0.0, 64.0 / 255.0)},
        std::vector<glm::dvec2>{glm::dvec2(0.0, 0.0)},
        std::vector<glm::dvec2>{
            glm::dvec2(128.0 / 255.0, 1.0),
            glm::dvec2(1.0, 1.0),
            glm::dvec2(32.0 / 255.0, 64.0 / 255.0)},
    };
    checkNormalizedVariableLengthArray(
        data,
        offsets,
        PropertyComponentType::Uint32,
        3,
        expected);
  }

  SUBCASE("Array of normalized u8vec2 with NoData and DefaultValue") {
    // clang-format off
    std::vector<glm::u8vec2> data{
        glm::u8vec2(255, 0), glm::u8vec2(0, 64),
        glm::u8vec2(0, 0),
        glm::u8vec2(128, 255), glm::u8vec2(255, 255), glm::u8vec2(32, 64)
    };
    // clang-format on
    std::vector<uint32_t> offsets{0, 2, 3, 6};

    JsonValue::Array noData{};
    noData.push_back(JsonValue::Array{0, 0});
    JsonValue::Array defaultValue{};
    defaultValue.push_back(JsonValue::Array{-1.0, -1.0});

    std::vector<std::optional<std::vector<glm::dvec2>>> expected{
        std::vector<glm::dvec2>{
            glm::dvec2(1.0, 0.0),
            glm::dvec2(0.0, 64.0 / 255.0)},
        std::vector<glm::dvec2>{glm::dvec2(-1.0)},
        std::vector<glm::dvec2>{
            glm::dvec2(128.0 / 255.0, 1.0),
            glm::dvec2(1.0, 1.0),
            glm::dvec2(32.0 / 255.0, 64.0 / 255.0)},
    };
    checkNormalizedVariableLengthArray(
        data,
        offsets,
        PropertyComponentType::Uint32,
        3,
        expected,
        noData,
        defaultValue);
  }
}

TEST_CASE("Check variable-length matN array PropertyTablePropertyView") {
  SUBCASE("Array of dmat2") {
    // clang-format off
    std::vector<glm::dmat2> data0{
        glm::dmat2(
          3.23, -2.456,
          1.0, 0.003),
        glm::dmat2(
          40.0, 3.66,
          8.567, -9.8)
    };
    std::vector<glm::dmat2> data1{
        glm::dmat2(
          1.1, 10.02,
          7.0, 0.0),
    };
    std::vector<glm::dmat2> data2{
        glm::dmat2(
          18.8, 0.0,
          1.0, 17.2),
        glm::dmat2(
          -4.0, -0.053,
          -9.0, 1.0),
        glm::dmat2(
          1.1, 8.88,
          -99.0, 1.905),
    };
    // clang-format on

    std::vector<glm::dmat2> data;
    data.reserve(data0.size() + data1.size() + data2.size());
    data.insert(data.end(), data0.begin(), data0.end());
    data.insert(data.end(), data1.begin(), data1.end());
    data.insert(data.end(), data2.begin(), data2.end());

    std::vector<uint32_t> offsets{0, 2, 3, 6};

    checkVariableLengthArray(data, offsets, PropertyComponentType::Uint32, 3);
  }

  SUBCASE("Array of i16mat3x3") {
    // clang-format off
    std::vector<glm::i16mat3x3> data0{
        glm::i16mat3x3(
          1, 0, 0,
          0, -1, 0,
          0, 0, 1),
    };
    std::vector<glm::i16mat3x3> data1{
        glm::i16mat3x3(
          2, 3, 0,
          -9, 14, 4,
          -2, -5, 10),
        glm::i16mat3x3(
          0, 5, 10,
          -8, 33, 2,
          -9, 8, 41),
        glm::i16mat3x3(
          10, -7, 8,
          21, -9, 2,
          3, 4, 5)
    };
    std::vector<glm::i16mat3x3> data2{
        glm::i16mat3x3(
          -10, 50, 30,
          8, 17, 2,
          16, 40, 3),
        glm::i16mat3x3(
          -9, 18, 8,
          20, 3, 4,
          16, 7, -9),
    };
    // clang-format on

    std::vector<glm::i16mat3x3> data;
    data.reserve(data0.size() + data1.size() + data2.size());
    data.insert(data.end(), data0.begin(), data0.end());
    data.insert(data.end(), data1.begin(), data1.end());
    data.insert(data.end(), data2.begin(), data2.end());

    std::vector<uint32_t> offsets{0, 1, 4, 6};

    checkVariableLengthArray(data, offsets, PropertyComponentType::Uint32, 3);
  }

  SUBCASE("Array of u8mat4x4") {
    // clang-format off
    std::vector<glm::u8mat4x4> data0{
        glm::u8mat4x4(
          1, 0, 0, 0,
          0, 4, 0, 0,
          0, 0, 1, 10,
          0, 0, 0, 1),
        glm::u8mat4x4(
          10, 0, 0, 8,
          0, 5, 0, 4,
          0, 0, 1, 3,
          0, 0, 0, 1),
        glm::u8mat4x4(
          8, 0, 0, 9,
          0, 3, 0, 11,
          0, 0, 20, 0,
          0, 0, 0, 1),
    };
    std::vector<glm::u8mat4x4> data1{
        glm::u8mat4x4(
          1, 2, 3, 4,
          4, 3, 2, 1,
          0, 9, 8, 7,
          6, 5, 5, 6),
    };
    std::vector<glm::u8mat4x4> data2{
        glm::u8mat4x4(
          4, 1, 8, 9,
          2, 6, 50, 1,
          10, 20, 30, 9,
          8, 7, 20, 4),
        glm::u8mat4x4(
          0, 2, 1, 0,
          25, 19, 8, 2,
          3, 6, 40, 50,
          15, 9, 0, 3),
    };
    // clang-format on

    std::vector<glm::u8mat4x4> data;
    data.reserve(data0.size() + data1.size() + data2.size());
    data.insert(data.end(), data0.begin(), data0.end());
    data.insert(data.end(), data1.begin(), data1.end());
    data.insert(data.end(), data2.begin(), data2.end());

    std::vector<uint32_t> offsets{0, 3, 4, 6};

    checkVariableLengthArray(data, offsets, PropertyComponentType::Uint32, 3);
  }

  SUBCASE("Array of imat3x3 with NoData") {
    // clang-format off
    std::vector<glm::imat3x3> data{
        glm::imat3x3(3), glm::imat3x3(2),
        glm::imat3x3(-1),
        glm::imat3x3(200), glm::imat3x3(1), glm::imat3x3(0),
        glm::imat3x3(-1),
        glm::imat3x3(1), glm::imat3x3(24)
    };
    // clang-format on
    std::vector<uint32_t> offsets{0, 2, 3, 6, 7, 9};

    // clang-format off
    JsonValue::Array noData{};
    noData.push_back(JsonValue::Array{
      -1, 0, 0,
       0, -1, 0,
       0, 0, -1});
    // clang-format on

    std::vector<std::optional<std::vector<glm::imat3x3>>> expected{
        std::vector<glm::imat3x3>{glm::imat3x3(3), glm::imat3x3(2)},
        std::nullopt,
        std::vector<glm::imat3x3>{
            glm::imat3x3(200),
            glm::imat3x3(1),
            glm::imat3x3(0)},
        std::nullopt,
        std::vector<glm::imat3x3>{glm::imat3x3(1), glm::imat3x3(24)}};
    checkVariableLengthArray(
        data,
        offsets,
        PropertyComponentType::Uint32,
        5,
        expected,
        noData);
  }

  SUBCASE("Array of imat3x3 with NoData and DefaultValue") {
    // clang-format off
    std::vector<glm::imat3x3> data{
        glm::imat3x3(3), glm::imat3x3(2),
        glm::imat3x3(-1),
        glm::imat3x3(200), glm::imat3x3(1), glm::imat3x3(0),
        glm::imat3x3(-1),
        glm::imat3x3(1), glm::imat3x3(24)
    };
    // clang-format on
    std::vector<uint32_t> offsets{0, 2, 3, 6, 7, 9};

    // clang-format off
    JsonValue::Array noData{};
    noData.push_back(JsonValue::Array{
      -1, 0, 0,
      0, -1, 0,
      0, 0, -1});
    JsonValue::Array defaultValue{};
    defaultValue.push_back(JsonValue::Array{
      99, 0, 0,
      0, 99, 0,
      0, 0, 99});
    // clang-format on

    std::vector<std::optional<std::vector<glm::imat3x3>>> expected{
        std::vector<glm::imat3x3>{glm::imat3x3(3), glm::imat3x3(2)},
        std::vector<glm::imat3x3>{glm::imat3x3(99)},
        std::vector<glm::imat3x3>{
            glm::imat3x3(200),
            glm::imat3x3(1),
            glm::imat3x3(0)},
        std::vector<glm::imat3x3>{glm::imat3x3(99)},
        std::vector<glm::imat3x3>{glm::imat3x3(1), glm::imat3x3(24)}};
    checkVariableLengthArray(
        data,
        offsets,
        PropertyComponentType::Uint32,
        5,
        expected,
        noData,
        defaultValue);
  }
}

TEST_CASE(
    "Check variable-length matN array PropertyTablePropertyView (normalized)") {
  SUBCASE("Array of u8mat2x2") {
    // clang-format off
    std::vector<glm::u8mat2x2> data{
        glm::u8mat2x2(255), glm::u8mat2x2(64),
        glm::u8mat2x2(0),
        glm::u8mat2x2(128), glm::u8mat2x2(255), glm::u8mat2x2(32)
    };
    // clang-format on
    std::vector<uint32_t> offsets{0, 2, 3, 6};

    std::vector<std::optional<std::vector<glm::dmat2>>> expected{
        std::vector<glm::dmat2>{glm::dmat2(1.0), glm::dmat2(64.0 / 255.0)},
        std::vector<glm::dmat2>{glm::dmat2(0.0)},
        std::vector<glm::dmat2>{
            glm::dmat2(128.0 / 255.0),
            glm::dmat2(1.0),
            glm::dmat2(32.0 / 255.0)},
    };
    checkNormalizedVariableLengthArray(
        data,
        offsets,
        PropertyComponentType::Uint32,
        3,
        expected);
  }

  SUBCASE("Array of u8mat2x2 with NoData and DefaultValue") {
    // clang-format off
    std::vector<glm::u8mat2x2> data{
        glm::u8mat2x2(255), glm::u8mat2x2(64),
        glm::u8mat2x2(0),
        glm::u8mat2x2(128), glm::u8mat2x2(255), glm::u8mat2x2(32)
    };
    // clang-format on
    std::vector<uint32_t> offsets{0, 2, 3, 6};

    // clang-format off
    JsonValue::Array noData{};
    noData.push_back(JsonValue::Array{
      0, 0,
      0, 0});
    JsonValue::Array defaultValue{};
    defaultValue.push_back(JsonValue::Array{
      -1.0, 0.0,
      0.0, -1.0});
    // clang-format on
    std::vector<std::optional<std::vector<glm::dmat2>>> expected{
        std::vector<glm::dmat2>{glm::dmat2(1.0), glm::dmat2(64.0 / 255.0)},
        std::vector<glm::dmat2>{glm::dmat2(-1.0)},
        std::vector<glm::dmat2>{
            glm::dmat2(128.0 / 255.0),
            glm::dmat2(1.0),
            glm::dmat2(32.0 / 255.0)},
    };
    checkNormalizedVariableLengthArray(
        data,
        offsets,
        PropertyComponentType::Uint32,
        3,
        expected,
        noData,
        defaultValue);
  }
}

TEST_CASE("Check fixed-length array of string") {
  std::vector<std::string> strings{
      "Test 1",
      "Test 2",
      "Test 3",
      "Test 4",
      "Test 5",
      "Test 6",
      "This is a fine test",
      "What's going on",
      "Good morning"};
  size_t totalSize = 0;
  for (const auto& str : strings) {
    totalSize += str.size();
  }

  const size_t stringCount = strings.size();
  std::vector<std::byte> buffer;
  buffer.resize(totalSize);
  uint32_t currentStringOffset = 0;
  for (size_t i = 0; i < stringCount; ++i) {
    std::memcpy(
        buffer.data() + currentStringOffset,
        strings[i].data(),
        strings[i].size());
    currentStringOffset += static_cast<uint32_t>(strings[i].size());
  }

  // Create string offset buffer
  std::vector<std::byte> stringOffsets;
  stringOffsets.resize((stringCount + 1) * sizeof(uint32_t));
  currentStringOffset = 0;
  for (size_t i = 0; i < stringCount; ++i) {
    std::memcpy(
        stringOffsets.data() + i * sizeof(uint32_t),
        &currentStringOffset,
        sizeof(uint32_t));
    currentStringOffset += static_cast<uint32_t>(strings[i].size());
  }

  std::memcpy(
      stringOffsets.data() + stringCount * sizeof(uint32_t),
      &currentStringOffset,
      sizeof(uint32_t));

  SUBCASE("Returns correct values") {
    PropertyTableProperty propertyTableProperty;
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.array = true;
    classProperty.count = 3;

    PropertyTablePropertyView<PropertyArrayView<std::string_view>> property(
        propertyTableProperty,
        classProperty,
        static_cast<int64_t>(stringCount / 3),
        std::span<const std::byte>(buffer.data(), buffer.size()),
        std::span<const std::byte>(),
        std::span<const std::byte>(stringOffsets.data(), stringOffsets.size()),
        PropertyComponentType::None,
        PropertyComponentType::Uint32);

    REQUIRE(property.arrayCount() == classProperty.count);

    size_t expectedIdx = 0;
    for (int64_t i = 0; i < property.size(); ++i) {
      PropertyArrayView<std::string_view> values = property.getRaw(i);
      auto maybeValues = property.get(i);
      REQUIRE(maybeValues);

      for (int64_t j = 0; j < values.size(); ++j) {
        REQUIRE(values[j] == strings[expectedIdx]);
        REQUIRE((*maybeValues)[j] == strings[expectedIdx]);
        ++expectedIdx;
      }
    }

    REQUIRE(expectedIdx == stringCount);
  }

  SUBCASE("Uses NoData value") {
    PropertyTableProperty propertyTableProperty;
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.array = true;
    classProperty.count = 3;
    classProperty.noData = {"Test 1", "Test 2", "Test 3"};

    PropertyTablePropertyView<PropertyArrayView<std::string_view>> property(
        propertyTableProperty,
        classProperty,
        static_cast<int64_t>(stringCount / 3),
        std::span<const std::byte>(buffer.data(), buffer.size()),
        std::span<const std::byte>(),
        std::span<const std::byte>(stringOffsets.data(), stringOffsets.size()),
        PropertyComponentType::None,
        PropertyComponentType::Uint32);

    REQUIRE(property.arrayCount() == classProperty.count);

    std::vector<std::optional<std::vector<std::string_view>>> expected{
        std::nullopt,
        std::vector<std::string_view>{"Test 4", "Test 5", "Test 6"},
        std::vector<std::string_view>{
            "This is a fine test",
            "What's going on",
            "Good morning"}};

    // Check raw values first
    size_t expectedIdx = 0;
    for (int64_t i = 0; i < property.size(); ++i) {
      PropertyArrayView<std::string_view> values = property.getRaw(i);
      for (int64_t j = 0; j < values.size(); ++j) {
        std::string_view v = values[j];
        REQUIRE(v == strings[expectedIdx]);
        ++expectedIdx;
      }
    }

    REQUIRE(expectedIdx == stringCount);

    // Check values with properties
    for (int64_t i = 0; i < property.size(); ++i) {
      auto maybeValues = property.get(i);
      if (!maybeValues) {
        REQUIRE(!expected[static_cast<size_t>(i)]);
        continue;
      }

      auto values = *maybeValues;
      auto expectedValues = *expected[static_cast<size_t>(i)];
      for (int64_t j = 0; j < values.size(); ++j) {
        REQUIRE(values[j] == expectedValues[static_cast<size_t>(j)]);
      }
    }
  }

  SUBCASE("Uses NoData and DefaultValue") {
    PropertyTableProperty propertyTableProperty;
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.array = true;
    classProperty.count = 3;
    classProperty.noData = {"Test 1", "Test 2", "Test 3"};
    classProperty.defaultProperty = {"Default 1", "Default 2", "Default 3"};

    PropertyTablePropertyView<PropertyArrayView<std::string_view>> property(
        propertyTableProperty,
        classProperty,
        static_cast<int64_t>(stringCount / 3),
        std::span<const std::byte>(buffer.data(), buffer.size()),
        std::span<const std::byte>(),
        std::span<const std::byte>(stringOffsets.data(), stringOffsets.size()),
        PropertyComponentType::None,
        PropertyComponentType::Uint32);

    REQUIRE(property.arrayCount() == classProperty.count);

    std::vector<std::optional<std::vector<std::string_view>>> expected{
        std::vector<std::string_view>{"Default 1", "Default 2", "Default 3"},
        std::vector<std::string_view>{"Test 4", "Test 5", "Test 6"},
        std::vector<std::string_view>{
            "This is a fine test",
            "What's going on",
            "Good morning"}};

    // Check raw values first
    size_t expectedIdx = 0;
    for (int64_t i = 0; i < property.size(); ++i) {
      PropertyArrayView<std::string_view> values = property.getRaw(i);
      for (int64_t j = 0; j < values.size(); ++j) {
        std::string_view v = values[j];
        REQUIRE(v == strings[expectedIdx]);
        ++expectedIdx;
      }
    }

    REQUIRE(expectedIdx == stringCount);

    // Check values with properties
    for (int64_t i = 0; i < property.size(); ++i) {
      auto maybeValues = property.get(i);
      if (!maybeValues) {
        REQUIRE(!expected[static_cast<size_t>(i)]);
        continue;
      }

      auto values = *maybeValues;
      auto expectedValues = *expected[static_cast<size_t>(i)];
      for (int64_t j = 0; j < values.size(); ++j) {
        REQUIRE(values[j] == expectedValues[static_cast<size_t>(j)]);
      }
    }
  }
}

TEST_CASE("Check variable-length string array PropertyTablePropertyView") {
  // clang-format off
  std::vector<uint32_t> arrayOffsets{
    0,
    4 * sizeof(uint32_t),
    7 * sizeof(uint32_t),
    8 * sizeof(uint32_t),
    12 * sizeof(uint32_t)
  };

  std::vector<std::string> strings{
    "Test 1", "Test 2", "Test 3", "Test 4",
    "Test 5", "Test 6", "Test 7",
    "Null",
    "Test 8", "Test 9", "Test 10", "Test 11"
  };
  // clang-format on

  size_t totalSize = 0;
  for (const auto& str : strings) {
    totalSize += str.size();
  }

  const size_t stringCount = strings.size();
  uint32_t currentOffset = 0;
  std::vector<std::byte> buffer;
  buffer.resize(totalSize);
  for (size_t i = 0; i < stringCount; ++i) {
    std::memcpy(
        buffer.data() + currentOffset,
        strings[i].data(),
        strings[i].size());
    currentOffset += static_cast<uint32_t>(strings[i].size());
  }

  std::vector<std::byte> stringOffsets;
  stringOffsets.resize((stringCount + 1) * sizeof(uint32_t));
  currentOffset = 0;
  for (size_t i = 0; i < stringCount; ++i) {
    std::memcpy(
        stringOffsets.data() + i * sizeof(uint32_t),
        &currentOffset,
        sizeof(uint32_t));
    currentOffset += static_cast<uint32_t>(strings[i].size());
  }
  std::memcpy(
      stringOffsets.data() + stringCount * sizeof(uint32_t),
      &currentOffset,
      sizeof(uint32_t));

  SUBCASE("Returns correct values") {
    PropertyTableProperty propertyTableProperty;
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.array = true;

    PropertyTablePropertyView<PropertyArrayView<std::string_view>> property(
        propertyTableProperty,
        classProperty,
        4,
        std::span<const std::byte>(buffer.data(), buffer.size()),
        std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(arrayOffsets.data()),
            arrayOffsets.size() * sizeof(uint32_t)),
        std::span<const std::byte>(stringOffsets.data(), stringOffsets.size()),
        PropertyComponentType::Uint32,
        PropertyComponentType::Uint32);

    REQUIRE(property.arrayCount() == 0);

    size_t expectedIdx = 0;
    for (int64_t i = 0; i < property.size(); ++i) {
      PropertyArrayView<std::string_view> values = property.getRaw(i);
      auto maybeValues = property.get(i);
      REQUIRE(maybeValues);

      for (int64_t j = 0; j < values.size(); ++j) {
        REQUIRE(values[j] == strings[expectedIdx]);
        REQUIRE((*maybeValues)[j] == strings[expectedIdx]);
        ++expectedIdx;
      }
    }

    REQUIRE(expectedIdx == stringCount);
  }

  SUBCASE("Uses NoData Value") {
    PropertyTableProperty propertyTableProperty;
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.array = true;
    classProperty.noData = JsonValue::Array{"Null"};

    PropertyTablePropertyView<PropertyArrayView<std::string_view>> property(
        propertyTableProperty,
        classProperty,
        4,
        std::span<const std::byte>(buffer.data(), buffer.size()),
        std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(arrayOffsets.data()),
            arrayOffsets.size() * sizeof(uint32_t)),
        std::span<const std::byte>(stringOffsets.data(), stringOffsets.size()),
        PropertyComponentType::Uint32,
        PropertyComponentType::Uint32);

    REQUIRE(property.arrayCount() == 0);

    std::vector<std::optional<std::vector<std::string_view>>> expected{
        std::vector<std::string_view>{"Test 1", "Test 2", "Test 3", "Test 4"},
        std::vector<std::string_view>{"Test 5", "Test 6", "Test 7"},
        std::nullopt,
        std::vector<std::string_view>{
            "Test 8",
            "Test 9",
            "Test 10",
            "Test 11"}};

    // Check raw values first
    size_t expectedIdx = 0;
    for (int64_t i = 0; i < property.size(); ++i) {
      PropertyArrayView<std::string_view> values = property.getRaw(i);
      for (int64_t j = 0; j < values.size(); ++j) {
        std::string_view v = values[j];
        REQUIRE(v == strings[expectedIdx]);
        ++expectedIdx;
      }
    }

    REQUIRE(expectedIdx == stringCount);

    // Check values with properties
    for (int64_t i = 0; i < property.size(); ++i) {
      auto maybeValues = property.get(i);
      if (!maybeValues) {
        REQUIRE(!expected[static_cast<size_t>(i)]);
        continue;
      }

      auto values = *maybeValues;
      auto expectedValues = *expected[static_cast<size_t>(i)];
      for (int64_t j = 0; j < values.size(); ++j) {
        REQUIRE(values[j] == expectedValues[static_cast<size_t>(j)]);
      }
    }
  }

  SUBCASE("Uses NoData and DefaultValue") {
    PropertyTableProperty propertyTableProperty;
    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::STRING;
    classProperty.array = true;
    classProperty.noData = JsonValue::Array{"Null"};
    classProperty.defaultProperty = JsonValue::Array{"Default"};

    PropertyTablePropertyView<PropertyArrayView<std::string_view>> property(
        propertyTableProperty,
        classProperty,
        4,
        std::span<const std::byte>(buffer.data(), buffer.size()),
        std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(arrayOffsets.data()),
            arrayOffsets.size() * sizeof(uint32_t)),
        std::span<const std::byte>(stringOffsets.data(), stringOffsets.size()),
        PropertyComponentType::Uint32,
        PropertyComponentType::Uint32);

    REQUIRE(property.arrayCount() == 0);

    std::vector<std::optional<std::vector<std::string_view>>> expected{
        std::vector<std::string_view>{"Test 1", "Test 2", "Test 3", "Test 4"},
        std::vector<std::string_view>{"Test 5", "Test 6", "Test 7"},
        std::vector<std::string_view>{"Default"},
        std::vector<std::string_view>{
            "Test 8",
            "Test 9",
            "Test 10",
            "Test 11"}};

    // Check raw values first
    size_t expectedIdx = 0;
    for (int64_t i = 0; i < property.size(); ++i) {
      PropertyArrayView<std::string_view> values = property.getRaw(i);
      for (int64_t j = 0; j < values.size(); ++j) {
        std::string_view v = values[j];
        REQUIRE(v == strings[expectedIdx]);
        ++expectedIdx;
      }
    }

    REQUIRE(expectedIdx == stringCount);

    // Check values with properties
    for (int64_t i = 0; i < property.size(); ++i) {
      auto maybeValues = property.get(i);
      if (!maybeValues) {
        REQUIRE(!expected[static_cast<size_t>(i)]);
        continue;
      }

      auto values = *maybeValues;
      auto expectedValues = *expected[static_cast<size_t>(i)];
      for (int64_t j = 0; j < values.size(); ++j) {
        REQUIRE(values[j] == expectedValues[static_cast<size_t>(j)]);
      }
    }
  }
}

TEST_CASE("Check fixed-length boolean array PropertyTablePropertyView") {
  std::vector<std::byte> buffer{
      static_cast<std::byte>(0b10101111),
      static_cast<std::byte>(0b11111010),
      static_cast<std::byte>(0b11100111)};

  PropertyTableProperty propertyTableProperty;
  ClassProperty classProperty;
  classProperty.type = ClassProperty::Type::BOOLEAN;
  classProperty.array = true;
  classProperty.count = 12;

  PropertyTablePropertyView<PropertyArrayView<bool>> property(
      propertyTableProperty,
      classProperty,
      2,
      std::span<const std::byte>(buffer.data(), buffer.size()),
      std::span<const std::byte>(),
      std::span<const std::byte>(),
      PropertyComponentType::Uint32,
      PropertyComponentType::None);

  REQUIRE(property.size() == 2);
  REQUIRE(property.arrayCount() == classProperty.count);

  PropertyArrayView<bool> val0 = property.getRaw(0);
  REQUIRE(val0.size() == 12);
  REQUIRE(static_cast<int>(val0[0]) == 1);
  REQUIRE(static_cast<int>(val0[1]) == 1);
  REQUIRE(static_cast<int>(val0[2]) == 1);
  REQUIRE(static_cast<int>(val0[3]) == 1);
  REQUIRE(static_cast<int>(val0[4]) == 0);
  REQUIRE(static_cast<int>(val0[5]) == 1);
  REQUIRE(static_cast<int>(val0[6]) == 0);
  REQUIRE(static_cast<int>(val0[7]) == 1);
  REQUIRE(static_cast<int>(val0[8]) == 0);
  REQUIRE(static_cast<int>(val0[9]) == 1);
  REQUIRE(static_cast<int>(val0[10]) == 0);
  REQUIRE(static_cast<int>(val0[11]) == 1);

  PropertyArrayView<bool> val1 = property.getRaw(1);
  REQUIRE(static_cast<int>(val1[0]) == 1);
  REQUIRE(static_cast<int>(val1[1]) == 1);
  REQUIRE(static_cast<int>(val1[2]) == 1);
  REQUIRE(static_cast<int>(val1[3]) == 1);
  REQUIRE(static_cast<int>(val1[4]) == 1);
  REQUIRE(static_cast<int>(val1[5]) == 1);
  REQUIRE(static_cast<int>(val1[6]) == 1);
  REQUIRE(static_cast<int>(val1[7]) == 0);
  REQUIRE(static_cast<int>(val1[8]) == 0);
  REQUIRE(static_cast<int>(val1[9]) == 1);
  REQUIRE(static_cast<int>(val1[10]) == 1);
  REQUIRE(static_cast<int>(val1[11]) == 1);

  for (int64_t i = 0; i < property.size(); i++) {
    auto value = property.getRaw(i);
    auto maybeValue = property.get(i);
    REQUIRE(maybeValue);
    REQUIRE(maybeValue->size() == value.size());
    for (int64_t j = 0; j < maybeValue->size(); j++) {
      REQUIRE((*maybeValue)[j] == value[j]);
    }
  }
}

TEST_CASE("Check variable-length boolean array PropertyTablePropertyView") {
  std::vector<std::byte> buffer{
      static_cast<std::byte>(0b10101111),
      static_cast<std::byte>(0b11111010),
      static_cast<std::byte>(0b11100111),
      static_cast<std::byte>(0b11110110)};

  std::vector<uint32_t> offsetBuffer{0, 3, 12, 28};

  PropertyTableProperty propertyTableProperty;
  ClassProperty classProperty;
  classProperty.type = ClassProperty::Type::BOOLEAN;
  classProperty.array = true;

  PropertyTablePropertyView<PropertyArrayView<bool>> property(
      propertyTableProperty,
      classProperty,
      3,
      std::span<const std::byte>(buffer.data(), buffer.size()),
      std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(offsetBuffer.data()),
          offsetBuffer.size() * sizeof(uint32_t)),
      std::span<const std::byte>(),
      PropertyComponentType::Uint32,
      PropertyComponentType::None);

  REQUIRE(property.size() == 3);
  REQUIRE(property.arrayCount() == 0);

  PropertyArrayView<bool> val0 = property.getRaw(0);
  REQUIRE(val0.size() == 3);
  REQUIRE(static_cast<int>(val0[0]) == 1);
  REQUIRE(static_cast<int>(val0[1]) == 1);
  REQUIRE(static_cast<int>(val0[2]) == 1);

  PropertyArrayView<bool> val1 = property.getRaw(1);
  REQUIRE(val1.size() == 9);
  REQUIRE(static_cast<int>(val1[0]) == 1);
  REQUIRE(static_cast<int>(val1[1]) == 0);
  REQUIRE(static_cast<int>(val1[2]) == 1);
  REQUIRE(static_cast<int>(val1[3]) == 0);
  REQUIRE(static_cast<int>(val1[4]) == 1);
  REQUIRE(static_cast<int>(val1[5]) == 0);
  REQUIRE(static_cast<int>(val1[6]) == 1);
  REQUIRE(static_cast<int>(val1[7]) == 0);
  REQUIRE(static_cast<int>(val1[8]) == 1);

  PropertyArrayView<bool> val2 = property.getRaw(2);
  REQUIRE(val2.size() == 16);
  REQUIRE(static_cast<int>(val2[0]) == 1);
  REQUIRE(static_cast<int>(val2[1]) == 1);
  REQUIRE(static_cast<int>(val2[2]) == 1);
  REQUIRE(static_cast<int>(val2[3]) == 1);
  REQUIRE(static_cast<int>(val2[4]) == 1);
  REQUIRE(static_cast<int>(val2[5]) == 1);
  REQUIRE(static_cast<int>(val2[6]) == 1);
  REQUIRE(static_cast<int>(val2[7]) == 0);
  REQUIRE(static_cast<int>(val2[8]) == 0);
  REQUIRE(static_cast<int>(val2[9]) == 1);
  REQUIRE(static_cast<int>(val2[10]) == 1);
  REQUIRE(static_cast<int>(val2[11]) == 1);
  REQUIRE(static_cast<int>(val2[12]) == 0);
  REQUIRE(static_cast<int>(val2[13]) == 1);
  REQUIRE(static_cast<int>(val2[14]) == 1);
  REQUIRE(static_cast<int>(val2[15]) == 0);

  for (int64_t i = 0; i < property.size(); i++) {
    auto value = property.getRaw(i);
    auto maybeValue = property.get(i);
    REQUIRE(maybeValue);
    REQUIRE(maybeValue->size() == value.size());
    for (int64_t j = 0; j < maybeValue->size(); j++) {
      REQUIRE((*maybeValue)[j] == value[j]);
    }
  }
}

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
