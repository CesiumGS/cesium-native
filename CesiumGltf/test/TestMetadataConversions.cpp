#include "CesiumGltf/MetadataConversions.h"

#include <catch2/catch.hpp>

using namespace CesiumGltf;

TEST_CASE("Test PropertyConversions for boolean") {
  SECTION("converts from boolean") {
    REQUIRE(MetadataConversions<bool, bool>::convert(true, false));
    REQUIRE(!MetadataConversions<bool, bool>::convert(false, true));
  }

  SECTION("converts from scalar") {
    // true for nonzero value
    REQUIRE(MetadataConversions<bool, int8_t>::convert(10, false));
    // false for zero value
    REQUIRE(!MetadataConversions<bool, int8_t>::convert(0, true));
  }

  SECTION("converts from string") {
    std::string_view stringView("true");
    REQUIRE(MetadataConversions<bool, std::string_view>::convert(
        stringView,
        false));

    stringView = std::string_view("yes");
    REQUIRE(MetadataConversions<bool, std::string_view>::convert(
        stringView,
        false));

    stringView = std::string_view("1");
    REQUIRE(MetadataConversions<bool, std::string_view>::convert(
        stringView,
        false));

    stringView = std::string_view("false");
    REQUIRE(!MetadataConversions<bool, std::string_view>::convert(
        stringView,
        true));

    stringView = std::string_view("no");
    REQUIRE(!MetadataConversions<bool, std::string_view>::convert(
        stringView,
        true));

    stringView = std::string_view("0");
    REQUIRE(!MetadataConversions<bool, std::string_view>::convert(
        stringView,
        true));
  }

  SECTION("uses default value for incompatible strings") {
    std::string_view stringView("11");
    // invalid number
    REQUIRE(!MetadataConversions<bool, std::string_view>::convert(
        stringView,
        false));

    stringView = std::string_view("this is true");
    // invalid word
    REQUIRE(!MetadataConversions<bool, std::string_view>::convert(
        stringView,
        false));
  }

  SECTION("uses default value for incompatible types") {
    // vecN
    REQUIRE(!MetadataConversions<bool, glm::vec3>::convert(
        glm::vec3(1, 2, 3),
        false));
    // matN
    REQUIRE(!MetadataConversions<bool, glm::mat2>::convert(glm::mat2(), false));
    // array
    REQUIRE(!MetadataConversions<bool, PropertyArrayView<bool>>::convert(
        PropertyArrayView<bool>(),
        false));
  }
}

TEST_CASE("Test PropertyConversions for integer") {
  SECTION("converts from in-range integer") {
    // same type
    REQUIRE(MetadataConversions<int32_t, int32_t>::convert(50, 0) == 50);
    // different size
    REQUIRE(MetadataConversions<int32_t, int64_t>::convert(50, 0) == 50);
    // different sign
    REQUIRE(MetadataConversions<int32_t, uint32_t>::convert(50, 0) == 50);
  }

  SECTION("converts from in-range floating point number") {
    REQUIRE(MetadataConversions<int32_t, float>::convert(50.125f, 0) == 50);
    REQUIRE(
        MetadataConversions<int32_t, double>::convert(1234.05678f, 0) == 1234);
  }

  SECTION("converts from boolean") {
    REQUIRE(MetadataConversions<int32_t, bool>::convert(true, -1) == 1);
    REQUIRE(MetadataConversions<int32_t, bool>::convert(false, -1) == 0);
  }

  SECTION("converts from string") {
    // integer string
    std::string_view value("-123");
    REQUIRE(
        MetadataConversions<int32_t, std::string_view>::convert(value, 0) ==
        -123);
    // double string
    value = std::string_view("123.456");
    REQUIRE(
        MetadataConversions<int32_t, std::string_view>::convert(value, 0) ==
        123);
  }

  SECTION("uses default value for out-of-range numbers") {
    // out-of-range unsigned int
    REQUIRE(
        MetadataConversions<int32_t, uint32_t>::convert(
            std::numeric_limits<uint32_t>::max(),
            0) == 0);
    // out-of-range signed int
    REQUIRE(
        MetadataConversions<int32_t, int64_t>::convert(
            std::numeric_limits<int64_t>::min(),
            0) == 0);
    // out-of-range float
    REQUIRE(MetadataConversions<uint8_t, float>::convert(1234.56f, 0) == 0);
    // out-of-range double
    REQUIRE(
        MetadataConversions<int32_t, double>::convert(
            std::numeric_limits<double>::max(),
            0) == 0);
  }

  SECTION("uses default value for invalid strings") {
    // out-of-range number
    REQUIRE(
        MetadataConversions<int8_t, std::string_view>::convert(
            std::string_view("-255"),
            0) == 0);
    // mixed number and non-number input
    REQUIRE(
        MetadataConversions<int8_t, std::string_view>::convert(
            std::string_view("10 hello"),
            0) == 0);
    // non-number input
    REQUIRE(
        MetadataConversions<int8_t, std::string_view>::convert(
            std::string_view("not a number"),
            0) == 0);
    // empty input
    REQUIRE(
        MetadataConversions<int8_t, std::string_view>::convert(
            std::string_view(),
            0) == 0);
  }

  SECTION("uses default value for incompatible types") {
    // vecN
    REQUIRE(
        MetadataConversions<int32_t, glm::ivec3>::convert(
            glm::ivec3(1, 2, 3),
            0) == 0);
    // matN
    REQUIRE(
        MetadataConversions<int32_t, glm::imat2x2>::convert(
            glm::imat2x2(),
            0) == 0);
    // array
    PropertyArrayView<int32_t> arrayView;
    REQUIRE(
        MetadataConversions<int32_t, PropertyArrayView<int32_t>>::convert(
            arrayView,
            0) == 0);
  }
}

TEST_CASE("Test PropertyConversions for float") {
  SECTION("converts from in-range floating point number") {
    REQUIRE(
        MetadataConversions<float, float>::convert(123.45f, 0.0f) == 123.45f);
    REQUIRE(
        MetadataConversions<float, double>::convert(123.45, 0.0f) ==
        static_cast<float>(123.45));
  }

  SECTION("converts from integer") {
    int32_t int32Value = -1234;
    REQUIRE(
        MetadataConversions<float, int32_t>::convert(int32Value, 0.0f) ==
        static_cast<float>(int32Value));
    constexpr uint64_t uint64Value = std::numeric_limits<uint64_t>::max();
    REQUIRE(
        MetadataConversions<float, uint64_t>::convert(uint64Value, 0.0f) ==
        static_cast<float>(uint64Value));
  }

  SECTION("converts from boolean") {
    REQUIRE(MetadataConversions<float, bool>::convert(true, -1.0f) == 1.0f);
    REQUIRE(MetadataConversions<float, bool>::convert(false, -1.0f) == 0.0f);
  }

  SECTION("converts from string") {
    REQUIRE(
        MetadataConversions<float, std::string_view>::convert(
            std::string_view("123"),
            0) == static_cast<float>(123));
    REQUIRE(
        MetadataConversions<float, std::string_view>::convert(
            std::string_view("123.456"),
            0) == static_cast<float>(123.456));
  }

  SECTION("uses default value for invalid strings") {
    // out-of-range number
    REQUIRE(
        MetadataConversions<float, std::string_view>::convert(
            std::string_view(
                std::to_string(std::numeric_limits<double>::max())),
            0.0f) == 0.0f);
    // mixed number and non-number input
    REQUIRE(
        MetadataConversions<float, std::string_view>::convert(
            std::string_view("10.00f hello"),
            0.0f) == 0.0f);
    // non-number input
    REQUIRE(
        MetadataConversions<float, std::string_view>::convert(
            std::string_view("not a number"),
            0.0f) == 0.0f);
    // empty input
    REQUIRE(
        MetadataConversions<float, std::string_view>::convert(
            std::string_view(),
            0.0f) == 0.0f);
  }

  SECTION("uses default value for incompatible types") {
    // vecN
    REQUIRE(
        MetadataConversions<float, glm::vec3>::convert(
            glm::vec3(1, 2, 3),
            0.0f) == 0.0f);
    // matN
    REQUIRE(
        MetadataConversions<float, glm::mat2>::convert(glm::mat2(), 0.0f) ==
        0.0f);
    // array
    PropertyArrayView<float> arrayView;
    REQUIRE(
        MetadataConversions<float, PropertyArrayView<float>>::convert(
            arrayView,
            0.0f) == 0.0f);
  }
}

TEST_CASE("Test PropertyConversions for double") {
  SECTION("converts from floating point number") {
    REQUIRE(
        MetadataConversions<double, float>::convert(123.45f, 0.0) ==
        static_cast<double>(123.45f));
    REQUIRE(
        MetadataConversions<double, double>::convert(123.45, 0.0) == 123.45);
  }

  SECTION("converts from integer") {
    constexpr uint64_t uint64Value = std::numeric_limits<uint64_t>::max();
    REQUIRE(
        MetadataConversions<double, uint64_t>::convert(uint64Value, 0.0) ==
        static_cast<double>(uint64Value));
  }

  SECTION("converts from boolean") {
    REQUIRE(MetadataConversions<double, bool>::convert(true, -1.0) == 1.0);
    REQUIRE(MetadataConversions<double, bool>::convert(false, -1.0) == 0.0);
  }

  SECTION("converts from string") {
    REQUIRE(
        MetadataConversions<double, std::string_view>::convert(
            std::string_view("123"),
            0) == static_cast<double>(123));
    REQUIRE(
        MetadataConversions<double, std::string_view>::convert(
            std::string_view("123.456"),
            0) == 123.456);
  }

  SECTION("uses default value for invalid strings") {
    // mixed number and non-number input
    REQUIRE(
        MetadataConversions<double, std::string_view>::convert(
            std::string_view("10.00 hello"),
            0.0) == 0.0);
    // non-number input
    REQUIRE(
        MetadataConversions<double, std::string_view>::convert(
            std::string_view("not a number"),
            0.0) == 0.0);
    // empty input
    REQUIRE(
        MetadataConversions<double, std::string_view>::convert(
            std::string_view(),
            0.0) == 0.0);
  }

  SECTION("uses default value for incompatible types") {
    // vecN
    REQUIRE(
        MetadataConversions<double, glm::dvec3>::convert(
            glm::dvec3(1, 2, 3),
            0.0) == 0.0);
    // matN
    REQUIRE(
        MetadataConversions<double, glm::dmat2>::convert(
            glm::dmat2(1.0, 2.0, 3.0, 4.0),
            0.0) == 0.0);
    // array
    PropertyArrayView<double> arrayView;
    REQUIRE(
        MetadataConversions<double, PropertyArrayView<double>>::convert(
            arrayView,
            0.0) == 0.0);
  }
}

TEST_CASE("Test PropertyConversions for vec2") {
  SECTION("converts from same vec2 type") {
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::ivec2, glm::ivec2>::convert(
            glm::ivec2(12, 76),
            glm::ivec2(0)) == glm::ivec2(12, 76));
    // float-to-float
    REQUIRE(
        MetadataConversions<glm::vec2, glm::vec2>::convert(
            glm::vec2(-3.5f, 1.234f),
            glm::vec2(0.0)) == glm::vec2(-3.5f, 1.234f));
  }

  SECTION("converts from other vec2 types") {
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::ivec2, glm::u8vec2>::convert(
            glm::u8vec2(12, 76),
            glm::ivec2(0)) == glm::ivec2(12, 76));
    // int-to-float
    REQUIRE(
        MetadataConversions<glm::vec2, glm::ivec2>::convert(
            glm::ivec2(12, 76),
            glm::vec2(0)) == glm::vec2(12, 76));
    // float-to-int
    REQUIRE(
        MetadataConversions<glm::i8vec2, glm::dvec2>::convert(
            glm::dvec2(-3.5, 1.23456),
            glm::i8vec2(0)) == glm::i8vec2(-3, 1));
    // float-to-float
    REQUIRE(
        MetadataConversions<glm::dvec2, glm::vec2>::convert(
            glm::vec2(-3.5f, 1.234f),
            glm::dvec2(0.0)) == glm::dvec2(-3.5f, 1.234f));
  }

  SECTION("converts from vec3 types") {
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::i8vec2, glm::ivec3>::convert(
            glm::ivec3(-84, 5, 129),
            glm::i8vec2(0)) == glm::i8vec2(-84, 5));
    // int-to-float
    REQUIRE(
        MetadataConversions<glm::dvec2, glm::ivec3>::convert(
            glm::ivec3(-84, 5, 25),
            glm::dvec2(0.0)) == glm::dvec2(-84, 5));
    // float-to-int
    REQUIRE(
        MetadataConversions<glm::uvec2, glm::vec3>::convert(
            glm::vec3(4.5f, 2.345f, 81.0f),
            glm::uvec2(0)) == glm::uvec2(4, 2));
    // float-to-float
    REQUIRE(
        MetadataConversions<glm::vec2, glm::dvec3>::convert(
            glm::dvec3(4.5, -2.345, 81.0),
            glm::vec2(0)) ==
        glm::vec2(static_cast<float>(4.5), static_cast<float>(-2.345)));
  }

  SECTION("converts from vec4 types") {
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::ivec2, glm::i16vec4>::convert(
            glm::i16vec4(-42, 278, 23, 1),
            glm::ivec2(0)) == glm::ivec2(-42, 278));
    // float-to-int
    REQUIRE(
        MetadataConversions<glm::i8vec2, glm::dvec4>::convert(
            glm::dvec4(-3.5, 1.23456, 26.0, 8.0),
            glm::i8vec2(0)) == glm::i8vec2(-3, 1));
  }

  SECTION("converts from boolean") {
    REQUIRE(
        MetadataConversions<glm::dvec2, bool>::convert(
            true,
            glm::dvec2(-1.0)) == glm ::dvec2(1.0));
  }

  SECTION("converts from integer") {
    // int to int
    REQUIRE(
        MetadataConversions<glm::u8vec2, int32_t>::convert(
            45,
            glm::u8vec2(0)) == glm::u8vec2(45));
    REQUIRE(
        MetadataConversions<glm::i16vec2, uint32_t>::convert(
            45,
            glm::i16vec2(0)) == glm::i16vec2(45));
    // int to float
    REQUIRE(
        MetadataConversions<glm::dvec2, int32_t>::convert(
            -12345,
            glm::dvec2(0)) == glm::dvec2(-12345));
    REQUIRE(
        MetadataConversions<glm::vec2, uint8_t>::convert(12, glm::vec2(0)) ==
        glm::vec2(12));
  }

  SECTION("converts from float") {
    // float to int
    REQUIRE(
        MetadataConversions<glm::u8vec2, float>::convert(
            45.4f,
            glm::u8vec2(0)) == glm::u8vec2(45));
    REQUIRE(
        MetadataConversions<glm::i16vec2, double>::convert(
            -1.0111,
            glm::i16vec2(0)) == glm::i16vec2(-1));
    // float to float
    REQUIRE(
        MetadataConversions<glm::dvec2, float>::convert(
            -1234.5f,
            glm::dvec2(0)) == glm::dvec2(-1234.5f));
    REQUIRE(
        MetadataConversions<glm::vec2, double>::convert(12.0, glm::vec2(0)) ==
        glm::vec2(12.0));
  }

  SECTION("uses default value if not all components can be converted") {
    // scalar
    REQUIRE(
        MetadataConversions<glm::u8vec2, int16_t>::convert(
            -1,
            glm::u8vec2(0)) == glm::u8vec2(0));
    // int
    REQUIRE(
        MetadataConversions<glm::u8vec2, glm::ivec3>::convert(
            glm::ivec3(0, -1, 2),
            glm::u8vec2(0)) == glm::u8vec2(0));
    REQUIRE(
        MetadataConversions<glm::i8vec2, glm::u8vec4>::convert(
            glm::u8vec4(0, 255, 2, 1),
            glm::i8vec2(0)) == glm::i8vec2(0));
    // float
    REQUIRE(
        MetadataConversions<glm::i8vec2, glm::vec2>::convert(
            glm::vec2(129.0f, -129.0f),
            glm::i8vec2(0)) == glm::i8vec2(0));
    REQUIRE(
        MetadataConversions<glm::vec2, glm::dvec3>::convert(
            glm::dvec3(std::numeric_limits<double>::max()),
            glm::vec2(0)) == glm::vec2(0));
  };

  SECTION("uses default value for incompatible types") {
    // matN
    REQUIRE(
        MetadataConversions<glm::dvec2, glm::dmat2>::convert(
            glm::dmat2(1.0, 2.0, 3.0, 4.0),
            glm::dvec2(0.0)) == glm::dvec2(0.0));
    // array
    PropertyArrayView<glm::ivec2> arrayView;
    REQUIRE(
        MetadataConversions<glm::ivec2, PropertyArrayView<glm::ivec2>>::convert(
            arrayView,
            glm::ivec2(0)) == glm::ivec2(0));
  };
}

TEST_CASE("Test PropertyConversions for vec3") {
  SECTION("converts from same vec3 type") {
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::ivec3, glm::ivec3>::convert(
            glm::ivec3(12, 76, -1),
            glm::ivec3(0)) == glm::ivec3(12, 76, -1));
    // float-to-float
    REQUIRE(
        MetadataConversions<glm::vec3, glm::vec3>::convert(
            glm::vec3(-3.5f, 1.234f, 1.0f),
            glm::vec3(0.0)) == glm::vec3(-3.5f, 1.234f, 1.0f));
  }

  SECTION("converts from other vec3 types") {
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::ivec3, glm::u8vec3>::convert(
            glm::u8vec3(12, 76, 1),
            glm::ivec3(0)) == glm::ivec3(12, 76, 1));
    // int-to-float
    REQUIRE(
        MetadataConversions<glm::vec3, glm::ivec3>::convert(
            glm::ivec3(12, 76, 1),
            glm::vec3(0)) == glm::vec3(12, 76, 1));
    // float-to-int
    REQUIRE(
        MetadataConversions<glm::i8vec3, glm::dvec3>::convert(
            glm::dvec3(-3.5, 1.23456, -1.40),
            glm::i8vec3(0)) == glm::i8vec3(-3, 1, -1));
    // float-to-float
    REQUIRE(
        MetadataConversions<glm::dvec3, glm::vec3>::convert(
            glm::vec3(-3.5f, 1.234f, 2.4f),
            glm::dvec3(0.0)) == glm::dvec3(-3.5f, 1.234f, 2.4f));
  }

  SECTION("converts from vec2 types") {
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::i8vec3, glm::ivec2>::convert(
            glm::ivec2(-84, 5),
            glm::i8vec3(0)) == glm::i8vec3(-84, 5, 0));
    // int-to-float
    REQUIRE(
        MetadataConversions<glm::dvec3, glm::ivec2>::convert(
            glm::ivec2(-84, 5),
            glm::dvec3(0.0)) == glm::dvec3(-84, 5, 0));
    // float-to-int
    REQUIRE(
        MetadataConversions<glm::uvec3, glm::vec2>::convert(
            glm::vec2(4.5f, 2.345f),
            glm::uvec3(0)) == glm::uvec3(4, 2, 0));
    // float-to-float
    REQUIRE(
        MetadataConversions<glm::vec3, glm::dvec2>::convert(
            glm::dvec2(4.5, -2.345),
            glm::vec3(0)) == glm::vec3(4.5, -2.345, 0));
  }

  SECTION("converts from vec4 types") {
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::ivec3, glm::i16vec4>::convert(
            glm::i16vec4(-42, 278, 23, 1),
            glm::ivec3(0)) == glm::ivec3(-42, 278, 23));
    // float-to-int
    REQUIRE(
        MetadataConversions<glm::i8vec3, glm::dvec4>::convert(
            glm::dvec4(-3.5, 1.23456, 26.0, 8.0),
            glm::i8vec3(0)) == glm::i8vec3(-3, 1, 26));
  }

  SECTION("converts from boolean") {
    REQUIRE(
        MetadataConversions<glm::dvec3, bool>::convert(
            true,
            glm::dvec3(-1.0)) == glm ::dvec3(1.0));
  }

  SECTION("converts from integer") {
    // int to int
    REQUIRE(
        MetadataConversions<glm::u8vec3, int32_t>::convert(
            45,
            glm::u8vec3(0)) == glm::u8vec3(45));
    REQUIRE(
        MetadataConversions<glm::i16vec3, uint32_t>::convert(
            45,
            glm::i16vec3(0)) == glm::i16vec3(45));
    // int to float
    REQUIRE(
        MetadataConversions<glm::dvec3, int32_t>::convert(
            -12345,
            glm::dvec3(0)) == glm::dvec3(-12345));
    REQUIRE(
        MetadataConversions<glm::vec3, uint8_t>::convert(12, glm::vec3(0)) ==
        glm::vec3(12));
  }

  SECTION("converts from float") {
    // float to int
    REQUIRE(
        MetadataConversions<glm::u8vec3, float>::convert(
            45.4f,
            glm::u8vec3(0)) == glm::u8vec3(45));
    REQUIRE(
        MetadataConversions<glm::i16vec3, double>::convert(
            -1.0111,
            glm::i16vec3(0)) == glm::i16vec3(-1));
    // float to float
    REQUIRE(
        MetadataConversions<glm::dvec3, float>::convert(
            -1234.5f,
            glm::dvec3(0)) == glm::dvec3(-1234.5f));
    REQUIRE(
        MetadataConversions<glm::vec3, double>::convert(12.0, glm::vec3(0)) ==
        glm::vec3(12.0));
  }

  SECTION("uses default value if not all components can be converted") {
    // scalar
    REQUIRE(
        MetadataConversions<glm::u8vec3, int16_t>::convert(
            -1,
            glm::u8vec3(0)) == glm::u8vec3(0));
    // int
    REQUIRE(
        MetadataConversions<glm::u8vec3, glm::ivec3>::convert(
            glm::ivec3(0, -1, 2),
            glm::u8vec3(0)) == glm::u8vec3(0));
    REQUIRE(
        MetadataConversions<glm::i8vec3, glm::u8vec4>::convert(
            glm::u8vec4(0, 255, 2, 1),
            glm::i8vec3(0)) == glm::i8vec3(0));
    // float
    REQUIRE(
        MetadataConversions<glm::i8vec3, glm::vec2>::convert(
            glm::vec2(129.0f, -129.0f),
            glm::i8vec3(0)) == glm::i8vec3(0));
    REQUIRE(
        MetadataConversions<glm::vec3, glm::dvec4>::convert(
            glm::dvec4(std::numeric_limits<double>::max()),
            glm::vec3(0)) == glm::vec3(0));
  };

  SECTION("uses default value for incompatible types") {
    // matN
    REQUIRE(
        MetadataConversions<glm::dvec3, glm::dmat2>::convert(
            glm::dmat2(1.0, 2.0, 3.0, 4.0),
            glm::dvec3(0.0)) == glm::dvec3(0.0));
    // array
    PropertyArrayView<glm::ivec3> arrayView;
    REQUIRE(
        MetadataConversions<glm::ivec3, PropertyArrayView<glm::ivec3>>::convert(
            arrayView,
            glm::ivec3(0)) == glm::ivec3(0));
  };
}
