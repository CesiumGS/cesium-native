#include "CesiumGltf/MetadataPropertyView.h"

#include <catch2/catch.hpp>
#include <gsl/span>

#include <bitset>
#include <climits>
#include <cstddef>
#include <cstring>
#include <vector>

template <typename T> static void checkNumeric(const std::vector<T>& expected) {
  std::vector<std::byte> data;
  data.resize(expected.size() * sizeof(T));
  std::memcpy(data.data(), expected.data(), data.size());

  CesiumGltf::MetadataPropertyView<T> property(
      CesiumGltf::MetadataPropertyViewStatus::Valid,
      gsl::span<const std::byte>(data.data(), data.size()),
      gsl::span<const std::byte>(),
      gsl::span<const std::byte>(),
      CesiumGltf::PropertyType::None,
      0,
      static_cast<int64_t>(expected.size()),
      false);

  for (int64_t i = 0; i < property.size(); ++i) {
    REQUIRE(property.get(i) == expected[static_cast<size_t>(i)]);
  }
}

template <typename T, typename E>
static void checkDynamicArray(
    const std::vector<T>& data,
    const std::vector<E> offset,
    CesiumGltf::PropertyType offsetType,
    int64_t instanceCount) {
  // copy data to buffer
  std::vector<std::byte> buffer;
  buffer.resize(data.size() * sizeof(T));
  std::memcpy(buffer.data(), data.data(), data.size() * sizeof(T));

  // copy offset to buffer
  std::vector<std::byte> offsetBuffer;
  offsetBuffer.resize(offset.size() * sizeof(E));
  std::memcpy(offsetBuffer.data(), offset.data(), offset.size() * sizeof(E));

  CesiumGltf::MetadataPropertyView<CesiumGltf::MetadataArrayView<T>> property(
      CesiumGltf::MetadataPropertyViewStatus::Valid,
      gsl::span<const std::byte>(buffer.data(), buffer.size()),
      gsl::span<const std::byte>(offsetBuffer.data(), offsetBuffer.size()),
      gsl::span<const std::byte>(),
      offsetType,
      0,
      instanceCount,
      false);

  size_t expectedIdx = 0;
  for (int64_t i = 0; i < property.size(); ++i) {
    CesiumGltf::MetadataArrayView<T> vals = property.get(i);
    for (int64_t j = 0; j < vals.size(); ++j) {
      REQUIRE(vals[j] == data[expectedIdx]);
      ++expectedIdx;
    }
  }

  REQUIRE(expectedIdx == data.size());
}

template <typename T>
static void checkFixedArray(
    const std::vector<T>& data,
    int64_t componentCount,
    int64_t instanceCount) {
  std::vector<std::byte> buffer;
  buffer.resize(data.size() * sizeof(T));
  std::memcpy(buffer.data(), data.data(), data.size() * sizeof(T));

  CesiumGltf::MetadataPropertyView<CesiumGltf::MetadataArrayView<T>> property(
      CesiumGltf::MetadataPropertyViewStatus::Valid,
      gsl::span<const std::byte>(buffer.data(), buffer.size()),
      gsl::span<const std::byte>(),
      gsl::span<const std::byte>(),
      CesiumGltf::PropertyType::None,
      componentCount,
      instanceCount,
      false);

  size_t expectedIdx = 0;
  for (int64_t i = 0; i < property.size(); ++i) {
    CesiumGltf::MetadataArrayView<T> vals = property.get(i);
    for (int64_t j = 0; j < vals.size(); ++j) {
      REQUIRE(vals[j] == data[expectedIdx]);
      ++expectedIdx;
    }
  }

  REQUIRE(expectedIdx == data.size());
}

TEST_CASE("Check create numeric property view") {
  SECTION("Uint8") {
    std::vector<uint8_t> data{12, 33, 56, 67};
    checkNumeric(data);
  }

  SECTION("Int32") {
    std::vector<int32_t> data{111222, -11133, -56000, 670000};
    checkNumeric(data);
  }

  SECTION("Float") {
    std::vector<float> data{12.3333f, -12.44555f, -5.6111f, 6.7421f};
    checkNumeric(data);
  }

  SECTION("Double") {
    std::vector<double> data{
        12222.3302121,
        -12000.44555,
        -5000.6113111,
        6.7421};
    checkNumeric(data);
  }
}

TEST_CASE("Check boolean value") {
  std::bitset<sizeof(unsigned long)* CHAR_BIT> bits = 0b11110101;
  unsigned long val = bits.to_ulong();
  std::vector<std::byte> data(sizeof(val));
  std::memcpy(data.data(), &val, sizeof(val));

  size_t instanceCount = sizeof(unsigned long) * CHAR_BIT;
  CesiumGltf::MetadataPropertyView<bool> property(
      CesiumGltf::MetadataPropertyViewStatus::Valid,
      gsl::span<const std::byte>(data.data(), data.size()),
      gsl::span<const std::byte>(),
      gsl::span<const std::byte>(),
      CesiumGltf::PropertyType::None,
      0,
      static_cast<int64_t>(instanceCount),
      false);
  for (int64_t i = 0; i < property.size(); ++i) {
    REQUIRE(property.get(i) == bits[static_cast<size_t>(i)]);
  }
}

TEST_CASE("Check string value") {
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

  CesiumGltf::MetadataPropertyView<std::string_view> property(
      CesiumGltf::MetadataPropertyViewStatus::Valid,
      gsl::span<const std::byte>(buffer.data(), buffer.size()),
      gsl::span<const std::byte>(),
      gsl::span<const std::byte>(offsetBuffer.data(), offsetBuffer.size()),
      CesiumGltf::PropertyType::Uint32,
      0,
      static_cast<int64_t>(strings.size()),
      false);
  for (int64_t i = 0; i < property.size(); ++i) {
    REQUIRE(property.get(i) == strings[static_cast<size_t>(i)]);
  }
}

TEST_CASE("Check fixed numeric array") {
  SECTION("Fixed array of 4 uint8_ts") {
    // clang-format off
    std::vector<uint8_t> data{
        210, 211, 3, 42, 
        122, 22, 1, 45};
    // clang-format on
    checkFixedArray(data, 4, static_cast<int64_t>(data.size() / 4));
  }

  SECTION("Fixed array of 3 int8_ts") {
    // clang-format off
    std::vector<int8_t> data{
        122, -12, 3, 
        44, 11, -2, 
        5, 6, -22, 
        5, 6, 1};
    // clang-format on
    checkFixedArray(data, 3, static_cast<int64_t>(data.size() / 3));
  }

  SECTION("Fixed array of 4 int16_ts") {
    // clang-format off
    std::vector<int16_t> data{
        -122, 12, 3, 44, 
        11, 2, 5, -6000, 
        119, 30, 51, 200, 
        22000, -500, 6000, 1};
    // clang-format on
    checkFixedArray(data, 4, static_cast<int64_t>(data.size() / 4));
  }

  SECTION("Fixed array of 6 uint32_ts") {
    // clang-format off
    std::vector<uint32_t> data{
        122, 12, 3, 44, 34444, 2222,
        11, 2, 5, 6000, 1111, 2222,
        119, 30, 51, 200, 12534, 11,
        22000, 500, 6000, 1, 3, 7};
    // clang-format on
    checkFixedArray(data, 6, static_cast<int64_t>(data.size() / 6));
  }

  SECTION("Fixed array of 2 int32_ts") {
    // clang-format off
    std::vector<uint32_t> data{
        122, 12, 
        3, 44};
    // clang-format on
    checkFixedArray(data, 2, static_cast<int64_t>(data.size() / 2));
  }

  SECTION("Fixed array of 4 uint64_ts") {
    // clang-format off
    std::vector<uint64_t> data{
        10022, 120000, 2422, 1111, 
        3, 440000, 333, 1455};
    // clang-format on
    checkFixedArray(data, 4, static_cast<int64_t>(data.size() / 4));
  }

  SECTION("Fixed array of 4 int64_ts") {
    // clang-format off
    std::vector<int64_t> data{
        10022, -120000, 2422, 1111, 
        3, 440000, -333, 1455};
    // clang-format on
    checkFixedArray(data, 4, static_cast<int64_t>(data.size() / 4));
  }

  SECTION("Fixed array of 4 floats") {
    // clang-format off
    std::vector<float> data{
        10.022f, -12.43f, 242.2f, 1.111f, 
        3.333f, 440000.1f, -33.3f, 14.55f};
    // clang-format on
    checkFixedArray(data, 4, static_cast<int64_t>(data.size() / 4));
  }

  SECTION("Fixed array of 4 double") {
    // clang-format off
    std::vector<double> data{
        10.022, -12.43, 242.2, 1.111, 
        3.333, 440000.1, -33.3, 14.55};
    // clang-format on
    checkFixedArray(data, 4, static_cast<int64_t>(data.size() / 4));
  }
}

TEST_CASE("Check numeric dynamic array") {
  SECTION("array of uint8_t") {
    // clang-format off
    std::vector<uint8_t> data{
        3, 2,
        0, 45, 2, 1, 4,
        1, 3, 2,
        1, 3, 4, 1
    };
    std::vector<uint32_t> offset{
        0, 2, 7, 10, 14
    };
    // clang-format on

    checkDynamicArray(data, offset, CesiumGltf::PropertyType::Uint32, 4);
  }

  SECTION("array of int32_t") {
    // clang-format off
    std::vector<int32_t> data{
        3, 200,
        0, 450, 200, 1, 4,
        1, 3, 2,
        1, 3, 4, 1
    };
    std::vector<uint32_t> offset{
        0, 2 * sizeof(int32_t), 7 * sizeof(int32_t), 10 * sizeof(int32_t), 14 * sizeof(int32_t)
    };
    // clang-format on

    checkDynamicArray(data, offset, CesiumGltf::PropertyType::Uint32, 4);
  }

  SECTION("array of double") {
    // clang-format off
    std::vector<double> data{
        3.333, 200.2,
        0.1122, 4.50, 2.30, 1.22, 4.444,
        1.4, 3.3, 2.2,
        1.11, 3.2, 4.111, 1.44
    };
    std::vector<uint32_t> offset{
        0, 2 * sizeof(double), 7 * sizeof(double), 10 * sizeof(double), 14 * sizeof(double)
    };
    // clang-format on

    checkDynamicArray(data, offset, CesiumGltf::PropertyType::Uint32, 4);
  }
}

TEST_CASE("Check fixed array of string") {
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

  CesiumGltf::MetadataPropertyView<
      CesiumGltf::MetadataArrayView<std::string_view>>
      property(
          CesiumGltf::MetadataPropertyViewStatus::Valid,
          gsl::span<const std::byte>(buffer.data(), buffer.size()),
          gsl::span<const std::byte>(),
          gsl::span<const std::byte>(offsetBuffer.data(), offsetBuffer.size()),
          CesiumGltf::PropertyType::Uint32,
          3,
          static_cast<int64_t>(strings.size() / 3),
          false);

  size_t expectedIdx = 0;
  for (int64_t i = 0; i < property.size(); ++i) {
    CesiumGltf::MetadataArrayView<std::string_view> vals = property.get(i);
    for (int64_t j = 0; j < vals.size(); ++j) {
      std::string_view v = vals[j];
      REQUIRE(v == strings[expectedIdx]);
      ++expectedIdx;
    }
  }

  REQUIRE(expectedIdx == strings.size());
}

TEST_CASE("Check dynamic array of string") {
  // clang-format off
  std::vector<uint32_t> arrayOffset{
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

  CesiumGltf::MetadataPropertyView<
      CesiumGltf::MetadataArrayView<std::string_view>>
      property(
          CesiumGltf::MetadataPropertyViewStatus::Valid,
          gsl::span<const std::byte>(buffer.data(), buffer.size()),
          gsl::span<const std::byte>(
              reinterpret_cast<const std::byte*>(arrayOffset.data()),
              arrayOffset.size() * sizeof(uint32_t)),
          gsl::span<const std::byte>(offsetBuffer.data(), offsetBuffer.size()),
          CesiumGltf::PropertyType::Uint32,
          0,
          3,
          false);

  size_t expectedIdx = 0;
  for (int64_t i = 0; i < property.size(); ++i) {
    CesiumGltf::MetadataArrayView<std::string_view> vals = property.get(i);
    for (int64_t j = 0; j < vals.size(); ++j) {
      std::string_view v = vals[j];
      REQUIRE(v == strings[expectedIdx]);
      ++expectedIdx;
    }
  }

  REQUIRE(expectedIdx == strings.size());
}

TEST_CASE("Check fixed array of boolean") {
  std::vector<std::byte> buffer{
      static_cast<std::byte>(0b10101111),
      static_cast<std::byte>(0b11111010),
      static_cast<std::byte>(0b11100111)};

  CesiumGltf::MetadataPropertyView<CesiumGltf::MetadataArrayView<bool>>
      property(
          CesiumGltf::MetadataPropertyViewStatus::Valid,
          gsl::span<const std::byte>(buffer.data(), buffer.size()),
          gsl::span<const std::byte>(),
          gsl::span<const std::byte>(),
          CesiumGltf::PropertyType::Uint32,
          12,
          2,
          false);

  REQUIRE(property.size() == 2);

  CesiumGltf::MetadataArrayView<bool> val0 = property.get(0);
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

  CesiumGltf::MetadataArrayView<bool> val1 = property.get(1);
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

TEST_CASE("Check dynamic array of boolean") {
  std::vector<std::byte> buffer{
      static_cast<std::byte>(0b10101111),
      static_cast<std::byte>(0b11111010),
      static_cast<std::byte>(0b11100111),
      static_cast<std::byte>(0b11110110)};

  std::vector<uint32_t> offsetBuffer{0, 3, 12, 28};

  CesiumGltf::MetadataPropertyView<CesiumGltf::MetadataArrayView<bool>>
      property(
          CesiumGltf::MetadataPropertyViewStatus::Valid,
          gsl::span<const std::byte>(buffer.data(), buffer.size()),
          gsl::span<const std::byte>(
              reinterpret_cast<const std::byte*>(offsetBuffer.data()),
              offsetBuffer.size() * sizeof(uint32_t)),
          gsl::span<const std::byte>(),
          CesiumGltf::PropertyType::Uint32,
          0,
          3,
          false);

  REQUIRE(property.size() == 3);

  CesiumGltf::MetadataArrayView<bool> val0 = property.get(0);
  REQUIRE(val0.size() == 3);
  REQUIRE(static_cast<int>(val0[0]) == 1);
  REQUIRE(static_cast<int>(val0[1]) == 1);
  REQUIRE(static_cast<int>(val0[2]) == 1);

  CesiumGltf::MetadataArrayView<bool> val1 = property.get(1);
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

  CesiumGltf::MetadataArrayView<bool> val2 = property.get(2);
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
