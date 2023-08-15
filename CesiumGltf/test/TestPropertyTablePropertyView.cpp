#include "CesiumGltf/PropertyTablePropertyView.h"

#include <catch2/catch.hpp>
#include <gsl/span>

#include <bitset>
#include <climits>
#include <cstddef>
#include <cstring>
#include <vector>

using namespace CesiumGltf;

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
      gsl::span<const std::byte>(data.data(), data.size()));

  for (int64_t i = 0; i < property.size(); ++i) {
    REQUIRE(property.get(i) == expected[static_cast<size_t>(i)]);
  }
}

template <typename DataType, typename OffsetType>
static void checkVariableLengthArray(
    const std::vector<DataType>& data,
    const std::vector<OffsetType> offsets,
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
      gsl::span<const std::byte>(buffer.data(), buffer.size()),
      gsl::span<const std::byte>(offsetBuffer.data(), offsetBuffer.size()),
      gsl::span<const std::byte>(),
      offsetType,
      PropertyComponentType::None);

  REQUIRE(property.arrayCount() == 0);

  size_t expectedIdx = 0;
  for (int64_t i = 0; i < property.size(); ++i) {
    PropertyArrayView<DataType> values = property.get(i);
    for (int64_t j = 0; j < values.size(); ++j) {
      REQUIRE(values[j] == data[expectedIdx]);
      ++expectedIdx;
    }
  }

  REQUIRE(expectedIdx == data.size());
}

template <typename T>
static void checkFixedLengthArray(
    const std::vector<T>& data,
    int64_t fixedLengthArrayCount,
    int64_t instanceCount) {
  std::vector<std::byte> buffer;
  buffer.resize(data.size() * sizeof(T));
  std::memcpy(buffer.data(), data.data(), data.size() * sizeof(T));

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
      gsl::span<const std::byte>(buffer.data(), buffer.size()),
      gsl::span<const std::byte>(),
      gsl::span<const std::byte>(),
      PropertyComponentType::None,
      PropertyComponentType::None);

  REQUIRE(property.arrayCount() == fixedLengthArrayCount);

  size_t expectedIdx = 0;
  for (int64_t i = 0; i < property.size(); ++i) {
    PropertyArrayView<T> values = property.get(i);
    for (int64_t j = 0; j < values.size(); ++j) {
      REQUIRE(values[j] == data[expectedIdx]);
      ++expectedIdx;
    }
  }

  REQUIRE(expectedIdx == data.size());
}

TEST_CASE("Check scalar PropertyTablePropertyView") {
  SECTION("Uint8 Scalar") {
    std::vector<uint8_t> data{12, 33, 56, 67};
    checkNumeric(data);
  }

  SECTION("Int32 Scalar") {
    std::vector<int32_t> data{111222, -11133, -56000, 670000};
    checkNumeric(data);
  }

  SECTION("Float Scalar") {
    std::vector<float> data{12.3333f, -12.44555f, -5.6111f, 6.7421f};
    checkNumeric(data);
  }

  SECTION("Double Scalar") {
    std::vector<double> data{
        12222.3302121,
        -12000.44555,
        -5000.6113111,
        6.7421};
    checkNumeric(data);
  }
}

TEST_CASE("Check vecN PropertyTablePropertyView") {
  SECTION("Float Vec2") {
    std::vector<glm::vec2> data{
        glm::vec2(10.001f, 0.005f),
        glm::vec2(-20.8f, 50.0f),
        glm::vec2(99.9f, -9.9f),
        glm::vec2(-64.456f, 12.01f)};
    checkNumeric(data);
  }

  SECTION("Int32 Vec3") {
    std::vector<glm::ivec3> data{
        glm::ivec3(10, 0, -3),
        glm::ivec3(-20, 10, 52),
        glm::ivec3(9, 9, -9),
        glm::ivec3(8, -40, 2)};
    checkNumeric(data);
  }

  SECTION("Uint8 Vec4") {
    std::vector<glm::u8vec4> data{
        glm::u8vec4(1, 2, 3, 0),
        glm::u8vec4(4, 5, 6, 1),
        glm::u8vec4(7, 8, 9, 0),
        glm::u8vec4(0, 0, 0, 1)};
    checkNumeric(data);
  }
}

TEST_CASE("Check matN PropertyTablePropertyView") {
  SECTION("Float Mat2") {
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

  SECTION("Int16 Mat3") {
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

  SECTION("Double Mat4") {
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
      gsl::span<const std::byte>(data.data(), data.size()));

  for (int64_t i = 0; i < property.size(); ++i) {
    REQUIRE(property.get(i) == bits[static_cast<size_t>(i)]);
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

  PropertyTableProperty propertyTableProperty;
  ClassProperty classProperty;
  classProperty.type = ClassProperty::Type::STRING;

  PropertyTablePropertyView<std::string_view> property(
      propertyTableProperty,
      classProperty,
      static_cast<int64_t>(strings.size()),
      gsl::span<const std::byte>(buffer.data(), buffer.size()),
      gsl::span<const std::byte>(),
      gsl::span<const std::byte>(offsetBuffer.data(), offsetBuffer.size()),
      PropertyComponentType::None,
      PropertyComponentType::Uint32);

  for (int64_t i = 0; i < property.size(); ++i) {
    REQUIRE(property.get(i) == strings[static_cast<size_t>(i)]);
  }
}

TEST_CASE("Check fixed-length scalar array PropertyTablePropertyView") {
  SECTION("Fixed-length array of 4 uint8_ts") {
    // clang-format off
    std::vector<uint8_t> data{
        210, 211, 3, 42,
        122, 22, 1, 45};
    // clang-format on
    checkFixedLengthArray(data, 4, static_cast<int64_t>(data.size() / 4));
  }

  SECTION("Fixed-length array of 3 int8_ts") {
    // clang-format off
    std::vector<int8_t> data{
        122, -12, 3,
        44, 11, -2,
        5, 6, -22,
        5, 6, 1};
    // clang-format on
    checkFixedLengthArray(data, 3, static_cast<int64_t>(data.size() / 3));
  }

  SECTION("Fixed-length array of 4 int16_ts") {
    // clang-format off
    std::vector<int16_t> data{
        -122, 12, 3, 44,
        11, 2, 5, -6000,
        119, 30, 51, 200,
        22000, -500, 6000, 1};
    // clang-format on
    checkFixedLengthArray(data, 4, static_cast<int64_t>(data.size() / 4));
  }

  SECTION("Fixed-length array of 6 uint32_ts") {
    // clang-format off
    std::vector<uint32_t> data{
        122, 12, 3, 44, 34444, 2222,
        11, 2, 5, 6000, 1111, 2222,
        119, 30, 51, 200, 12534, 11,
        22000, 500, 6000, 1, 3, 7};
    // clang-format on
    checkFixedLengthArray(data, 6, static_cast<int64_t>(data.size() / 6));
  }

  SECTION("Fixed-length array of 2 int32_ts") {
    // clang-format off
    std::vector<uint32_t> data{
        122, 12,
        3, 44};
    // clang-format on
    checkFixedLengthArray(data, 2, static_cast<int64_t>(data.size() / 2));
  }

  SECTION("Fixed-length array of 4 uint64_ts") {
    // clang-format off
    std::vector<uint64_t> data{
        10022, 120000, 2422, 1111,
        3, 440000, 333, 1455};
    // clang-format on
    checkFixedLengthArray(data, 4, static_cast<int64_t>(data.size() / 4));
  }

  SECTION("Fixed-length array of 4 int64_ts") {
    // clang-format off
    std::vector<int64_t> data{
        10022, -120000, 2422, 1111,
        3, 440000, -333, 1455};
    // clang-format on
    checkFixedLengthArray(data, 4, static_cast<int64_t>(data.size() / 4));
  }

  SECTION("Fixed-length array of 4 floats") {
    // clang-format off
    std::vector<float> data{
        10.022f, -12.43f, 242.2f, 1.111f,
        3.333f, 440000.1f, -33.3f, 14.55f};
    // clang-format on
    checkFixedLengthArray(data, 4, static_cast<int64_t>(data.size() / 4));
  }

  SECTION("Fixed-length array of 4 double") {
    // clang-format off
    std::vector<double> data{
        10.022, -12.43, 242.2, 1.111,
        3.333, 440000.1, -33.3, 14.55};
    // clang-format on
    checkFixedLengthArray(data, 4, static_cast<int64_t>(data.size() / 4));
  }
}

TEST_CASE("Check fixed-length vecN array PropertyTablePropertyView") {
  SECTION("Fixed-length array of 4 u8vec2s") {
    // clang-format off
    std::vector<glm::u8vec2> data{
        glm::u8vec2(10, 21),
        glm::u8vec2(3, 42),
        glm::u8vec2(122, 22),
        glm::u8vec2(1, 45),
        glm::u8vec2(0, 0),
        glm::u8vec2(32, 12),
        glm::u8vec2(8, 19),
        glm::u8vec2(6, 5)};
    // clang-format on
    checkFixedLengthArray(data, 4, static_cast<int64_t>(data.size() / 4));
  }

  SECTION("Fixed-length array of 2 i8vec3s") {
    // clang-format off
    std::vector<glm::i8vec3> data{
        glm::i8vec3(122, -12, 3),
        glm::i8vec3(44, 11, -2),
        glm::i8vec3(5, 6, -22),
        glm::i8vec3(5, 6, 1),
        glm::i8vec3(8, -7, 7),
        glm::i8vec3(-4, 36, 17)};
    // clang-format on
    checkFixedLengthArray(data, 2, static_cast<int64_t>(data.size() / 2));
  }

  SECTION("Fixed-length array of 3 vec4s") {
    // clang-format off
    std::vector<glm::vec4> data{
        glm::vec4(40.2f, -1.2f, 8.8f, 1.0f),
        glm::vec4(1.4f, 0.11, 34.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        glm::vec4(1.0f, 2.0f, 3.0f, 6.0f),
        glm::vec4(1.08f, -3.71f, 18.0f, -7.0f),
        glm::vec4(-17.0f, 33.0f, 8.0f, -3.0f)};
    // clang-format on
    checkFixedLengthArray(data, 3, static_cast<int64_t>(data.size() / 3));
  }
}

TEST_CASE("Check fixed-length matN array PropertyTablePropertyView") {
  SECTION("Fixed-length array of 4 i8mat2x2") {
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
    checkFixedLengthArray(data, 4, static_cast<int64_t>(data.size() / 4));
  }

  SECTION("Fixed-length array of 2 dmat3s") {
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
    checkFixedLengthArray(data, 2, static_cast<int64_t>(data.size() / 2));
  }

  SECTION("Fixed-length array of 3 u8mat4x4") {
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
    checkFixedLengthArray(data, 3, static_cast<int64_t>(data.size() / 3));
  }
}

TEST_CASE("Check variable-length scalar array PropertyTablePropertyView") {
  SECTION("Variable-length array of uint8_t") {
    // clang-format off
    std::vector<uint8_t> data{
        3, 2,
        0, 45, 2, 1, 4,
        1, 3, 2,
        1, 3, 4, 1
    };
    std::vector<uint32_t> offsets{
        0, 2, 7, 10, 14
    };
    // clang-format on

    checkVariableLengthArray(data, offsets, PropertyComponentType::Uint32, 4);
  }

  SECTION("Variable-length array of int32_t") {
    // clang-format off
    std::vector<int32_t> data{
        3, 200,
        0, 450, 200, 1, 4,
        1, 3, 2,
        1, 3, 4, 1
    };
    std::vector<uint32_t> offsets{
        0, 2 * sizeof(int32_t), 7 * sizeof(int32_t), 10 * sizeof(int32_t), 14
        * sizeof(int32_t)
    };
    // clang-format on

    checkVariableLengthArray(data, offsets, PropertyComponentType::Uint32, 4);
  }

  SECTION("Variable-length array of double") {
    // clang-format off
    std::vector<double> data{
        3.333, 200.2,
        0.1122, 4.50, 2.30, 1.22, 4.444,
        1.4, 3.3, 2.2,
        1.11, 3.2, 4.111, 1.44
    };
    std::vector<uint32_t> offsets{
        0, 2 * sizeof(double), 7 * sizeof(double), 10 * sizeof(double), 14 *
        sizeof(double)
    };
    // clang-format on

    checkVariableLengthArray(data, offsets, PropertyComponentType::Uint32, 4);
  }
}

TEST_CASE("Check variable-length vecN array PropertyTablePropertyView") {
  SECTION("Variable-length array of ivec2") {
    // clang-format off
    std::vector<glm::ivec2> data{
        glm::ivec2(3, -2), glm::ivec2(20, 3),
        glm::ivec2(0, 45), glm::ivec2(-10, 2), glm::ivec2(4, 4), glm::ivec2(1, -1),
        glm::ivec2(3, 1), glm::ivec2(3, 2), glm::ivec2(0, -5),
        glm::ivec2(-9, 10), glm::ivec2(8, -2)
    };
    // clang-format on
    std::vector<uint32_t> offsets{
        0,
        2 * sizeof(glm::ivec2),
        6 * sizeof(glm::ivec2),
        9 * sizeof(glm::ivec2),
        11 * sizeof(glm::ivec2)};

    checkVariableLengthArray(data, offsets, PropertyComponentType::Uint32, 4);
  }

  SECTION("Variable-length array of dvec3") {
    // clang-format off
    std::vector<glm::dvec3> data{
        glm::dvec3(-0.02, 2.0, 1.0), glm::dvec3(9.92, 9.0, -8.0),
        glm::dvec3(22.0, 5.5, -3.7), glm::dvec3(1.02, 9.0, -8.0), glm::dvec3(0.0, 0.5, 1.0),
        glm::dvec3(-1.3, -5.0, -90.0),
        glm::dvec3(4.4, 1.0, 2.3), glm::dvec3(5.8, 7.07, -4.0), 
        glm::dvec3(-2.0, 0.85, 0.22), glm::dvec3(-8.8, 5.1, 0.0), glm::dvec3(12.0, 8.0, -2.2),
    };
    // clang-format on
    std::vector<uint32_t> offsets{
        0,
        2 * sizeof(glm::dvec3),
        5 * sizeof(glm::dvec3),
        6 * sizeof(glm::dvec3),
        8 * sizeof(glm::dvec3),
        11 * sizeof(glm::dvec3)};

    checkVariableLengthArray(data, offsets, PropertyComponentType::Uint32, 5);
  }

  SECTION("Variable-length array of u8vec4") {
    // clang-format off
     std::vector<glm::u8vec4> data{
         glm::u8vec4(1, 2, 3, 4), glm::u8vec4(5, 6, 7, 8),
         glm::u8vec4(9, 2, 1, 0),
         glm::u8vec4(8, 7, 10, 21), glm::u8vec4(3, 6, 8, 0), glm::u8vec4(0, 0, 0, 1),
         glm::u8vec4(64, 8, 17, 5), glm::u8vec4(35, 23, 10, 0),
         glm::u8vec4(99, 8, 1, 2)
     };
    // clang-format on

    std::vector<uint32_t> offsets{
        0,
        2 * sizeof(glm::u8vec4),
        3 * sizeof(glm::u8vec4),
        6 * sizeof(glm::u8vec4),
        8 * sizeof(glm::u8vec4),
        9 * sizeof(glm::u8vec4)};

    checkVariableLengthArray(data, offsets, PropertyComponentType::Uint32, 5);
  }
}

TEST_CASE("Check variable-length matN array PropertyTablePropertyView") {
  SECTION("Variable-length array of dmat2") {
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

    std::vector<uint32_t> offsets{
        0,
        2 * sizeof(glm::dmat2),
        3 * sizeof(glm::dmat2),
        6 * sizeof(glm::dmat2),
    };

    checkVariableLengthArray(data, offsets, PropertyComponentType::Uint32, 3);
  }

  SECTION("Variable-length array of i16mat3x3") {
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

    std::vector<uint32_t> offsets{
        0,
        1 * sizeof(glm::i16mat3x3),
        4 * sizeof(glm::i16mat3x3),
        6 * sizeof(glm::i16mat3x3)};

    checkVariableLengthArray(data, offsets, PropertyComponentType::Uint32, 3);
  }

  SECTION("Variable-length array of u8mat4x4") {
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

    std::vector<uint32_t> offsets{
        0,
        3 * sizeof(glm::u8mat4x4),
        4 * sizeof(glm::u8mat4x4),
        6 * sizeof(glm::u8mat4x4)};

    checkVariableLengthArray(data, offsets, PropertyComponentType::Uint32, 3);
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

  PropertyTableProperty propertyTableProperty;
  ClassProperty classProperty;
  classProperty.type = ClassProperty::Type::STRING;
  classProperty.array = true;
  classProperty.count = 3;

  PropertyTablePropertyView<PropertyArrayView<std::string_view>> property(
      propertyTableProperty,
      classProperty,
      static_cast<int64_t>(stringCount / 3),
      gsl::span<const std::byte>(buffer.data(), buffer.size()),
      gsl::span<const std::byte>(),
      gsl::span<const std::byte>(stringOffsets.data(), stringOffsets.size()),
      PropertyComponentType::None,
      PropertyComponentType::Uint32);

  REQUIRE(property.arrayCount() == classProperty.count);

  size_t expectedIdx = 0;
  for (int64_t i = 0; i < property.size(); ++i) {
    PropertyArrayView<std::string_view> values = property.get(i);
    for (int64_t j = 0; j < values.size(); ++j) {
      std::string_view v = values[j];
      REQUIRE(v == strings[expectedIdx]);
      ++expectedIdx;
    }
  }

  REQUIRE(expectedIdx == stringCount);
}

TEST_CASE("Check variable-length string array PropertyTablePropertyView") {
  // clang-format off
  std::vector<uint32_t> arrayOffsets{
    0,
    4 * sizeof(uint32_t),
    7 * sizeof(uint32_t),
    11 * sizeof(uint32_t)
  };

  std::vector<std::string> strings{
    "Test 1", "Test 2", "Test 3", "Test 4",
    "Test 5", "Test 6", "Test 7",
    "test 8", "Test 9", "Test 10", "Test 11"
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

  PropertyTableProperty propertyTableProperty;
  ClassProperty classProperty;
  classProperty.type = ClassProperty::Type::STRING;
  classProperty.array = true;

  PropertyTablePropertyView<PropertyArrayView<std::string_view>> property(
      propertyTableProperty,
      classProperty,
      3,
      gsl::span<const std::byte>(buffer.data(), buffer.size()),
      gsl::span<const std::byte>(
          reinterpret_cast<const std::byte*>(arrayOffsets.data()),
          arrayOffsets.size() * sizeof(uint32_t)),
      gsl::span<const std::byte>(stringOffsets.data(), stringOffsets.size()),
      PropertyComponentType::Uint32,
      PropertyComponentType::Uint32);

  REQUIRE(property.arrayCount() == 0);

  size_t expectedIdx = 0;
  for (int64_t i = 0; i < property.size(); ++i) {
    PropertyArrayView<std::string_view> values = property.get(i);
    for (int64_t j = 0; j < values.size(); ++j) {
      std::string_view v = values[j];
      REQUIRE(v == strings[expectedIdx]);
      ++expectedIdx;
    }
  }

  REQUIRE(expectedIdx == stringCount);
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
      gsl::span<const std::byte>(buffer.data(), buffer.size()),
      gsl::span<const std::byte>(),
      gsl::span<const std::byte>(),
      PropertyComponentType::Uint32,
      PropertyComponentType::None);

  REQUIRE(property.size() == 2);
  REQUIRE(property.arrayCount() == classProperty.count);

  PropertyArrayView<bool> val0 = property.get(0);
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

  PropertyArrayView<bool> val1 = property.get(1);
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
      gsl::span<const std::byte>(buffer.data(), buffer.size()),
      gsl::span<const std::byte>(
          reinterpret_cast<const std::byte*>(offsetBuffer.data()),
          offsetBuffer.size() * sizeof(uint32_t)),
      gsl::span<const std::byte>(),
      PropertyComponentType::Uint32,
      PropertyComponentType::None);

  REQUIRE(property.size() == 3);
  REQUIRE(property.arrayCount() == 0);

  PropertyArrayView<bool> val0 = property.get(0);
  REQUIRE(val0.size() == 3);
  REQUIRE(static_cast<int>(val0[0]) == 1);
  REQUIRE(static_cast<int>(val0[1]) == 1);
  REQUIRE(static_cast<int>(val0[2]) == 1);

  PropertyArrayView<bool> val1 = property.get(1);
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

  PropertyArrayView<bool> val2 = property.get(2);
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
}
