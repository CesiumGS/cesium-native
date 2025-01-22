#include <CesiumGltf/ClassProperty.h>
#include <CesiumGltf/ExtensionKhrTextureTransform.h>
#include <CesiumGltf/ImageAsset.h>
#include <CesiumGltf/PropertyArrayView.h>
#include <CesiumGltf/PropertyTextureProperty.h>
#include <CesiumGltf/PropertyTransformations.h>
#include <CesiumGltf/PropertyType.h>
#include <CesiumGltf/PropertyTypeTraits.h>
#include <CesiumGltf/Sampler.h>
#include <CesiumGltf/TextureView.h>
#include <CesiumUtility/JsonValue.h>

#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_double4.hpp>
#include <glm/ext/vector_int2_sized.hpp>
#include <glm/ext/vector_int3_sized.hpp>
#include <glm/ext/vector_int4_sized.hpp>
#include <glm/ext/vector_uint2_sized.hpp>
#include <glm/ext/vector_uint3_sized.hpp>
#include <glm/ext/vector_uint4_sized.hpp>

#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

#include <CesiumGltf/KhrTextureTransform.h>
#include <CesiumGltf/PropertyTexturePropertyView.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>

#include <climits>
#include <cstddef>
#include <vector>

using namespace CesiumGltf;
using namespace CesiumUtility;

namespace {
template <typename T>
void checkTextureValues(
    const std::vector<uint8_t>& data,
    const std::vector<T>& expected) {
  PropertyTextureProperty property;
  ClassProperty classProperty;
  classProperty.type =
      convertPropertyTypeToString(TypeToPropertyType<T>::value);

  PropertyComponentType componentType = TypeToPropertyType<T>::component;
  classProperty.componentType =
      convertPropertyComponentTypeToString(componentType);

  Sampler sampler;
  ImageAsset image;
  image.width = 2;
  image.height = 2;
  image.channels = static_cast<int32_t>(sizeof(T));
  image.bytesPerChannel = 1;

  std::vector<std::byte>& imageData = image.pixelData;
  imageData.resize(data.size());
  std::memcpy(imageData.data(), data.data(), data.size());

  property.channels.resize(static_cast<size_t>(image.channels));
  for (size_t i = 0; i < property.channels.size(); i++) {
    property.channels[i] = static_cast<int64_t>(i);
  }

  PropertyTexturePropertyView<T> view(property, classProperty, sampler, image);
  switch (sizeof(T)) {
  case 1:
    CHECK(view.getSwizzle() == "r");
    break;
  case 2:
    CHECK(view.getSwizzle() == "rg");
    break;
  case 3:
    CHECK(view.getSwizzle() == "rgb");
    break;
  case 4:
    CHECK(view.getSwizzle() == "rgba");
    break;
  default:
    FAIL("Invalid property texture property view type");
  }

  REQUIRE(!view.normalized());

  std::vector<glm::dvec2> texCoords{
      glm::dvec2(0, 0),
      glm::dvec2(0.5, 0),
      glm::dvec2(0, 0.5),
      glm::dvec2(0.5, 0.5)};

  for (size_t i = 0; i < texCoords.size(); i++) {
    glm::dvec2 uv = texCoords[i];
    REQUIRE(view.getRaw(uv[0], uv[1]) == expected[i]);
    REQUIRE(view.get(uv[0], uv[1]) == expected[i]);
  }
}

template <typename T>
void checkTextureValues(
    const std::vector<uint8_t>& data,
    const std::vector<T>& expectedRaw,
    const std::vector<std::optional<T>>& expectedTransformed,
    const std::optional<JsonValue> offset = std::nullopt,
    const std::optional<JsonValue> scale = std::nullopt,
    const std::optional<JsonValue> noData = std::nullopt,
    const std::optional<JsonValue> defaultValue = std::nullopt) {
  PropertyTextureProperty property;
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

  Sampler sampler;
  ImageAsset image;
  image.width = 2;
  image.height = 2;
  image.channels = static_cast<int32_t>(sizeof(T));
  image.bytesPerChannel = 1;

  std::vector<std::byte>& imageData = image.pixelData;
  imageData.resize(data.size());
  std::memcpy(imageData.data(), data.data(), data.size());

  property.channels.resize(static_cast<size_t>(image.channels));
  for (size_t i = 0; i < property.channels.size(); i++) {
    property.channels[i] = static_cast<int64_t>(i);
  }

  PropertyTexturePropertyView<T> view(property, classProperty, sampler, image);
  switch (sizeof(T)) {
  case 1:
    CHECK(view.getSwizzle() == "r");
    break;
  case 2:
    CHECK(view.getSwizzle() == "rg");
    break;
  case 3:
    CHECK(view.getSwizzle() == "rgb");
    break;
  case 4:
    CHECK(view.getSwizzle() == "rgba");
    break;
  default:
    FAIL("Invalid property texture property view type");
  }

  REQUIRE(!view.normalized());

  std::vector<glm::dvec2> texCoords{
      glm::dvec2(0, 0),
      glm::dvec2(0.5, 0),
      glm::dvec2(0, 0.5),
      glm::dvec2(0.5, 0.5)};

  for (size_t i = 0; i < texCoords.size(); i++) {
    glm::dvec2 uv = texCoords[i];
    REQUIRE(view.getRaw(uv[0], uv[1]) == expectedRaw[i]);
    REQUIRE(view.get(uv[0], uv[1]) == expectedTransformed[i]);
  }
}

template <typename T, typename D = typename TypeToNormalizedType<T>::type>
void checkNormalizedTextureValues(
    const std::vector<uint8_t>& data,
    const std::vector<T>& expectedRaw,
    const std::vector<std::optional<D>>& expectedTransformed,
    const std::optional<JsonValue> offset = std::nullopt,
    const std::optional<JsonValue> scale = std::nullopt,
    const std::optional<JsonValue> noData = std::nullopt,
    const std::optional<JsonValue> defaultValue = std::nullopt) {
  PropertyTextureProperty property;
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

  Sampler sampler;
  ImageAsset image;
  image.width = 2;
  image.height = 2;
  image.channels = static_cast<int32_t>(sizeof(T));
  image.bytesPerChannel = 1;

  std::vector<std::byte>& imageData = image.pixelData;
  imageData.resize(data.size());
  std::memcpy(imageData.data(), data.data(), data.size());

  property.channels.resize(static_cast<size_t>(image.channels));
  for (size_t i = 0; i < property.channels.size(); i++) {
    property.channels[i] = static_cast<int64_t>(i);
  }

  PropertyTexturePropertyView<T, true> view(
      property,
      classProperty,
      sampler,
      image);
  switch (sizeof(T)) {
  case 1:
    CHECK(view.getSwizzle() == "r");
    break;
  case 2:
    CHECK(view.getSwizzle() == "rg");
    break;
  case 3:
    CHECK(view.getSwizzle() == "rgb");
    break;
  case 4:
    CHECK(view.getSwizzle() == "rgba");
    break;
  default:
    FAIL("Invalid property texture property view type");
  }

  REQUIRE(view.normalized());

  std::vector<glm::dvec2> texCoords{
      glm::dvec2(0, 0),
      glm::dvec2(0.5, 0),
      glm::dvec2(0, 0.5),
      glm::dvec2(0.5, 0.5)};

  for (size_t i = 0; i < texCoords.size(); i++) {
    glm::dvec2 uv = texCoords[i];
    REQUIRE(view.getRaw(uv[0], uv[1]) == expectedRaw[i]);
    REQUIRE(view.get(uv[0], uv[1]) == expectedTransformed[i]);
  }
}

template <typename T>
void checkTextureArrayValues(
    const std::vector<uint8_t>& data,
    const int64_t count,
    const std::vector<std::vector<T>>& expected) {
  PropertyTextureProperty property;
  ClassProperty classProperty;
  classProperty.type =
      convertPropertyTypeToString(TypeToPropertyType<T>::value);

  PropertyComponentType componentType = TypeToPropertyType<T>::component;
  classProperty.componentType =
      convertPropertyComponentTypeToString(componentType);

  classProperty.array = true;
  classProperty.count = count;

  Sampler sampler;
  ImageAsset image;
  image.width = 2;
  image.height = 2;
  image.channels =
      static_cast<int32_t>(count) * static_cast<int32_t>(sizeof(T));
  image.bytesPerChannel = 1;

  std::vector<std::byte>& imageData = image.pixelData;
  imageData.resize(data.size());
  std::memcpy(imageData.data(), data.data(), data.size());

  property.channels.resize(static_cast<size_t>(image.channels));
  for (size_t i = 0; i < property.channels.size(); i++) {
    property.channels[i] = static_cast<int64_t>(i);
  }

  PropertyTexturePropertyView<PropertyArrayView<T>> view(
      property,
      classProperty,
      sampler,
      image);
  switch (image.channels) {
  case 1:
    CHECK(view.getSwizzle() == "r");
    break;
  case 2:
    CHECK(view.getSwizzle() == "rg");
    break;
  case 3:
    CHECK(view.getSwizzle() == "rgb");
    break;
  case 4:
    CHECK(view.getSwizzle() == "rgba");
    break;
  default:
    FAIL("Unsupported count value");
  }

  REQUIRE(!view.normalized());

  std::vector<glm::dvec2> texCoords{
      glm::dvec2(0, 0),
      glm::dvec2(0.5, 0),
      glm::dvec2(0, 0.5),
      glm::dvec2(0.5, 0.5)};

  for (size_t i = 0; i < texCoords.size(); i++) {
    const std::vector<T>& expectedValue = expected[i];
    glm::dvec2 uv = texCoords[i];

    auto value = view.getRaw(uv[0], uv[1]);
    REQUIRE(static_cast<size_t>(value.size()) == expectedValue.size());

    auto maybeValue = view.get(uv[0], uv[1]);
    REQUIRE(maybeValue);
    REQUIRE(maybeValue->size() == value.size());
    for (int64_t j = 0; j < value.size(); j++) {
      REQUIRE(value[j] == expectedValue[static_cast<size_t>(j)]);
      REQUIRE((*maybeValue)[j] == value[j]);
    }
  }
}

template <typename T>
void checkTextureArrayValues(
    const std::vector<uint8_t>& data,
    const int64_t count,
    const std::vector<std::vector<T>>& expectedRaw,
    const std::vector<std::optional<std::vector<T>>>& expectedTransformed,
    const std::optional<JsonValue> noData = std::nullopt,
    const std::optional<JsonValue> defaultValue = std::nullopt) {
  PropertyTextureProperty property;
  ClassProperty classProperty;
  classProperty.type =
      convertPropertyTypeToString(TypeToPropertyType<T>::value);

  PropertyComponentType componentType = TypeToPropertyType<T>::component;
  classProperty.componentType =
      convertPropertyComponentTypeToString(componentType);

  classProperty.array = true;
  classProperty.count = count;
  classProperty.noData = noData;
  classProperty.defaultProperty = defaultValue;

  Sampler sampler;
  ImageAsset image;
  image.width = 2;
  image.height = 2;
  image.channels =
      static_cast<int32_t>(count) * static_cast<int32_t>(sizeof(T));
  image.bytesPerChannel = 1;

  std::vector<std::byte>& imageData = image.pixelData;
  imageData.resize(data.size());
  std::memcpy(imageData.data(), data.data(), data.size());

  property.channels.resize(static_cast<size_t>(image.channels));
  for (size_t i = 0; i < property.channels.size(); i++) {
    property.channels[i] = static_cast<int64_t>(i);
  }

  PropertyTexturePropertyView<PropertyArrayView<T>> view(
      property,
      classProperty,
      sampler,
      image);
  switch (count) {
  case 1:
    CHECK(view.getSwizzle() == "r");
    break;
  case 2:
    CHECK(view.getSwizzle() == "rg");
    break;
  case 3:
    CHECK(view.getSwizzle() == "rgb");
    break;
  case 4:
    CHECK(view.getSwizzle() == "rgba");
    break;
  default:
    FAIL("Unsupported count value");
  }

  REQUIRE(!view.normalized());

  std::vector<glm::dvec2> texCoords{
      glm::dvec2(0, 0),
      glm::dvec2(0.5, 0),
      glm::dvec2(0, 0.5),
      glm::dvec2(0.5, 0.5)};

  for (size_t i = 0; i < texCoords.size(); i++) {
    auto expectedRawValue = expectedRaw[i];
    glm::dvec2 uv = texCoords[i];

    // Check raw values first
    auto rawValue = view.getRaw(uv[0], uv[1]);
    REQUIRE(static_cast<size_t>(rawValue.size()) == expectedRawValue.size());
    for (int64_t j = 0; j < rawValue.size(); j++) {
      REQUIRE(rawValue[j] == expectedRawValue[static_cast<size_t>(j)]);
    }

    // Check transformed values
    auto maybeValue = view.get(uv[0], uv[1]);
    if (!maybeValue) {
      REQUIRE(!expectedTransformed[i]);
      continue;
    }

    auto expectedValue = *(expectedTransformed[i]);
    REQUIRE(maybeValue->size() == static_cast<int64_t>(expectedValue.size()));
    for (int64_t j = 0; j < maybeValue->size(); j++) {
      REQUIRE((*maybeValue)[j] == expectedValue[static_cast<size_t>(j)]);
    }
  }
}

template <typename T, typename D = typename TypeToNormalizedType<T>::type>
void checkNormalizedTextureArrayValues(
    const std::vector<uint8_t>& data,
    const int64_t count,
    const std::vector<std::vector<T>>& expectedRaw,
    const std::vector<std::optional<std::vector<D>>>& expectedTransformed,
    const std::optional<JsonValue> offset = std::nullopt,
    const std::optional<JsonValue> scale = std::nullopt,
    const std::optional<JsonValue> noData = std::nullopt,
    const std::optional<JsonValue> defaultValue = std::nullopt) {
  PropertyTextureProperty property;
  ClassProperty classProperty;
  classProperty.type =
      convertPropertyTypeToString(TypeToPropertyType<T>::value);

  PropertyComponentType componentType = TypeToPropertyType<T>::component;
  classProperty.componentType =
      convertPropertyComponentTypeToString(componentType);

  classProperty.array = true;
  classProperty.count = count;
  classProperty.normalized = true;
  classProperty.offset = offset;
  classProperty.scale = scale;
  classProperty.noData = noData;
  classProperty.defaultProperty = defaultValue;

  Sampler sampler;
  ImageAsset image;
  image.width = 2;
  image.height = 2;
  image.channels =
      static_cast<int32_t>(count) * static_cast<int32_t>(sizeof(T));
  image.bytesPerChannel = 1;

  std::vector<std::byte>& imageData = image.pixelData;
  imageData.resize(data.size());
  std::memcpy(imageData.data(), data.data(), data.size());

  property.channels.resize(static_cast<size_t>(image.channels));
  for (size_t i = 0; i < property.channels.size(); i++) {
    property.channels[i] = static_cast<int64_t>(i);
  }

  PropertyTexturePropertyView<PropertyArrayView<T>, true> view(
      property,
      classProperty,
      sampler,
      image);
  switch (image.channels) {
  case 1:
    CHECK(view.getSwizzle() == "r");
    break;
  case 2:
    CHECK(view.getSwizzle() == "rg");
    break;
  case 3:
    CHECK(view.getSwizzle() == "rgb");
    break;
  case 4:
    CHECK(view.getSwizzle() == "rgba");
    break;
  default:
    FAIL("Unsupported count value");
  }

  REQUIRE(view.normalized());

  std::vector<glm::dvec2> texCoords{
      glm::dvec2(0, 0),
      glm::dvec2(0.5, 0),
      glm::dvec2(0, 0.5),
      glm::dvec2(0.5, 0.5)};

  for (size_t i = 0; i < texCoords.size(); i++) {
    auto expectedRawValue = expectedRaw[i];
    glm::dvec2 uv = texCoords[i];

    // Check raw values first
    auto rawValue = view.getRaw(uv[0], uv[1]);
    REQUIRE(static_cast<size_t>(rawValue.size()) == expectedRawValue.size());
    for (int64_t j = 0; j < rawValue.size(); j++) {
      REQUIRE(rawValue[j] == expectedRawValue[static_cast<size_t>(j)]);
    }

    // Check transformed values
    auto maybeValue = view.get(uv[0], uv[1]);
    if (!maybeValue) {
      REQUIRE(!expectedTransformed[i]);
      continue;
    }

    auto expectedValue = *(expectedTransformed[i]);
    REQUIRE(maybeValue->size() == static_cast<int64_t>(expectedValue.size()));
    for (int64_t j = 0; j < maybeValue->size(); j++) {
      REQUIRE((*maybeValue)[j] == expectedValue[static_cast<size_t>(j)]);
    }
  }
}
} // namespace

TEST_CASE("Check scalar PropertyTexturePropertyView") {
  SUBCASE("uint8_t") {
    std::vector<uint8_t> data{12, 33, 56, 67};
    checkTextureValues(data, data);
  }

  SUBCASE("int8_t") {
    std::vector<uint8_t> data{255, 0, 223, 67};
    std::vector<int8_t> expected{-1, 0, -33, 67};
    checkTextureValues(data, expected);
  }

  SUBCASE("uint16_t") {
    // clang-format off
    std::vector<uint8_t> data{
      28, 0,
      1, 1,
      0, 3,
      182, 1};
    // clang-format on
    std::vector<uint16_t> expected{28, 257, 768, 438};
    checkTextureValues(data, expected);
  }

  SUBCASE("int16_t") {
    // clang-format off
    std::vector<uint8_t> data{
      255, 255,
      1, 129,
      0, 3,
      182, 1};
    // clang-format on
    std::vector<int16_t> expected{-1, -32511, 768, 438};
    checkTextureValues(data, expected);
  }

  SUBCASE("uint32_t") {
    // clang-format off
    std::vector<uint8_t> data{
      0, 0, 0, 1,
      9, 0, 1, 0,
      20, 2, 2, 0,
      8, 1, 0, 1};
    // clang-format on
    std::vector<uint32_t> expected{16777216, 65545, 131604, 16777480};
    checkTextureValues(data, expected);
  }

  SUBCASE("int32_t") {
    // clang-format off
    std::vector<uint8_t> data{
      255, 255, 255, 255,
      9, 0, 1, 0,
      20, 2, 2, 255,
      8, 1, 0, 1};
    // clang-format on
    std::vector<int32_t> expected{-1, 65545, -16645612, 16777480};
    checkTextureValues(data, expected);
  }

  SUBCASE("float") {
    // clang-format off
    std::vector<uint8_t> data{
      0, 0, 0, 1,
      9, 0, 1, 0,
      20, 2, 2, 0,
      8, 1, 0, 1};
    // clang-format on

    std::vector<uint32_t> expectedUint{16777216, 65545, 131604, 16777480};
    std::vector<float> expected(expectedUint.size());
    for (size_t i = 0; i < expectedUint.size(); i++) {
      float value = *reinterpret_cast<float*>(&expectedUint[i]);
      expected[i] = value;
    }

    checkTextureValues(data, expected);
  }

  SUBCASE("float with offset / scale") {
    // clang-format off
    std::vector<uint8_t> data{
      0, 0, 0, 1,
      9, 0, 1, 0,
      20, 2, 2, 0,
      8, 1, 0, 1};
    // clang-format on

    std::vector<uint32_t> expectedUint{16777216, 65545, 131604, 16777480};
    std::vector<float> expectedRaw(expectedUint.size());
    std::vector<std::optional<float>> expectedTransformed(expectedUint.size());

    const float offset = 1.0f;
    const float scale = 2.0f;

    for (size_t i = 0; i < expectedUint.size(); i++) {
      float value = *reinterpret_cast<float*>(&expectedUint[i]);
      expectedRaw[i] = value;
      expectedTransformed[i] = value * scale + offset;
    }

    checkTextureValues(data, expectedRaw, expectedTransformed, offset, scale);
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
    checkTextureValues(
        data,
        data,
        expected,
        std::nullopt,
        std::nullopt,
        noData);
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
    checkTextureValues(
        data,
        data,
        expected,
        std::nullopt,
        std::nullopt,
        noData,
        defaultValue);
  }
}

TEST_CASE("Check scalar PropertyTexturePropertyView (normalized)") {
  SUBCASE("uint8_t") {
    std::vector<uint8_t> data{12, 33, 56, 67};
    std::vector<std::optional<double>> expected{
        12.0 / 255.0,
        33 / 255.0,
        56 / 255.0,
        67 / 255.0};
    checkNormalizedTextureValues(data, data, expected);
  }

  SUBCASE("int16_t") {
    // clang-format off
    std::vector<uint8_t> data{
      255, 255,
      1, 129,
      0, 3,
      182, 1};
    // clang-format on
    std::vector<int16_t> expectedRaw{-1, -32511, 768, 438};
    std::vector<std::optional<double>> expectedTransformed{
        normalize(expectedRaw[0]),
        normalize(expectedRaw[1]),
        normalize(expectedRaw[2]),
        normalize(expectedRaw[3])};
    checkNormalizedTextureValues(data, expectedRaw, expectedTransformed);
  }

  SUBCASE("uint32_t") {
    // clang-format off
    std::vector<uint8_t> data{
      0, 0, 0, 1,
      9, 0, 1, 0,
      20, 2, 2, 0,
      8, 1, 0, 1};
    // clang-format on
    std::vector<uint32_t> expectedRaw{16777216, 65545, 131604, 16777480};
    std::vector<std::optional<double>> expectedTransformed{
        normalize(expectedRaw[0]),
        normalize(expectedRaw[1]),
        normalize(expectedRaw[2]),
        normalize(expectedRaw[3])};
    checkNormalizedTextureValues(data, expectedRaw, expectedTransformed);
  }

  SUBCASE("uint8_t with offset / scale") {
    std::vector<uint8_t> data{12, 33, 56, 67};
    const double offset = 1.0;
    const double scale = 2.0;
    std::vector<std::optional<double>> expected{
        normalize(data[0]) * scale + offset,
        normalize(data[1]) * scale + offset,
        normalize(data[2]) * scale + offset,
        normalize(data[3]) * scale + offset,
    };
    checkNormalizedTextureValues(data, data, expected, offset, scale);
  }

  SUBCASE("uint8_t with all properties") {
    std::vector<uint8_t> data{12, 33, 56, 0};
    const double offset = 1.0;
    const double scale = 2.0;
    const uint8_t noData = 0;
    const double defaultValue = 10.0;
    std::vector<std::optional<double>> expected{
        normalize(data[0]) * scale + offset,
        normalize(data[1]) * scale + offset,
        normalize(data[2]) * scale + offset,
        10.0,
    };
    checkNormalizedTextureValues(
        data,
        data,
        expected,
        offset,
        scale,
        noData,
        defaultValue);
  }
}

TEST_CASE("Check vecN PropertyTexturePropertyView") {
  SUBCASE("glm::u8vec2") {
    // clang-format off
    std::vector<uint8_t> data{
      28, 0,
      1, 1,
      0, 3,
      182, 1};
    // clang-format on
    std::vector<glm::u8vec2> expected{
        glm::u8vec2(28, 0),
        glm::u8vec2(1, 1),
        glm::u8vec2(0, 3),
        glm::u8vec2(182, 1)};
    checkTextureValues(data, expected);
  }

  SUBCASE("glm::i8vec2") {
    // clang-format off
    std::vector<uint8_t> data{
      28, 255,
      254, 1,
      0, 3,
      182, 1};
    // clang-format on
    std::vector<glm::i8vec2> expected{
        glm::i8vec2(28, -1),
        glm::i8vec2(-2, 1),
        glm::i8vec2(0, 3),
        glm::i8vec2(-74, 1)};
    checkTextureValues(data, expected);
  }

  SUBCASE("glm::u8vec3") {
    // clang-format off
    std::vector<uint8_t> data{
      1, 2, 3,
      4, 5, 6,
      7, 8, 9,
      0, 5, 2};
    // clang-format on
    std::vector<glm::u8vec3> expected{
        glm::u8vec3(1, 2, 3),
        glm::u8vec3(4, 5, 6),
        glm::u8vec3(7, 8, 9),
        glm::u8vec3(0, 5, 2)};
    checkTextureValues(data, expected);
  }

  SUBCASE("glm::i8vec3") {
    // clang-format off
    std::vector<uint8_t> data{
      255, 2, 3,
      4, 254, 6,
      7, 8, 159,
      0, 5, 2};
    // clang-format on
    std::vector<glm::i8vec3> expected{
        glm::i8vec3(-1, 2, 3),
        glm::i8vec3(4, -2, 6),
        glm::i8vec3(7, 8, -97),
        glm::i8vec3(0, 5, 2)};
    checkTextureValues(data, expected);
  }

  SUBCASE("glm::u8vec4") {
    // clang-format off
    std::vector<uint8_t> data{
      1, 2, 3, 0,
      4, 5, 6, 11,
      7, 8, 9, 3,
      0, 5, 2, 27};
    // clang-format on
    std::vector<glm::u8vec4> expected{
        glm::u8vec4(1, 2, 3, 0),
        glm::u8vec4(4, 5, 6, 11),
        glm::u8vec4(7, 8, 9, 3),
        glm::u8vec4(0, 5, 2, 27)};
    checkTextureValues(data, expected);
  }

  SUBCASE("glm::i8vec4") {
    // clang-format off
    std::vector<uint8_t> data{
      1, 200, 3, 0,
      4, 5, 6, 251,
      129, 8, 9, 3,
      0, 155, 2, 27};
    // clang-format on
    std::vector<glm::i8vec4> expected{
        glm::i8vec4(1, -56, 3, 0),
        glm::i8vec4(4, 5, 6, -5),
        glm::i8vec4(-127, 8, 9, 3),
        glm::i8vec4(0, -101, 2, 27)};
    checkTextureValues(data, expected);
  }

  SUBCASE("glm::u16vec2") {
    // clang-format off
    std::vector<uint8_t> data{
      0, 0, 0, 1,
      9, 0, 1, 0,
      20, 2, 2, 0,
      8, 1, 0, 1};
    // clang-format on
    std::vector<glm::u16vec2> expected{
        glm::u16vec2(0, 256),
        glm::u16vec2(9, 1),
        glm::u16vec2(532, 2),
        glm::u16vec2(264, 256)};
    checkTextureValues(data, expected);
  }

  SUBCASE("glm::i16vec2") {
    // clang-format off
    std::vector<uint8_t> data{
      255, 255, 0, 1,
      9, 0, 146, 195,
      20, 2, 2, 0,
      8, 1, 255, 1};
    // clang-format on
    std::vector<glm::i16vec2> expected{
        glm::i16vec2(-1, 256),
        glm::i16vec2(9, -15470),
        glm::i16vec2(532, 2),
        glm::i16vec2(264, 511)};
    checkTextureValues(data, expected);
  }

  SUBCASE("glm::i8vec2 with noData") {
    // clang-format off
    std::vector<uint8_t> data{
      28, 255,
      254, 1,
      0, 3,
      0, 0,
      182, 1};
    // clang-format on
    JsonValue::Array noData{0, 0};
    std::vector<glm::i8vec2> expectedRaw{
        glm::i8vec2(28, -1),
        glm::i8vec2(-2, 1),
        glm::i8vec2(0, 3),
        glm::i8vec2(0),
        glm::i8vec2(-74, 1)};
    std::vector<std::optional<glm::i8vec2>> expectedTransformed{
        glm::i8vec2(28, -1),
        glm::i8vec2(-2, 1),
        glm::i8vec2(0, 3),
        std::nullopt,
        glm::i8vec2(-74, 1)};
    checkTextureValues(
        data,
        expectedRaw,
        expectedTransformed,
        std::nullopt,
        std::nullopt,
        noData);
  }

  SUBCASE("glm::i8vec2 with defaultValue") {
    // clang-format off
    std::vector<uint8_t> data{
      28, 255,
      254, 1,
      0, 3,
      0, 0,
      182, 1};
    // clang-format on
    JsonValue::Array noData{0, 0};
    JsonValue::Array defaultValue{127, 127};
    std::vector<glm::i8vec2> expectedRaw{
        glm::i8vec2(28, -1),
        glm::i8vec2(-2, 1),
        glm::i8vec2(0, 3),
        glm::i8vec2(0, 0),
        glm::i8vec2(-74, 1)};
    std::vector<std::optional<glm::i8vec2>> expectedTransformed{
        glm::i8vec2(28, -1),
        glm::i8vec2(-2, 1),
        glm::i8vec2(0, 3),
        glm::i8vec2(127, 127),
        glm::i8vec2(-74, 1)};
    checkTextureValues(
        data,
        expectedRaw,
        expectedTransformed,
        std::nullopt,
        std::nullopt,
        noData,
        defaultValue);
  }
}

TEST_CASE("Check vecN PropertyTexturePropertyView (normalized)") {
  SUBCASE("glm::i8vec2") {
    // clang-format off
    std::vector<uint8_t> data{
      28, 255,
      254, 1,
      0, 3,
      182, 1};
    // clang-format on
    std::vector<glm::i8vec2> expectedRaw{
        glm::i8vec2(28, -1),
        glm::i8vec2(-2, 1),
        glm::i8vec2(0, 3),
        glm::i8vec2(-74, 1)};
    std::vector<std::optional<glm::dvec2>> expectedTransformed{
        normalize(expectedRaw[0]),
        normalize(expectedRaw[1]),
        normalize(expectedRaw[2]),
        normalize(expectedRaw[3])};
    checkNormalizedTextureValues(data, expectedRaw, expectedTransformed);
  }

  SUBCASE("glm::u8vec3") {
    // clang-format off
    std::vector<uint8_t> data{
      1, 2, 3,
      4, 5, 6,
      7, 8, 9,
      0, 5, 2};
    // clang-format on
    std::vector<glm::u8vec3> expectedRaw{
        glm::u8vec3(1, 2, 3),
        glm::u8vec3(4, 5, 6),
        glm::u8vec3(7, 8, 9),
        glm::u8vec3(0, 5, 2)};
    std::vector<std::optional<glm::dvec3>> expectedTransformed{
        normalize(expectedRaw[0]),
        normalize(expectedRaw[1]),
        normalize(expectedRaw[2]),
        normalize(expectedRaw[3])};
    checkNormalizedTextureValues(data, expectedRaw, expectedTransformed);
  }

  SUBCASE("glm::u8vec4") {
    // clang-format off
    std::vector<uint8_t> data{
      1, 2, 3, 0,
      4, 5, 6, 11,
      7, 8, 9, 3,
      0, 5, 2, 27};
    // clang-format on
    std::vector<glm::u8vec4> expectedRaw{
        glm::u8vec4(1, 2, 3, 0),
        glm::u8vec4(4, 5, 6, 11),
        glm::u8vec4(7, 8, 9, 3),
        glm::u8vec4(0, 5, 2, 27)};
    std::vector<std::optional<glm::dvec4>> expectedTransformed{
        normalize(expectedRaw[0]),
        normalize(expectedRaw[1]),
        normalize(expectedRaw[2]),
        normalize(expectedRaw[3])};
    checkNormalizedTextureValues(data, expectedRaw, expectedTransformed);
  }

  SUBCASE("glm::i16vec2") {
    // clang-format off
    std::vector<uint8_t> data{
      255, 255, 0, 1,
      9, 0, 146, 195,
      20, 2, 2, 0,
      8, 1, 255, 1};
    // clang-format on
    std::vector<glm::i16vec2> expectedRaw{
        glm::i16vec2(-1, 256),
        glm::i16vec2(9, -15470),
        glm::i16vec2(532, 2),
        glm::i16vec2(264, 511)};
    std::vector<std::optional<glm::dvec2>> expectedTransformed{
        normalize(expectedRaw[0]),
        normalize(expectedRaw[1]),
        normalize(expectedRaw[2]),
        normalize(expectedRaw[3])};
    checkNormalizedTextureValues(data, expectedRaw, expectedTransformed);
  }

  SUBCASE("glm::i8vec2 with offset / scale") {
    // clang-format off
    std::vector<uint8_t> data{
      28, 255,
      254, 1,
      0, 3,
      0, 0,
      182, 1};
    // clang-format on
    const glm::dvec2 offset(-1.0, 4.0);
    const glm::dvec2 scale(2.0, 1.0);

    std::vector<glm::i8vec2> expectedRaw{
        glm::i8vec2(28, -1),
        glm::i8vec2(-2, 1),
        glm::i8vec2(0, 3),
        glm::i8vec2(0),
        glm::i8vec2(-74, 1)};
    std::vector<std::optional<glm::dvec2>> expectedTransformed{
        normalize(expectedRaw[0]) * scale + offset,
        normalize(expectedRaw[1]) * scale + offset,
        normalize(expectedRaw[2]) * scale + offset,
        normalize(expectedRaw[3]) * scale + offset,
        normalize(expectedRaw[4]) * scale + offset,
    };
    checkNormalizedTextureValues(
        data,
        expectedRaw,
        expectedTransformed,
        JsonValue::Array{offset[0], offset[1]},
        JsonValue::Array{scale[0], scale[1]});
  }

  SUBCASE("glm::i8vec2 with all properties") {
    // clang-format off
    std::vector<uint8_t> data{
      28, 255,
      254, 1,
      0, 3,
      0, 0,
      182, 1};
    // clang-format on
    const glm::dvec2 offset(-1.0, 4.0);
    const glm::dvec2 scale(2.0, 1.0);
    const glm::i8vec2 noData(0);
    const glm::dvec2 defaultValue(100.0, 5.5);

    std::vector<glm::i8vec2> expectedRaw{
        glm::i8vec2(28, -1),
        glm::i8vec2(-2, 1),
        glm::i8vec2(0, 3),
        glm::i8vec2(0),
        glm::i8vec2(-74, 1)};
    std::vector<std::optional<glm::dvec2>> expectedTransformed{
        normalize(expectedRaw[0]) * scale + offset,
        normalize(expectedRaw[1]) * scale + offset,
        normalize(expectedRaw[2]) * scale + offset,
        defaultValue,
        normalize(expectedRaw[4]) * scale + offset,
    };
    checkNormalizedTextureValues(
        data,
        expectedRaw,
        expectedTransformed,
        JsonValue::Array{offset[0], offset[1]},
        JsonValue::Array{scale[0], scale[1]},
        JsonValue::Array{noData[0], noData[1]},
        JsonValue::Array{defaultValue[0], defaultValue[1]});
  }
}

TEST_CASE("Check array PropertyTexturePropertyView") {
  SUBCASE("uint8_t array") {
    // clang-format off
    std::vector<uint8_t> data{
      1, 2, 3, 0,
      4, 5, 6, 11,
      7, 8, 9, 3,
      0, 5, 2, 27};
    // clang-format on

    std::vector<std::vector<uint8_t>> expected{
        {1, 2, 3, 0},
        {4, 5, 6, 11},
        {7, 8, 9, 3},
        {0, 5, 2, 27}};
    checkTextureArrayValues(data, 4, expected);
  }

  SUBCASE("int8_t array") {
    // clang-format off
    std::vector<uint8_t> data{
      1, 200, 3, 0,
      4, 5, 6, 251,
      129, 8, 9, 3,
      0, 155, 2, 27};
    // clang-format on
    std::vector<std::vector<int8_t>> expected{
        {1, -56, 3, 0},
        {4, 5, 6, -5},
        {-127, 8, 9, 3},
        {0, -101, 2, 27}};
    checkTextureArrayValues(data, 4, expected);
  }

  SUBCASE("uint16_t array") {
    // clang-format off
    std::vector<uint8_t> data{
      0, 0, 0, 1,
      9, 0, 1, 0,
      20, 2, 2, 0,
      8, 1, 0, 1};
    // clang-format on
    std::vector<std::vector<uint16_t>> expected{
        {0, 256},
        {9, 1},
        {532, 2},
        {264, 256}};
    checkTextureArrayValues(data, 2, expected);
  }

  SUBCASE("int16_t array") {
    // clang-format off
    std::vector<uint8_t> data{
      255, 255, 0, 1,
      9, 0, 146, 195,
      20, 2, 2, 0,
      8, 255, 0, 1};
    // clang-format on
    std::vector<std::vector<int16_t>> expected{
        {-1, 256},
        {9, -15470},
        {532, 2},
        {-248, 256}};
    checkTextureArrayValues(data, 2, expected);
  }

  SUBCASE("uint8_t array with noData") {
    // clang-format off
    std::vector<uint8_t> data{
      1, 2, 3, 0,
      4, 5, 6, 11,
      7, 8, 9, 3,
      0, 0, 0, 0,};
    // clang-format on

    JsonValue::Array noData{0, 0, 0, 0};
    std::vector<std::vector<uint8_t>> expectedRaw{
        {1, 2, 3, 0},
        {4, 5, 6, 11},
        {7, 8, 9, 3},
        {0, 0, 0, 0}};
    std::vector<std::optional<std::vector<uint8_t>>> expectedTransformed{
        std::vector<uint8_t>{1, 2, 3, 0},
        std::vector<uint8_t>{4, 5, 6, 11},
        std::vector<uint8_t>{7, 8, 9, 3},
        std::nullopt};
    checkTextureArrayValues(data, 4, expectedRaw, expectedTransformed, noData);
  }

  SUBCASE("uint8_t array with noData and defaultValue") {
    // clang-format off
    std::vector<uint8_t> data{
      1, 2, 3, 0,
      4, 5, 6, 11,
      7, 8, 9, 3,
      0, 0, 0, 0,};
    // clang-format on

    JsonValue::Array noData{0, 0, 0, 0};
    JsonValue::Array defaultValue{255, 8, 12, 5};
    std::vector<std::vector<uint8_t>> expectedRaw{
        {1, 2, 3, 0},
        {4, 5, 6, 11},
        {7, 8, 9, 3},
        {0, 0, 0, 0}};
    std::vector<std::optional<std::vector<uint8_t>>> expectedTransformed{
        std::vector<uint8_t>{1, 2, 3, 0},
        std::vector<uint8_t>{4, 5, 6, 11},
        std::vector<uint8_t>{7, 8, 9, 3},
        std::vector<uint8_t>{255, 8, 12, 5}};
    checkTextureArrayValues(
        data,
        4,
        expectedRaw,
        expectedTransformed,
        noData,
        defaultValue);
  }
}

TEST_CASE("Check array PropertyTexturePropertyView (normalized)") {
  SUBCASE("uint8_t array") {
    // clang-format off
    std::vector<uint8_t> data{
      1, 2, 3, 0,
      4, 5, 6, 11,
      7, 8, 9, 3,
      0, 5, 2, 27};
    // clang-format on
    std::vector<std::vector<uint8_t>> expectedRaw{
        {1, 2, 3, 0},
        {4, 5, 6, 11},
        {7, 8, 9, 3},
        {0, 5, 2, 27}};
    std::vector<std::optional<std::vector<double>>> expectedTransformed(
        expectedRaw.size());

    for (size_t i = 0; i < expectedRaw.size(); i++) {
      auto rawValues = expectedRaw[i];
      std::vector<double> transformedValues(rawValues.size());
      for (size_t j = 0; j < rawValues.size(); j++) {
        transformedValues[j] = normalize(rawValues[j]);
      }
      expectedTransformed[i] = transformedValues;
    }
    checkNormalizedTextureArrayValues(
        data,
        4,
        expectedRaw,
        expectedTransformed);
  }

  SUBCASE("int16_t array") {
    // clang-format off
    std::vector<uint8_t> data{
      255, 255, 0, 1,
      9, 0, 146, 195,
      20, 2, 2, 0,
      8, 255, 0, 1};
    // clang-format on
    std::vector<std::vector<int16_t>> expectedRaw{
        {-1, 256},
        {9, -15470},
        {532, 2},
        {-248, 256}};
    std::vector<std::optional<std::vector<double>>> expectedTransformed(
        expectedRaw.size());

    for (size_t i = 0; i < expectedRaw.size(); i++) {
      auto rawValues = expectedRaw[i];
      std::vector<double> transformedValues(rawValues.size());
      for (size_t j = 0; j < rawValues.size(); j++) {
        transformedValues[j] = normalize(rawValues[j]);
      }
      expectedTransformed[i] = transformedValues;
    }

    checkNormalizedTextureArrayValues(
        data,
        2,
        expectedRaw,
        expectedTransformed);
  }

  SUBCASE("uint8_t array with offset / scale") {
    // clang-format off
    std::vector<uint8_t> data{
      1, 2, 3, 0,
      4, 5, 6, 11,
      7, 8, 9, 3,
      0, 0, 0, 0,};
    // clang-format on

    const std::vector<double> offset{1, 2, 0, 4};
    const std::vector<double> scale{1, -1, 3, -2};
    std::vector<std::vector<uint8_t>> expectedRaw{
        {1, 2, 3, 0},
        {4, 5, 6, 11},
        {7, 8, 9, 3},
        {0, 0, 0, 0}};
    std::vector<std::optional<std::vector<double>>> expectedTransformed(
        expectedRaw.size());

    for (size_t i = 0; i < expectedRaw.size(); i++) {
      auto rawValues = expectedRaw[i];
      std::vector<double> transformedValues(rawValues.size());
      for (size_t j = 0; j < rawValues.size(); j++) {
        transformedValues[j] = normalize(rawValues[j]) * scale[j] + offset[j];
      }
      expectedTransformed[i] = transformedValues;
    }

    checkNormalizedTextureArrayValues(
        data,
        4,
        expectedRaw,
        expectedTransformed,
        JsonValue::Array{offset[0], offset[1], offset[2], offset[3]},
        JsonValue::Array{scale[0], scale[1], scale[2], scale[3]});
  }

  SUBCASE("uint8_t array with noData") {
    // clang-format off
    std::vector<uint8_t> data{
      1, 2, 3, 0,
      4, 5, 6, 11,
      7, 8, 9, 3,
      0, 0, 0, 0,};
    // clang-format on

    JsonValue::Array noData{0, 0, 0, 0};
    std::vector<std::vector<uint8_t>> expectedRaw{
        {1, 2, 3, 0},
        {4, 5, 6, 11},
        {7, 8, 9, 3},
        {0, 0, 0, 0}};
    std::vector<std::optional<std::vector<double>>> expectedTransformed(
        expectedRaw.size());

    for (size_t i = 0; i < expectedRaw.size() - 1; i++) {
      auto rawValues = expectedRaw[i];
      std::vector<double> transformedValues(rawValues.size());
      for (size_t j = 0; j < rawValues.size(); j++) {
        transformedValues[j] = normalize(rawValues[j]);
      }
      expectedTransformed[i] = transformedValues;
    }

    expectedTransformed[3] = std::nullopt;

    checkNormalizedTextureArrayValues(
        data,
        4,
        expectedRaw,
        expectedTransformed,
        std::nullopt,
        std::nullopt,
        noData);
  }

  SUBCASE("uint8_t array with all properties") {
    // clang-format off
    std::vector<uint8_t> data{
      1, 2, 3, 0,
      4, 5, 6, 11,
      7, 8, 9, 3,
      0, 0, 0, 0,};
    // clang-format on

    const std::vector<double> offset{1, 2, 0, 4};
    const std::vector<double> scale{1, -1, 3, -2};
    JsonValue::Array noData{0, 0, 0, 0};
    JsonValue::Array defaultValue{1.0, 2.0, 3.0, 4.0};

    std::vector<std::vector<uint8_t>> expectedRaw{
        {1, 2, 3, 0},
        {4, 5, 6, 11},
        {7, 8, 9, 3},
        {0, 0, 0, 0}};
    std::vector<std::optional<std::vector<double>>> expectedTransformed(
        expectedRaw.size());

    for (size_t i = 0; i < expectedRaw.size() - 1; i++) {
      auto rawValues = expectedRaw[i];
      std::vector<double> transformedValues(rawValues.size());
      for (size_t j = 0; j < rawValues.size(); j++) {
        transformedValues[j] = normalize(rawValues[j]) * scale[j] + offset[j];
      }
      expectedTransformed[i] = transformedValues;
    }

    expectedTransformed[3] = std::vector<double>{1.0, 2.0, 3.0, 4.0};

    checkNormalizedTextureArrayValues(
        data,
        4,
        expectedRaw,
        expectedTransformed,
        JsonValue::Array{offset[0], offset[1], offset[2], offset[3]},
        JsonValue::Array{scale[0], scale[1], scale[2], scale[3]},
        noData,
        defaultValue);
  }
}

TEST_CASE("Check that PropertyTextureProperty values override class property "
          "values") {
  ClassProperty classProperty;
  classProperty.type = ClassProperty::Type::SCALAR;
  classProperty.componentType = ClassProperty::ComponentType::FLOAT32;

  classProperty.offset = 0.0f;
  classProperty.scale = 1.0f;
  classProperty.min = -10.0f;
  classProperty.max = 10.0f;

  Sampler sampler;
  ImageAsset image;
  image.width = 2;
  image.height = 2;
  image.channels = 4;
  image.bytesPerChannel = 1;

  // clang-format off
    std::vector<uint8_t> data{
      0, 0, 0, 1,
      9, 0, 1, 0,
      20, 2, 2, 0,
      8, 1, 0, 1};
  // clang-format on

  const float offset = 1.0f;
  const float scale = 2.0f;
  std::vector<uint32_t> expectedUint{16777216, 65545, 131604, 16777480};
  std::vector<float> expectedRaw(expectedUint.size());
  std::vector<std::optional<float>> expectedTransformed(expectedUint.size());
  for (size_t i = 0; i < expectedUint.size(); i++) {
    float value = *reinterpret_cast<float*>(&expectedUint[i]);
    expectedRaw[i] = value;
    expectedTransformed[i] = value * scale + offset;
  }

  std::vector<std::byte>& imageData = image.pixelData;
  imageData.resize(data.size());
  std::memcpy(imageData.data(), data.data(), data.size());

  PropertyTextureProperty property;
  property.offset = offset;
  property.scale = scale;
  property.min = std::numeric_limits<float>::lowest();
  property.max = std::numeric_limits<float>::max();
  property.channels = {0, 1, 2, 3};

  PropertyTexturePropertyView<float> view(
      property,
      classProperty,
      sampler,
      image);
  CHECK(view.getSwizzle() == "rgba");

  REQUIRE(view.offset() == offset);
  REQUIRE(view.scale() == scale);
  REQUIRE(view.min() == std::numeric_limits<float>::lowest());
  REQUIRE(view.max() == std::numeric_limits<float>::max());

  std::vector<glm::dvec2> texCoords{
      glm::dvec2(0, 0),
      glm::dvec2(0.5, 0),
      glm::dvec2(0, 0.5),
      glm::dvec2(0.5, 0.5)};

  for (size_t i = 0; i < texCoords.size(); i++) {
    glm::dvec2 uv = texCoords[i];
    REQUIRE(view.getRaw(uv[0], uv[1]) == expectedRaw[i]);
    REQUIRE(view.get(uv[0], uv[1]) == expectedTransformed[i]);
  }
}

TEST_CASE("Check that non-adjacent channels resolve to expected output") {
  std::vector<glm::dvec2> texCoords{
      glm::dvec2(0, 0),
      glm::dvec2(0.5, 0),
      glm::dvec2(0, 0.5),
      glm::dvec2(0.5, 0.5)};

  SUBCASE("single-byte scalar") {
    PropertyTextureProperty property;
    property.channels = {3};

    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;

    Sampler sampler;
    ImageAsset image;
    image.width = 2;
    image.height = 2;
    image.channels = 4;
    image.bytesPerChannel = 1;

    // clang-format off
    std::vector<uint8_t> data{
      0, 1, 2, 3,
      1, 2, 3, 4,
      1, 0, 1, 0,
      2, 3, 8, 1};
    // clang-format on

    std::vector<std::byte>& imageData = image.pixelData;
    imageData.resize(data.size());
    std::memcpy(imageData.data(), data.data(), data.size());

    PropertyTexturePropertyView<uint8_t> view(
        property,
        classProperty,
        sampler,
        image);
    CHECK(view.getSwizzle() == "a");

    std::vector<uint8_t> expected{3, 4, 0, 1};
    for (size_t i = 0; i < expected.size(); i++) {
      glm::dvec2 uv = texCoords[i];
      REQUIRE(view.getRaw(uv[0], uv[1]) == expected[i]);
      REQUIRE(view.get(uv[0], uv[1]) == expected[i]);
    }
  }

  SUBCASE("multi-byte scalar") {
    PropertyTextureProperty property;
    property.channels = {2, 0};

    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT16;

    Sampler sampler;
    ImageAsset image;
    image.width = 2;
    image.height = 2;
    image.channels = 4;
    image.bytesPerChannel = 1;

    // clang-format off
    std::vector<uint8_t> data{
      0, 1, 2, 3,
      1, 2, 3, 4,
      1, 0, 1, 0,
      2, 3, 8, 1};
    // clang-format on
    std::vector<std::byte>& imageData = image.pixelData;
    imageData.resize(data.size());
    std::memcpy(imageData.data(), data.data(), data.size());

    PropertyTexturePropertyView<uint16_t> view(
        property,
        classProperty,
        sampler,
        image);
    CHECK(view.getSwizzle() == "br");

    std::vector<uint16_t> expected{2, 259, 257, 520};
    for (size_t i = 0; i < expected.size(); i++) {
      glm::dvec2 uv = texCoords[i];
      REQUIRE(view.getRaw(uv[0], uv[1]) == expected[i]);
      REQUIRE(view.get(uv[0], uv[1]) == expected[i]);
    }
  }

  SUBCASE("vecN") {
    PropertyTextureProperty property;
    property.channels = {3, 2, 1};

    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::VEC3;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;

    Sampler sampler;
    ImageAsset image;
    image.width = 2;
    image.height = 2;
    image.channels = 4;
    image.bytesPerChannel = 1;

    // clang-format off
    std::vector<uint8_t> data{
      0, 1, 2, 3,
      1, 2, 3, 4,
      1, 0, 1, 0,
      2, 3, 8, 1};
    // clang-format on
    std::vector<std::byte>& imageData = image.pixelData;
    imageData.resize(data.size());
    std::memcpy(imageData.data(), data.data(), data.size());

    PropertyTexturePropertyView<glm::u8vec3> view(
        property,
        classProperty,
        sampler,
        image);
    CHECK(view.getSwizzle() == "abg");

    std::vector<glm::u8vec3> expected{
        glm::u8vec3(3, 2, 1),
        glm::u8vec3(4, 3, 2),
        glm::u8vec3(0, 1, 0),
        glm::u8vec3(1, 8, 3)};
    for (size_t i = 0; i < expected.size(); i++) {
      glm::dvec2 uv = texCoords[i];
      REQUIRE(view.getRaw(uv[0], uv[1]) == expected[i]);
      REQUIRE(view.get(uv[0], uv[1]) == expected[i]);
    }
  }

  SUBCASE("array") {
    PropertyTextureProperty property;
    property.channels = {1, 0, 3, 2};

    ClassProperty classProperty;
    classProperty.type = ClassProperty::Type::SCALAR;
    classProperty.componentType = ClassProperty::ComponentType::UINT8;
    classProperty.array = true;

    Sampler sampler;
    ImageAsset image;
    image.width = 2;
    image.height = 2;
    image.channels = 4;
    image.bytesPerChannel = 1;

    // clang-format off
    std::vector<uint8_t> data{
      1, 2, 3, 0,
      4, 5, 6, 11,
      7, 8, 9, 3,
      0, 5, 2, 27};
    // clang-format on

    std::vector<std::byte>& imageData = image.pixelData;
    imageData.resize(data.size());
    std::memcpy(imageData.data(), data.data(), data.size());

    PropertyTexturePropertyView<PropertyArrayView<uint8_t>> view(
        property,
        classProperty,
        sampler,
        image);
    CHECK(view.getSwizzle() == "grab");

    std::vector<std::vector<uint8_t>> expected{
        {2, 1, 0, 3},
        {5, 4, 11, 6},
        {8, 7, 3, 9},
        {5, 0, 27, 2}};

    for (size_t i = 0; i < expected.size(); i++) {
      std::vector<uint8_t>& expectedValue = expected[i];
      glm::dvec2 uv = texCoords[i];

      auto value = view.getRaw(uv[0], uv[1]);
      REQUIRE(static_cast<size_t>(value.size()) == expectedValue.size());

      auto maybeValue = view.get(uv[0], uv[1]);
      REQUIRE(maybeValue);
      REQUIRE(maybeValue->size() == value.size());
      for (int64_t j = 0; j < value.size(); j++) {
        REQUIRE(value[j] == expectedValue[static_cast<size_t>(j)]);
        REQUIRE((*maybeValue)[j] == value[j]);
      }
    }
  }
}

TEST_CASE("Check sampling with different sampler values") {
  PropertyTextureProperty property;
  property.channels = {0};

  ClassProperty classProperty;
  classProperty.type = ClassProperty::Type::SCALAR;
  classProperty.componentType = ClassProperty::ComponentType::UINT8;

  ImageAsset image;
  image.width = 2;
  image.height = 2;
  image.channels = 1;
  image.bytesPerChannel = 1;

  std::vector<uint8_t> data{12, 33, 56, 67};
  std::vector<std::byte>& imageData = image.pixelData;
  imageData.resize(data.size());
  std::memcpy(imageData.data(), data.data(), data.size());

  SUBCASE("REPEAT") {
    Sampler sampler;
    sampler.wrapS = Sampler::WrapS::REPEAT;
    sampler.wrapT = Sampler::WrapT::REPEAT;

    PropertyTexturePropertyView<uint8_t> view(
        property,
        classProperty,
        sampler,
        image);
    CHECK(view.getSwizzle() == "r");

    std::vector<glm::dvec2> uvs{
        glm::dvec2(1.0, 0),
        glm::dvec2(-1.5, 0),
        glm::dvec2(0, -0.5),
        glm::dvec2(1.5, -0.5)};
    for (size_t i = 0; i < uvs.size(); i++) {
      glm::dvec2 uv = uvs[i];
      REQUIRE(view.getRaw(uv[0], uv[1]) == data[i]);
      REQUIRE(view.get(uv[0], uv[1]) == data[i]);
    }
  }

  SUBCASE("MIRRORED_REPEAT") {
    Sampler sampler;
    sampler.wrapS = Sampler::WrapS::MIRRORED_REPEAT;
    sampler.wrapT = Sampler::WrapT::MIRRORED_REPEAT;
    // REPEAT:   | 1 2 3 | 1 2 3 |
    // MIRRORED: | 1 2 3 | 3 2 1 |
    // Sampling 0.6 is equal to sampling 1.4 or -0.6.

    PropertyTexturePropertyView<uint8_t> view(
        property,
        classProperty,
        sampler,
        image);
    CHECK(view.getSwizzle() == "r");

    std::vector<glm::dvec2> uvs{
        glm::dvec2(2.0, 0),
        glm::dvec2(-0.75, 0),
        glm::dvec2(0, 1.25),
        glm::dvec2(-1.25, 2.75)};
    for (size_t i = 0; i < uvs.size(); i++) {
      glm::dvec2 uv = uvs[i];
      REQUIRE(view.getRaw(uv[0], uv[1]) == data[i]);
      REQUIRE(view.get(uv[0], uv[1]) == data[i]);
    }
  }

  SUBCASE("CLAMP_TO_EDGE") {
    Sampler sampler;
    sampler.wrapS = Sampler::WrapS::CLAMP_TO_EDGE;
    sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

    PropertyTexturePropertyView<uint8_t> view(
        property,
        classProperty,
        sampler,
        image);
    CHECK(view.getSwizzle() == "r");

    std::vector<glm::dvec2> uvs{
        glm::dvec2(-1.0, 0),
        glm::dvec2(1.4, 0),
        glm::dvec2(0, 2.0),
        glm::dvec2(1.5, 1.5)};
    for (size_t i = 0; i < uvs.size(); i++) {
      glm::dvec2 uv = uvs[i];
      REQUIRE(view.getRaw(uv[0], uv[1]) == data[i]);
      REQUIRE(view.get(uv[0], uv[1]) == data[i]);
    }
  }

  SUBCASE("Mismatched wrap values") {
    Sampler sampler;
    sampler.wrapS = Sampler::WrapS::REPEAT;
    sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

    PropertyTexturePropertyView<uint8_t> view(
        property,
        classProperty,
        sampler,
        image);
    CHECK(view.getSwizzle() == "r");

    std::vector<glm::dvec2> uvs{
        glm::dvec2(1.0, 0),
        glm::dvec2(-1.5, -1.0),
        glm::dvec2(0, 1.5),
        glm::dvec2(1.5, 1.5)};
    for (size_t i = 0; i < uvs.size(); i++) {
      glm::dvec2 uv = uvs[i];
      REQUIRE(view.getRaw(uv[0], uv[1]) == data[i]);
      REQUIRE(view.get(uv[0], uv[1]) == data[i]);
    }
  }
}

TEST_CASE("Test PropertyTextureProperty constructs with "
          "applyKhrTextureTransformExtension = true") {
  std::vector<uint8_t> data{1, 2, 3, 4};

  PropertyTextureProperty property;
  property.texCoord = 0;

  ExtensionKhrTextureTransform& textureTransformExtension =
      property.addExtension<ExtensionKhrTextureTransform>();
  textureTransformExtension.offset = {0.5, -0.5};
  textureTransformExtension.rotation = CesiumUtility::Math::PiOverTwo;
  textureTransformExtension.scale = {0.5, 0.5};
  textureTransformExtension.texCoord = 10;

  ClassProperty classProperty;
  classProperty.type = ClassProperty::Type::SCALAR;
  classProperty.componentType = ClassProperty::ComponentType::UINT8;

  Sampler sampler;
  sampler.wrapS = Sampler::WrapS::REPEAT;
  sampler.wrapT = Sampler::WrapT::REPEAT;

  ImageAsset image;
  image.width = 2;
  image.height = 2;
  image.channels = 1;
  image.bytesPerChannel = 1;

  std::vector<std::byte>& imageData = image.pixelData;
  imageData.resize(data.size());
  std::memcpy(imageData.data(), data.data(), data.size());

  property.channels = {0};

  TextureViewOptions options;
  options.applyKhrTextureTransformExtension = true;

  PropertyTexturePropertyView<uint8_t>
      view(property, classProperty, sampler, image, options);
  REQUIRE(view.status() == PropertyTexturePropertyViewStatus::Valid);

  auto textureTransform = view.getTextureTransform();
  REQUIRE(textureTransform != std::nullopt);
  REQUIRE(textureTransform->offset() == glm::dvec2(0.5, -0.5));
  REQUIRE(textureTransform->rotation() == CesiumUtility::Math::PiOverTwo);
  REQUIRE(textureTransform->scale() == glm::dvec2(0.5, 0.5));

  // Texcoord is overridden by value in KHR_texture_transform.
  REQUIRE(
      view.getTexCoordSetIndex() == textureTransform->getTexCoordSetIndex());
  REQUIRE(textureTransform->getTexCoordSetIndex() == 10);

  // This transforms to the following UV values:
  // (0, 0) -> (0.5, -0.5) -> wraps to (0.5, 0.5)
  // (1, 0) -> (0.5, -1) -> wraps to (0.5, 0)
  // (0, 1) -> (1, -0.5) -> wraps to (0, 0.5)
  // (1, 1) -> (1, -1) -> wraps to (0.0, 0.0)
  std::vector<glm::dvec2> texCoords{
      glm::dvec2(0, 0),
      glm::dvec2(1.0, 0),
      glm::dvec2(0, 1.0),
      glm::dvec2(1.0, 1.0)};

  std::vector<uint8_t> expectedValues{4, 2, 3, 1};

  for (size_t i = 0; i < texCoords.size(); i++) {
    glm::dvec2 uv = texCoords[i];
    REQUIRE(view.getRaw(uv[0], uv[1]) == expectedValues[i]);
    REQUIRE(view.get(uv[0], uv[1]) == expectedValues[i]);
  }
}

TEST_CASE("Test normalized PropertyTextureProperty constructs with "
          "applyKhrTextureTransformExtension = true") {
  std::vector<uint8_t> data{0, 64, 127, 255};

  PropertyTextureProperty property;
  property.texCoord = 0;

  ExtensionKhrTextureTransform& textureTransformExtension =
      property.addExtension<ExtensionKhrTextureTransform>();
  textureTransformExtension.offset = {0.5, -0.5};
  textureTransformExtension.rotation = CesiumUtility::Math::PiOverTwo;
  textureTransformExtension.scale = {0.5, 0.5};
  textureTransformExtension.texCoord = 10;

  ClassProperty classProperty;
  classProperty.type = ClassProperty::Type::SCALAR;
  classProperty.componentType = ClassProperty::ComponentType::UINT8;
  classProperty.normalized = true;

  Sampler sampler;
  sampler.wrapS = Sampler::WrapS::REPEAT;
  sampler.wrapT = Sampler::WrapT::REPEAT;

  ImageAsset image;
  image.width = 2;
  image.height = 2;
  image.channels = 1;
  image.bytesPerChannel = 1;

  std::vector<std::byte>& imageData = image.pixelData;
  imageData.resize(data.size());
  std::memcpy(imageData.data(), data.data(), data.size());

  property.channels = {0};

  TextureViewOptions options;
  options.applyKhrTextureTransformExtension = true;

  PropertyTexturePropertyView<uint8_t, true>
      view(property, classProperty, sampler, image, options);
  REQUIRE(view.status() == PropertyTexturePropertyViewStatus::Valid);

  auto textureTransform = view.getTextureTransform();
  REQUIRE(textureTransform != std::nullopt);
  REQUIRE(textureTransform->offset() == glm::dvec2(0.5, -0.5));
  REQUIRE(textureTransform->rotation() == CesiumUtility::Math::PiOverTwo);
  REQUIRE(textureTransform->scale() == glm::dvec2(0.5, 0.5));

  // Texcoord is overridden by value in KHR_texture_transform.
  REQUIRE(
      view.getTexCoordSetIndex() == textureTransform->getTexCoordSetIndex());
  REQUIRE(textureTransform->getTexCoordSetIndex() == 10);

  // This transforms to the following UV values:
  // (0, 0) -> (0.5, -0.5) -> wraps to (0.5, 0.5)
  // (1, 0) -> (0.5, -1) -> wraps to (0.5, 0)
  // (0, 1) -> (1, -0.5) -> wraps to (0, 0.5)
  // (1, 1) -> (1, -1) -> wraps to (0.0, 0.0)
  std::vector<glm::dvec2> texCoords{
      glm::dvec2(0, 0),
      glm::dvec2(1.0, 0),
      glm::dvec2(0, 1.0),
      glm::dvec2(1.0, 1.0)};

  std::vector<uint8_t> expectedValues{255, 64, 127, 0};

  for (size_t i = 0; i < texCoords.size(); i++) {
    glm::dvec2 uv = texCoords[i];
    REQUIRE(view.getRaw(uv[0], uv[1]) == expectedValues[i]);
    REQUIRE(
        view.get(uv[0], uv[1]) ==
        static_cast<double>(expectedValues[i]) / 255.0);
  }
}

TEST_CASE("Test PropertyTextureProperty constructs with "
          "makeImageCopy = true") {
  std::vector<uint8_t> data{1, 2, 3, 4};

  PropertyTextureProperty property;
  property.texCoord = 0;

  ExtensionKhrTextureTransform& textureTransformExtension =
      property.addExtension<ExtensionKhrTextureTransform>();
  textureTransformExtension.offset = {0.5, -0.5};
  textureTransformExtension.rotation = CesiumUtility::Math::PiOverTwo;
  textureTransformExtension.scale = {0.5, 0.5};
  textureTransformExtension.texCoord = 10;

  ClassProperty classProperty;
  classProperty.type = ClassProperty::Type::SCALAR;
  classProperty.componentType = ClassProperty::ComponentType::UINT8;

  Sampler sampler;
  sampler.wrapS = Sampler::WrapS::REPEAT;
  sampler.wrapT = Sampler::WrapT::REPEAT;

  ImageAsset image;
  image.width = 2;
  image.height = 2;
  image.channels = 1;
  image.bytesPerChannel = 1;

  std::vector<std::byte>& imageData = image.pixelData;
  imageData.resize(data.size());
  std::memcpy(imageData.data(), data.data(), data.size());

  property.channels = {0};

  TextureViewOptions options;
  options.makeImageCopy = true;

  PropertyTexturePropertyView<uint8_t>
      view(property, classProperty, sampler, image, options);
  REQUIRE(view.status() == PropertyTexturePropertyViewStatus::Valid);

  // Clear the original image data.
  std::vector<std::byte> emptyData;
  image.pixelData.swap(emptyData);

  const ImageAsset* pImage = view.getImage();
  REQUIRE(pImage);
  REQUIRE(pImage->width == image.width);
  REQUIRE(pImage->height == image.height);
  REQUIRE(pImage->channels == image.channels);
  REQUIRE(pImage->bytesPerChannel == image.bytesPerChannel);
  REQUIRE(pImage->pixelData.size() == data.size());

  std::vector<glm::dvec2> texCoords{
      glm::dvec2(0, 0),
      glm::dvec2(0.5, 0),
      glm::dvec2(0, 0.5),
      glm::dvec2(0.5, 0.5)};

  for (size_t i = 0; i < texCoords.size(); i++) {
    glm::dvec2 uv = texCoords[i];
    REQUIRE(view.getRaw(uv[0], uv[1]) == data[i]);
    REQUIRE(view.get(uv[0], uv[1]) == data[i]);
  }
}

TEST_CASE("Test normalized PropertyTextureProperty constructs with "
          "makeImageCopy = true") {
  std::vector<uint8_t> data{0, 64, 127, 255};

  PropertyTextureProperty property;
  property.texCoord = 0;

  ExtensionKhrTextureTransform& textureTransformExtension =
      property.addExtension<ExtensionKhrTextureTransform>();
  textureTransformExtension.offset = {0.5, -0.5};
  textureTransformExtension.rotation = CesiumUtility::Math::PiOverTwo;
  textureTransformExtension.scale = {0.5, 0.5};
  textureTransformExtension.texCoord = 10;

  ClassProperty classProperty;
  classProperty.type = ClassProperty::Type::SCALAR;
  classProperty.componentType = ClassProperty::ComponentType::UINT8;
  classProperty.normalized = true;

  Sampler sampler;
  sampler.wrapS = Sampler::WrapS::REPEAT;
  sampler.wrapT = Sampler::WrapT::REPEAT;

  ImageAsset image;
  image.width = 2;
  image.height = 2;
  image.channels = 1;
  image.bytesPerChannel = 1;

  std::vector<std::byte>& imageData = image.pixelData;
  imageData.resize(data.size());
  std::memcpy(imageData.data(), data.data(), data.size());

  property.channels = {0};

  TextureViewOptions options;
  options.makeImageCopy = true;

  PropertyTexturePropertyView<uint8_t, true>
      view(property, classProperty, sampler, image, options);
  REQUIRE(view.status() == PropertyTexturePropertyViewStatus::Valid);

  // Clear the original image data.
  std::vector<std::byte> emptyData;
  image.pixelData.swap(emptyData);

  const ImageAsset* pImage = view.getImage();
  REQUIRE(pImage);
  REQUIRE(pImage->width == image.width);
  REQUIRE(pImage->height == image.height);
  REQUIRE(pImage->channels == image.channels);
  REQUIRE(pImage->bytesPerChannel == image.bytesPerChannel);
  REQUIRE(pImage->pixelData.size() == data.size());

  std::vector<glm::dvec2> texCoords{
      glm::dvec2(0, 0),
      glm::dvec2(0.5, 0),
      glm::dvec2(0, 0.5),
      glm::dvec2(0.5, 0.5)};

  for (size_t i = 0; i < texCoords.size(); i++) {
    glm::dvec2 uv = texCoords[i];
    REQUIRE(view.getRaw(uv[0], uv[1]) == data[i]);
    REQUIRE(view.get(uv[0], uv[1]) == static_cast<double>(data[i]) / 255.0);
  }
}

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
