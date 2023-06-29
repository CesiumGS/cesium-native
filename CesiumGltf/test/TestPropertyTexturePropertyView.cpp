#include "CesiumGltf/PropertyTexturePropertyView.h"

#include <catch2/catch.hpp>
#include <gsl/span>

#include <bitset>
#include <climits>
#include <cstddef>
#include <cstring>
#include <vector>

using namespace CesiumGltf;

TEST_CASE("Check scalar PropertyTexturePropertyView") {
  SECTION("uint8_t") {
    Sampler sampler;
    ImageCesium image;
    image.width = 2;
    image.height = 2;
    image.channels = 1;
    image.bytesPerChannel = 1;

    std::vector<uint8_t> data{12, 33, 56, 67};
    std::vector<std::byte>& imageData = image.pixelData;
    imageData.resize(data.size());
    std::memcpy(imageData.data(), data.data(), data.size());

    std::vector<int64_t> channels{0};

    PropertyTexturePropertyView<uint8_t>
        view(sampler, image, 0, channels, false);
    CHECK(view.getSwizzle() == "r");

    std::vector<uint8_t> values{
        view.get(0, 0),
        view.get(0.5, 0),
        view.get(0, 0.5),
        view.get(0.5, 0.5)};
    for (size_t i = 0; i < values.size(); i++) {
      REQUIRE(values[i] == data[i]);
    }
  }

  SECTION("int8_t") {
    Sampler sampler;
    ImageCesium image;
    image.width = 2;
    image.height = 2;
    image.channels = 1;
    image.bytesPerChannel = 1;

    std::vector<uint8_t> data{255, 0, 223, 67};
    std::vector<std::byte>& imageData = image.pixelData;
    imageData.resize(data.size());
    std::memcpy(imageData.data(), data.data(), data.size());

    std::vector<int64_t> channels{0};

    PropertyTexturePropertyView<int8_t>
        view(sampler, image, 0, channels, false);
    CHECK(view.getSwizzle() == "r");

    std::vector<int8_t> expected{-1, 0, -33, 67};
    std::vector<int8_t> values{
        view.get(0, 0),
        view.get(0.5, 0),
        view.get(0, 0.5),
        view.get(0.5, 0.5)};
    for (size_t i = 0; i < values.size(); i++) {
      REQUIRE(values[i] == expected[i]);
    }
  }

  SECTION("uint16_t") {
    Sampler sampler;
    ImageCesium image;
    image.width = 2;
    image.height = 2;
    image.channels = 2;
    image.bytesPerChannel = 1;

    // clang-format off
    std::vector<uint8_t> data{
      28, 0,
      1, 1,
      0, 3,
      182, 1};
    // clang-format on

    std::vector<std::byte>& imageData = image.pixelData;
    imageData.resize(data.size());
    std::memcpy(imageData.data(), data.data(), data.size());

    std::vector<int64_t> channels{0, 1};

    PropertyTexturePropertyView<uint16_t>
        view(sampler, image, 0, channels, false);
    CHECK(view.getSwizzle() == "rg");

    std::vector<uint16_t> expected{28, 257, 768, 438};
    std::vector<uint16_t> values{
        view.get(0, 0),
        view.get(0.5, 0),
        view.get(0, 0.5),
        view.get(0.5, 0.5)};
    for (size_t i = 0; i < values.size(); i++) {
      REQUIRE(values[i] == expected[i]);
    }
  }

  SECTION("int16_t") {
    Sampler sampler;
    ImageCesium image;
    image.width = 2;
    image.height = 2;
    image.channels = 2;
    image.bytesPerChannel = 1;

    // clang-format off
    std::vector<uint8_t> data{
      255, 255,
      1, 129,
      0, 3,
      182, 1};
    // clang-format on

    std::vector<std::byte>& imageData = image.pixelData;
    imageData.resize(data.size());
    std::memcpy(imageData.data(), data.data(), data.size());

    std::vector<int64_t> channels{0, 1};

    PropertyTexturePropertyView<int16_t>
        view(sampler, image, 0, channels, false);
    CHECK(view.getSwizzle() == "rg");

    std::vector<int16_t> expected{-1, -32511, 768, 438};
    std::vector<int16_t> values{
        view.get(0, 0),
        view.get(0.5, 0),
        view.get(0, 0.5),
        view.get(0.5, 0.5)};
    for (size_t i = 0; i < values.size(); i++) {
      REQUIRE(values[i] == expected[i]);
    }
  }

  SECTION("uint32_t") {
    Sampler sampler;
    ImageCesium image;
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

    std::vector<std::byte>& imageData = image.pixelData;
    imageData.resize(data.size());
    std::memcpy(imageData.data(), data.data(), data.size());

    std::vector<int64_t> channels{0, 1, 2, 3};

    PropertyTexturePropertyView<uint32_t>
        view(sampler, image, 0, channels, false);
    CHECK(view.getSwizzle() == "rgba");

    std::vector<uint32_t> expected{16777216, 65545, 131604, 16777480};
    std::vector<uint32_t> values{
        view.get(0, 0),
        view.get(0.5, 0),
        view.get(0, 0.5),
        view.get(0.5, 0.5)};
    for (size_t i = 0; i < values.size(); i++) {
      REQUIRE(values[i] == expected[i]);
    }
  }

  SECTION("int32_t") {
    Sampler sampler;
    ImageCesium image;
    image.width = 2;
    image.height = 2;
    image.channels = 4;
    image.bytesPerChannel = 1;

    // clang-format off
    std::vector<uint8_t> data{
      255, 255, 255, 255,
      9, 0, 1, 0,
      20, 2, 2, 255,
      8, 1, 0, 1};
    // clang-format on

    std::vector<std::byte>& imageData = image.pixelData;
    imageData.resize(data.size());
    std::memcpy(imageData.data(), data.data(), data.size());

    std::vector<int64_t> channels{0, 1, 2, 3};

    PropertyTexturePropertyView<int32_t>
        view(sampler, image, 0, channels, false);
    CHECK(view.getSwizzle() == "rgba");
    std::vector<int32_t> expected{-1, 65545, -16645612, 16777480};
    std::vector<int32_t> values{
        view.get(0, 0),
        view.get(0.5, 0),
        view.get(0, 0.5),
        view.get(0.5, 0.5)};
    for (size_t i = 0; i < values.size(); i++) {
      REQUIRE(values[i] == expected[i]);
    }
  }

  SECTION("float") {
    Sampler sampler;
    ImageCesium image;
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

    std::vector<std::byte>& imageData = image.pixelData;
    imageData.resize(data.size());
    std::memcpy(imageData.data(), data.data(), data.size());

    std::vector<int64_t> channels{0, 1, 2, 3};

    PropertyTexturePropertyView<float> view(sampler, image, 0, channels, false);
    CHECK(view.getSwizzle() == "rgba");

    std::vector<uint32_t> expectedUint{16777216, 65545, 131604, 16777480};
    std::vector<float> expected(expectedUint.size());
    for (size_t i = 0; i < expectedUint.size(); i++) {
      float value = *reinterpret_cast<float*>(&expectedUint[i]);
      expected[i] = value;
    }

    std::vector<float> values{
        view.get(0, 0),
        view.get(0.5, 0),
        view.get(0, 0.5),
        view.get(0.5, 0.5)};
    for (size_t i = 0; i < values.size(); i++) {
      REQUIRE(values[i] == expected[i]);
    }
  }
}

TEST_CASE("Check vecN PropertyTexturePropertyView") {
  SECTION("glm::u8vec2") {
    Sampler sampler;
    ImageCesium image;
    image.width = 2;
    image.height = 2;
    image.channels = 2;
    image.bytesPerChannel = 1;

    // clang-format off
    std::vector<uint8_t> data{
      28, 0,
      1, 1,
      0, 3,
      182, 1};
    // clang-format on

    std::vector<std::byte>& imageData = image.pixelData;
    imageData.resize(data.size());
    std::memcpy(imageData.data(), data.data(), data.size());

    std::vector<int64_t> channels{0, 1};

    PropertyTexturePropertyView<glm::u8vec2>
        view(sampler, image, 0, channels, false);
    CHECK(view.getSwizzle() == "rg");

    std::vector<glm::u8vec2> expected{
        glm::u8vec2(28, 0),
        glm::u8vec2(1, 1),
        glm::u8vec2(0, 3),
        glm::u8vec2(182, 1)};
    std::vector<glm::u8vec2> values{
        view.get(0, 0),
        view.get(0.5, 0),
        view.get(0, 0.5),
        view.get(0.5, 0.5)};
    for (size_t i = 0; i < values.size(); i++) {
      REQUIRE(values[i] == expected[i]);
    }
  }

  SECTION("glm::i8vec2") {
    Sampler sampler;
    ImageCesium image;
    image.width = 2;
    image.height = 2;
    image.channels = 2;
    image.bytesPerChannel = 1;

    // clang-format off
    std::vector<uint8_t> data{
      28, 255,
      254, 1,
      0, 3,
      182, 1};
    // clang-format on

    std::vector<std::byte>& imageData = image.pixelData;
    imageData.resize(data.size());
    std::memcpy(imageData.data(), data.data(), data.size());

    std::vector<int64_t> channels{0, 1};

    PropertyTexturePropertyView<glm::i8vec2>
        view(sampler, image, 0, channels, false);
    CHECK(view.getSwizzle() == "rg");

    std::vector<glm::i8vec2> expected{
        glm::i8vec2(28, -1),
        glm::i8vec2(-2, 1),
        glm::i8vec2(0, 3),
        glm::i8vec2(-74, 1)};
    std::vector<glm::i8vec2> values{
        view.get(0, 0),
        view.get(0.5, 0),
        view.get(0, 0.5),
        view.get(0.5, 0.5)};
    for (size_t i = 0; i < values.size(); i++) {
      REQUIRE(values[i] == expected[i]);
    }
  }

  SECTION("glm::u8vec3") {
    Sampler sampler;
    ImageCesium image;
    image.width = 2;
    image.height = 2;
    image.channels = 3;
    image.bytesPerChannel = 1;

    // clang-format off
    std::vector<uint8_t> data{
      1, 2, 3,
      4, 5, 6,
      7, 8, 9,
      0, 5, 2};
    // clang-format on

    std::vector<std::byte>& imageData = image.pixelData;
    imageData.resize(data.size());
    std::memcpy(imageData.data(), data.data(), data.size());

    std::vector<int64_t> channels{0, 1, 2};

    PropertyTexturePropertyView<glm::u8vec3>
        view(sampler, image, 0, channels, false);
    CHECK(view.getSwizzle() == "rgb");

    std::vector<glm::u8vec3> expected{
        glm::u8vec3(1, 2, 3),
        glm::u8vec3(4, 5, 6),
        glm::u8vec3(7, 8, 9),
        glm::u8vec3(0, 5, 2)};
    std::vector<glm::u8vec3> values{
        view.get(0, 0),
        view.get(0.5, 0),
        view.get(0, 0.5),
        view.get(0.5, 0.5)};
    for (size_t i = 0; i < values.size(); i++) {
      REQUIRE(values[i] == expected[i]);
    }
  }

  SECTION("glm::i8vec3") {
    Sampler sampler;
    ImageCesium image;
    image.width = 2;
    image.height = 2;
    image.channels = 3;
    image.bytesPerChannel = 1;

    // clang-format off
    std::vector<uint8_t> data{
      255, 2, 3,
      4, 254, 6,
      7, 8, 159,
      0, 5, 2};
    // clang-format on

    std::vector<std::byte>& imageData = image.pixelData;
    imageData.resize(data.size());
    std::memcpy(imageData.data(), data.data(), data.size());

    std::vector<int64_t> channels{0, 1, 2};

    PropertyTexturePropertyView<glm::i8vec3>
        view(sampler, image, 0, channels, false);
    CHECK(view.getSwizzle() == "rgb");

    std::vector<glm::i8vec3> expected{
        glm::i8vec3(-1, 2, 3),
        glm::i8vec3(4, -2, 6),
        glm::i8vec3(7, 8, -97),
        glm::i8vec3(0, 5, 2)};
    std::vector<glm::i8vec3> values{
        view.get(0, 0),
        view.get(0.5, 0),
        view.get(0, 0.5),
        view.get(0.5, 0.5)};
    for (size_t i = 0; i < values.size(); i++) {
      REQUIRE(values[i] == expected[i]);
    }
  }

  SECTION("glm::u8vec4") {
    Sampler sampler;
    ImageCesium image;
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

    std::vector<int64_t> channels{0, 1, 2, 3};

    PropertyTexturePropertyView<glm::u8vec4>
        view(sampler, image, 0, channels, false);
    CHECK(view.getSwizzle() == "rgba");

    std::vector<glm::u8vec4> expected{
        glm::u8vec4(1, 2, 3, 0),
        glm::u8vec4(4, 5, 6, 11),
        glm::u8vec4(7, 8, 9, 3),
        glm::u8vec4(0, 5, 2, 27)};
    std::vector<glm::u8vec4> values{
        view.get(0, 0),
        view.get(0.5, 0),
        view.get(0, 0.5),
        view.get(0.5, 0.5)};
    for (size_t i = 0; i < values.size(); i++) {
      REQUIRE(values[i] == expected[i]);
    }
  }

  SECTION("glm::i8vec4") {
    Sampler sampler;
    ImageCesium image;
    image.width = 2;
    image.height = 2;
    image.channels = 4;
    image.bytesPerChannel = 1;

    // clang-format off
    std::vector<uint8_t> data{
      1, 200, 3, 0,
      4, 5, 6, 251,
      129, 8, 9, 3,
      0, 155, 2, 27};
    // clang-format on

    std::vector<std::byte>& imageData = image.pixelData;
    imageData.resize(data.size());
    std::memcpy(imageData.data(), data.data(), data.size());

    std::vector<int64_t> channels{0, 1, 2, 3};

    PropertyTexturePropertyView<glm::i8vec4>
        view(sampler, image, 0, channels, false);
    CHECK(view.getSwizzle() == "rgba");

    std::vector<glm::i8vec4> expected{
        glm::i8vec4(1, -56, 3, 0),
        glm::i8vec4(4, 5, 6, -5),
        glm::i8vec4(-127, 8, 9, 3),
        glm::i8vec4(0, -101, 2, 27)};
    std::vector<glm::i8vec4> values{
        view.get(0, 0),
        view.get(0.5, 0),
        view.get(0, 0.5),
        view.get(0.5, 0.5)};
    for (size_t i = 0; i < values.size(); i++) {
      REQUIRE(values[i] == expected[i]);
    }
  }

  SECTION("glm::u16vec2") {
    Sampler sampler;
    ImageCesium image;
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

    std::vector<std::byte>& imageData = image.pixelData;
    imageData.resize(data.size());
    std::memcpy(imageData.data(), data.data(), data.size());

    std::vector<int64_t> channels{0, 1, 2, 3};

    PropertyTexturePropertyView<glm::u16vec2>
        view(sampler, image, 0, channels, false);
    CHECK(view.getSwizzle() == "rgba");

    std::vector<glm::u16vec2> expected{
        glm::u16vec2(0, 256),
        glm::u16vec2(9, 1),
        glm::u16vec2(532, 2),
        glm::u16vec2(264, 256)};
    std::vector<glm::u16vec2> values{
        view.get(0, 0),
        view.get(0.5, 0),
        view.get(0, 0.5),
        view.get(0.5, 0.5)};
    for (size_t i = 0; i < values.size(); i++) {
      REQUIRE(values[i] == expected[i]);
    }
  }

  SECTION("glm::i16vec2") {
    Sampler sampler;
    ImageCesium image;
    image.width = 2;
    image.height = 2;
    image.channels = 4;
    image.bytesPerChannel = 1;

    // clang-format off
    std::vector<uint8_t> data{
      255, 255, 0, 1,
      9, 0, 146, 195,
      20, 2, 2, 0,
      8, 1, 255, 1};
    // clang-format on

    std::vector<std::byte>& imageData = image.pixelData;
    imageData.resize(data.size());
    std::memcpy(imageData.data(), data.data(), data.size());

    std::vector<int64_t> channels{0, 1, 2, 3};

    PropertyTexturePropertyView<glm::i16vec2>
        view(sampler, image, 0, channels, false);
    CHECK(view.getSwizzle() == "rgba");

    std::vector<glm::i16vec2> expected{
        glm::i16vec2(-1, 256),
        glm::i16vec2(9, -15470),
        glm::i16vec2(532, 2),
        glm::i16vec2(264, 511)};
    std::vector<glm::i16vec2> values{
        view.get(0, 0),
        view.get(0.5, 0),
        view.get(0, 0.5),
        view.get(0.5, 0.5)};
    for (size_t i = 0; i < values.size(); i++) {
      REQUIRE(values[i] == expected[i]);
    }
  }
}

TEST_CASE("Check array PropertyTexturePropertyView") {
  SECTION("uint8_t array") {
    Sampler sampler;
    ImageCesium image;
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

    std::vector<int64_t> channels{0, 1, 2, 3};

    PropertyTexturePropertyView<PropertyArrayView<uint8_t>>
        view(sampler, image, 0, channels, false);
    CHECK(view.getSwizzle() == "rgba");

    std::vector<PropertyArrayView<uint8_t>> values{
        view.get(0, 0),
        view.get(0.5, 0),
        view.get(0, 0.5),
        view.get(0.5, 0.5)};

    for (size_t i = 0; i < values.size(); i++) {
      auto dataStart = data.begin() + i * 4;
      std::vector<uint8_t> expected(dataStart, dataStart + 4);
      const PropertyArrayView<uint8_t>& value = values[i];
      REQUIRE(static_cast<size_t>(value.size()) == expected.size());
      for (size_t j = 0; j < expected.size(); j++) {
        REQUIRE(value[j] == expected[j]);
      }
    }
  }

  SECTION("int8_t array") {
    Sampler sampler;
    ImageCesium image;
    image.width = 2;
    image.height = 2;
    image.channels = 4;
    image.bytesPerChannel = 1;

    // clang-format off
    std::vector<uint8_t> data{
      1, 200, 3, 0,
      4, 5, 6, 251,
      129, 8, 9, 3,
      0, 155, 2, 27};
    // clang-format on

    std::vector<std::byte>& imageData = image.pixelData;
    imageData.resize(data.size());
    std::memcpy(imageData.data(), data.data(), data.size());

    std::vector<int64_t> channels{0, 1, 2, 3};

    PropertyTexturePropertyView<PropertyArrayView<int8_t>>
        view(sampler, image, 0, channels, false);
    CHECK(view.getSwizzle() == "rgba");

    std::vector<std::vector<int8_t>> expected{
        {1, -56, 3, 0},
        {4, 5, 6, -5},
        {-127, 8, 9, 3},
        {0, -101, 2, 27}};
    std::vector<PropertyArrayView<int8_t>> values{
        view.get(0, 0),
        view.get(0.5, 0),
        view.get(0, 0.5),
        view.get(0.5, 0.5)};
    for (size_t i = 0; i < values.size(); i++) {
      std::vector<int8_t>& expectedValue = expected[i];
      const PropertyArrayView<int8_t>& value = values[i];
      REQUIRE(static_cast<size_t>(value.size()) == expected.size());
      for (size_t j = 0; j < expected.size(); j++) {
        REQUIRE(value[j] == expectedValue[j]);
      }
    }
  }

  SECTION("uint16_t array") {
    Sampler sampler;
    ImageCesium image;
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

    std::vector<std::byte>& imageData = image.pixelData;
    imageData.resize(data.size());
    std::memcpy(imageData.data(), data.data(), data.size());

    std::vector<int64_t> channels{0, 1, 2, 3};

    PropertyTexturePropertyView<PropertyArrayView<uint16_t>>
        view(sampler, image, 0, channels, false);
    CHECK(view.getSwizzle() == "rgba");

    std::vector<PropertyArrayView<uint16_t>> values{
        view.get(0, 0),
        view.get(0.5, 0),
        view.get(0, 0.5),
        view.get(0.5, 0.5)};

    std::vector<std::vector<uint16_t>> expected{
        {0, 256},
        {9, 1},
        {532, 2},
        {264, 256}};

    for (size_t i = 0; i < expected.size(); i++) {
      std::vector<uint16_t>& expectedValue = expected[i];
      const PropertyArrayView<uint16_t>& value = values[i];
      REQUIRE(static_cast<size_t>(value.size()) == expectedValue.size());
      for (size_t j = 0; j < expectedValue.size(); j++) {
        REQUIRE(value[j] == expectedValue[j]);
      }
    }
  }

  SECTION("int16_t array") {
    Sampler sampler;
    ImageCesium image;
    image.width = 2;
    image.height = 2;
    image.channels = 4;
    image.bytesPerChannel = 1;

    // clang-format off
    std::vector<uint8_t> data{
      255, 255, 0, 1,
      9, 0, 146, 195,
      20, 2, 2, 0,
      8, 255, 0, 1};
    // clang-format on

    std::vector<std::byte>& imageData = image.pixelData;
    imageData.resize(data.size());
    std::memcpy(imageData.data(), data.data(), data.size());

    std::vector<int64_t> channels{0, 1, 2, 3};

    PropertyTexturePropertyView<PropertyArrayView<int16_t>>
        view(sampler, image, 0, channels, false);
    CHECK(view.getSwizzle() == "rgba");

    std::vector<std::vector<int16_t>> expected{
        {-1, 256},
        {9, -15470},
        {532, 2},
        {-248, 256}};
    std::vector<PropertyArrayView<int16_t>> values{
        view.get(0, 0),
        view.get(0.5, 0),
        view.get(0, 0.5),
        view.get(0.5, 0.5)};
    for (size_t i = 0; i < values.size(); i++) {
      std::vector<int16_t>& expectedValue = expected[i];
      const PropertyArrayView<int16_t>& value = values[i];
      REQUIRE(static_cast<size_t>(value.size()) == expectedValue.size());
      for (size_t j = 0; j < expectedValue.size(); j++) {
        REQUIRE(value[j] == expectedValue[j]);
      }
    }
  }
}

TEST_CASE("Check that non-adjacent channels resolve to expected output") {
  SECTION("single-byte scalar") {
    Sampler sampler;
    ImageCesium image;
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

    std::vector<int64_t> channels{3};

    PropertyTexturePropertyView<uint16_t>
        view(sampler, image, 0, channels, false);
    CHECK(view.getSwizzle() == "a");

    std::vector<uint16_t> expected{3, 4, 0, 1};
    std::vector<uint16_t> values{
        view.get(0, 0),
        view.get(0.5, 0),
        view.get(0, 0.5),
        view.get(0.5, 0.5)};
    for (size_t i = 0; i < values.size(); i++) {
      REQUIRE(values[i] == expected[i]);
    }
  }

  SECTION("multi-byte scalar") {
    Sampler sampler;
    ImageCesium image;
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

    std::vector<int64_t> channels{2, 0};

    PropertyTexturePropertyView<uint16_t>
        view(sampler, image, 0, channels, false);
    CHECK(view.getSwizzle() == "br");

    std::vector<uint16_t> expected{2, 259, 257, 520};
    std::vector<uint16_t> values{
        view.get(0, 0),
        view.get(0.5, 0),
        view.get(0, 0.5),
        view.get(0.5, 0.5)};
    for (size_t i = 0; i < values.size(); i++) {
      REQUIRE(values[i] == expected[i]);
    }
  }

  SECTION("vecN") {
    Sampler sampler;
    ImageCesium image;
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

    std::vector<int64_t> channels{3, 2, 1};

    PropertyTexturePropertyView<glm::u8vec3>
        view(sampler, image, 0, channels, false);
    CHECK(view.getSwizzle() == "abg");

    std::vector<glm::u8vec3> expected{
        glm::u8vec3(3, 2, 1),
        glm::u8vec3(4, 3, 2),
        glm::u8vec3(0, 1, 0),
        glm::u8vec3(1, 8, 3)};
    std::vector<glm::u8vec3> values{
        view.get(0, 0),
        view.get(0.5, 0),
        view.get(0, 0.5),
        view.get(0.5, 0.5)};
    for (size_t i = 0; i < values.size(); i++) {
      REQUIRE(values[i] == expected[i]);
    }
  }

  SECTION("array") {
    Sampler sampler;
    ImageCesium image;
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

    std::vector<int64_t> channels{1, 0, 3, 2};

    PropertyTexturePropertyView<PropertyArrayView<uint8_t>>
        view(sampler, image, 0, channels, false);
    CHECK(view.getSwizzle() == "grab");

    std::vector<PropertyArrayView<uint8_t>> values{
        view.get(0, 0),
        view.get(0.5, 0),
        view.get(0, 0.5),
        view.get(0.5, 0.5)};

    std::vector<std::vector<uint8_t>> expected{
        {2, 1, 0, 3},
        {5, 4, 11, 6},
        {8, 7, 3, 9},
        {5, 0, 27, 2}};

    for (size_t i = 0; i < expected.size(); i++) {
      std::vector<uint8_t>& expectedValue = expected[i];
      const PropertyArrayView<uint8_t>& value = values[i];
      REQUIRE(static_cast<size_t>(value.size()) == expectedValue.size());
      for (size_t j = 0; j < expectedValue.size(); j++) {
        REQUIRE(value[j] == expectedValue[j]);
      }
    }
  }
}

TEST_CASE("Check sampling with different sampler values") {
  SECTION("REPEAT") {
    Sampler sampler;
    sampler.wrapS = Sampler::WrapS::REPEAT;
    sampler.wrapT = Sampler::WrapT::REPEAT;

    ImageCesium image;
    image.width = 2;
    image.height = 2;
    image.channels = 1;
    image.bytesPerChannel = 1;

    std::vector<uint8_t> data{12, 33, 56, 67};
    std::vector<std::byte>& imageData = image.pixelData;
    imageData.resize(data.size());
    std::memcpy(imageData.data(), data.data(), data.size());

    std::vector<int64_t> channels{0};

    PropertyTexturePropertyView<uint8_t>
        view(sampler, image, 0, channels, false);
    CHECK(view.getSwizzle() == "r");

    std::vector<uint8_t> values{
        view.get(1.0, 0),
        view.get(-1.5, 0),
        view.get(0, -0.5),
        view.get(1.5, -0.5)};
    for (size_t i = 0; i < values.size(); i++) {
      REQUIRE(values[i] == data[i]);
    }
  }

  SECTION("MIRRORED_REPEAT") {
    Sampler sampler;
    sampler.wrapS = Sampler::WrapS::MIRRORED_REPEAT;
    sampler.wrapT = Sampler::WrapT::MIRRORED_REPEAT;
    // REPEAT:   | 1 2 3 | 1 2 3 |
    // MIRRORED: | 1 2 3 | 3 2 1 |
    // Sampling 0.6 is equal to sampling 1.4 or -0.6.

    ImageCesium image;
    image.width = 2;
    image.height = 2;
    image.channels = 1;
    image.bytesPerChannel = 1;

    std::vector<uint8_t> data{12, 33, 56, 67};
    std::vector<std::byte>& imageData = image.pixelData;
    imageData.resize(data.size());
    std::memcpy(imageData.data(), data.data(), data.size());

    std::vector<int64_t> channels{0};

    PropertyTexturePropertyView<uint8_t>
        view(sampler, image, 0, channels, false);
    CHECK(view.getSwizzle() == "r");

    std::vector<uint8_t> values{
        view.get(2.0, 0),
        view.get(-0.75, 0),
        view.get(0, 1.25),
        view.get(-1.25, 2.75)};
    for (size_t i = 0; i < values.size(); i++) {
      REQUIRE(values[i] == data[i]);
    }
  }

  SECTION("CLAMP_TO_EDGE") {
    Sampler sampler;
    sampler.wrapS = Sampler::WrapS::CLAMP_TO_EDGE;
    sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

    ImageCesium image;
    image.width = 2;
    image.height = 2;
    image.channels = 1;
    image.bytesPerChannel = 1;

    std::vector<uint8_t> data{12, 33, 56, 67};
    std::vector<std::byte>& imageData = image.pixelData;
    imageData.resize(data.size());
    std::memcpy(imageData.data(), data.data(), data.size());

    std::vector<int64_t> channels{0};

    PropertyTexturePropertyView<uint8_t>
        view(sampler, image, 0, channels, false);
    CHECK(view.getSwizzle() == "r");

    std::vector<uint8_t> values{
        view.get(-1.0, 0),
        view.get(1.4, 0),
        view.get(0, 2.0),
        view.get(1.5, 1.5)};
    for (size_t i = 0; i < values.size(); i++) {
      REQUIRE(values[i] == data[i]);
    }
  }

  SECTION("Mismatched wrap values") {
    Sampler sampler;
    sampler.wrapS = Sampler::WrapS::REPEAT;
    sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

    ImageCesium image;
    image.width = 2;
    image.height = 2;
    image.channels = 1;
    image.bytesPerChannel = 1;

    std::vector<uint8_t> data{12, 33, 56, 67};
    std::vector<std::byte>& imageData = image.pixelData;
    imageData.resize(data.size());
    std::memcpy(imageData.data(), data.data(), data.size());

    std::vector<int64_t> channels{0};

    PropertyTexturePropertyView<uint8_t>
        view(sampler, image, 0, channels, false);
    CHECK(view.getSwizzle() == "r");

    std::vector<uint8_t> values{
        view.get(1.0, 0),
        view.get(-1.5, -1.0),
        view.get(0, 1.5),
        view.get(1.5, 1.5)};
    for (size_t i = 0; i < values.size(); i++) {
      REQUIRE(values[i] == data[i]);
    }
  }
}
