#include <CesiumGltf/MetadataConversions.h>
#include <CesiumGltf/PropertyArrayView.h>

#include <doctest/doctest.h>
#include <glm/ext/matrix_double2x2.hpp>
#include <glm/ext/matrix_double3x3.hpp>
#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/matrix_float2x2.hpp>
#include <glm/ext/matrix_float3x3.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_double4.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/ext/vector_int2_sized.hpp>
#include <glm/ext/vector_int3.hpp>
#include <glm/ext/vector_int3_sized.hpp>
#include <glm/ext/vector_int4.hpp>
#include <glm/ext/vector_int4_sized.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <glm/ext/vector_uint2_sized.hpp>
#include <glm/ext/vector_uint3.hpp>
#include <glm/ext/vector_uint3_sized.hpp>
#include <glm/ext/vector_uint4.hpp>
#include <glm/ext/vector_uint4_sized.hpp>
#include <glm/fwd.hpp>

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

#ifdef _MSC_VER
// Some tests intentionally use a double value that overflows a float.
// Disable Visual Studio's spurious warning about this:
//  warning C4756: overflow in constant arithmetic
#pragma warning(disable : 4756)
#endif

using namespace CesiumGltf;

void testStringToBooleanConversion(
    const std::string& input,
    std::optional<bool> expected) {
  REQUIRE(MetadataConversions<bool, std::string>::convert(input) == expected);
  REQUIRE(
      MetadataConversions<bool, std::string_view>::convert(
          std::string_view(input)) == expected);
}

template <typename T>
void testStringToScalarConversion(
    const std::string& input,
    std::optional<T> expected) {
  REQUIRE(MetadataConversions<T, std::string>::convert(input) == expected);
  REQUIRE(
      MetadataConversions<T, std::string_view>::convert(
          std::string_view(input)) == expected);
}

TEST_CASE("Test MetadataConversions for boolean") {
  SUBCASE("converts from boolean") {
    REQUIRE(MetadataConversions<bool, bool>::convert(true) == true);
    REQUIRE(MetadataConversions<bool, bool>::convert(false) == false);
  }

  SUBCASE("converts from scalar") {
    // true for nonzero value
    REQUIRE(MetadataConversions<bool, int8_t>::convert(10) == true);
    // false for zero value
    REQUIRE(MetadataConversions<bool, int8_t>::convert(0) == false);
  }

  SUBCASE("converts from string") {
    testStringToBooleanConversion("true", true);
    testStringToBooleanConversion("yes", true);
    testStringToBooleanConversion("1", true);
    testStringToBooleanConversion("false", false);
    testStringToBooleanConversion("no", false);
    testStringToBooleanConversion("0", false);
  }

  SUBCASE("returns std::nullopt for incompatible strings") {
    testStringToBooleanConversion("11", std::nullopt);
    testStringToBooleanConversion("this is true", std::nullopt);
    testStringToBooleanConversion("false!", std::nullopt);
  }

  SUBCASE("returns std::nullopt for incompatible types") {
    // vecN
    REQUIRE(!MetadataConversions<bool, glm::vec3>::convert(glm::vec3(1, 2, 3)));
    // matN
    REQUIRE(!MetadataConversions<bool, glm::mat2>::convert(glm::mat2()));
    // array
    REQUIRE(!MetadataConversions<bool, PropertyArrayView<bool>>::convert(
        PropertyArrayView<bool>()));
  }
}

TEST_CASE("Test MetadataConversions for integer") {
  SUBCASE("converts from in-range integer") {
    // same type
    REQUIRE(MetadataConversions<int32_t, int32_t>::convert(50) == 50);
    // different size
    REQUIRE(MetadataConversions<int32_t, int64_t>::convert(50) == 50);
    // different sign
    REQUIRE(MetadataConversions<int32_t, uint32_t>::convert(50) == 50);
  }

  SUBCASE("converts from in-range floating point number") {
    REQUIRE(MetadataConversions<int32_t, float>::convert(50.125f) == 50);
    REQUIRE(MetadataConversions<int32_t, double>::convert(1234.05678f) == 1234);
  }

  SUBCASE("converts from boolean") {
    REQUIRE(MetadataConversions<int32_t, bool>::convert(true) == 1);
    REQUIRE(MetadataConversions<int32_t, bool>::convert(false) == 0);
  }

  SUBCASE("converts from string") {
    // integer string
    testStringToScalarConversion<int32_t>("-123", -123);
    // double string
    testStringToScalarConversion<int32_t>("123.456", 123);
  }

  SUBCASE("returns std::nullopt for out-of-range numbers") {
    // out-of-range unsigned int
    REQUIRE(!MetadataConversions<int32_t, uint32_t>::convert(
        std::numeric_limits<uint32_t>::max()));
    // out-of-range signed int
    REQUIRE(!MetadataConversions<int32_t, int64_t>::convert(
        std::numeric_limits<int64_t>::min()));
    // out-of-range float
    REQUIRE(!MetadataConversions<uint8_t, float>::convert(1234.56f));
    // out-of-range double
    REQUIRE(!MetadataConversions<int32_t, double>::convert(
        std::numeric_limits<double>::max()));
  }

  SUBCASE("returns std::nullopt for invalid strings") {
    // out-of-range number
    testStringToScalarConversion<uint8_t>("-1", std::nullopt);
    // mixed number and non-number input
    testStringToScalarConversion<int8_t>("10 hello", std::nullopt);
    // non-number input
    testStringToScalarConversion<uint8_t>("not a number", std::nullopt);
    // empty input
    testStringToScalarConversion<int8_t>("", std::nullopt);

    // extra tests for proper string parsing
    testStringToScalarConversion<uint64_t>("-1", std::nullopt);
    testStringToScalarConversion<uint64_t>(
        "184467440737095515000",
        std::nullopt);
    testStringToScalarConversion<int64_t>(
        "-111111111111111111111111111111111",
        std::nullopt);
    testStringToScalarConversion<int64_t>(
        "111111111111111111111111111111111",
        std::nullopt);
  }

  SUBCASE("returns std::nullopt for incompatible types") {
    // vecN
    REQUIRE(!MetadataConversions<int32_t, glm::ivec3>::convert(
        glm::ivec3(1, 2, 3)));
    // matN
    REQUIRE(
        !MetadataConversions<int32_t, glm::imat2x2>::convert(glm::imat2x2()));
    // array
    PropertyArrayView<int32_t> arrayView;
    REQUIRE(!MetadataConversions<int32_t, PropertyArrayView<int32_t>>::convert(
        arrayView));
  }
}

TEST_CASE("Test MetadataConversions for float") {
  SUBCASE("converts from in-range floating point number") {
    REQUIRE(MetadataConversions<float, float>::convert(123.45f) == 123.45f);
    REQUIRE(
        MetadataConversions<float, double>::convert(123.45) ==
        static_cast<float>(123.45));
  }

  SUBCASE("converts from integer") {
    int32_t int32Value = -1234;
    REQUIRE(
        MetadataConversions<float, int32_t>::convert(int32Value) ==
        static_cast<float>(int32Value));
    constexpr uint64_t uint64Value = std::numeric_limits<uint64_t>::max();
    REQUIRE(
        MetadataConversions<float, uint64_t>::convert(uint64Value) ==
        static_cast<float>(uint64Value));
  }

  SUBCASE("converts from boolean") {
    REQUIRE(MetadataConversions<float, bool>::convert(true) == 1.0f);
    REQUIRE(MetadataConversions<float, bool>::convert(false) == 0.0f);
  }

  SUBCASE("converts from string") {
    testStringToScalarConversion<float>("123", static_cast<float>(123));
    testStringToScalarConversion<float>("123.456", static_cast<float>(123.456));
  }

  SUBCASE("returns std::nullopt for invalid strings") {
    // out-of-range number
    testStringToScalarConversion<float>(
        std::to_string(std::numeric_limits<double>::max()),
        std::nullopt);
    // mixed number and non-number input
    testStringToScalarConversion<float>("10.00f hello", std::nullopt);
    // non-number input
    testStringToScalarConversion<float>("not a number", std::nullopt);
    // empty input
    testStringToScalarConversion<float>("", std::nullopt);
  }

  SUBCASE("returns std::nullopt for incompatible types") {
    // vecN
    REQUIRE(
        !MetadataConversions<float, glm::vec3>::convert(glm::vec3(1, 2, 3)));
    // matN
    REQUIRE(!MetadataConversions<float, glm::mat2>::convert(glm::mat2()));
    // array
    PropertyArrayView<float> arrayView;
    REQUIRE(!MetadataConversions<float, PropertyArrayView<float>>::convert(
        arrayView));
  }
}

TEST_CASE("Test MetadataConversions for double") {
  SUBCASE("converts from floating point number") {
    REQUIRE(
        MetadataConversions<double, float>::convert(123.45f) ==
        static_cast<double>(123.45f));
    REQUIRE(MetadataConversions<double, double>::convert(123.45) == 123.45);
  }

  SUBCASE("converts from integer") {
    constexpr uint64_t uint64Value = std::numeric_limits<uint64_t>::max();
    REQUIRE(
        MetadataConversions<double, uint64_t>::convert(uint64Value) ==
        static_cast<double>(uint64Value));
  }

  SUBCASE("converts from boolean") {
    REQUIRE(MetadataConversions<double, bool>::convert(true) == 1.0);
    REQUIRE(MetadataConversions<double, bool>::convert(false) == 0.0);
  }

  SUBCASE("converts from string") {
    testStringToScalarConversion<double>("123", static_cast<double>(123));
    testStringToScalarConversion<double>("123.456", 123.456);
  }

  SUBCASE("returns std::nullopt for invalid strings") {
    // mixed number and non-number input
    testStringToScalarConversion<double>("10.00 hello", std::nullopt);
    // non-number input
    testStringToScalarConversion<double>("not a number", std::nullopt);
    // empty input
    testStringToScalarConversion<double>("", std::nullopt);
  }

  SUBCASE("returns std::nullopt for incompatible types") {
    // vecN
    REQUIRE(
        !MetadataConversions<double, glm::dvec3>::convert(glm::dvec3(1, 2, 3)));
    // matN
    REQUIRE(!MetadataConversions<double, glm::dmat2>::convert(
        glm::dmat2(1.0, 2.0, 3.0, 4.0)));
    // array
    PropertyArrayView<double> arrayView;
    REQUIRE(!MetadataConversions<double, PropertyArrayView<double>>::convert(
        arrayView));
  }
}

TEST_CASE("Test MetadataConversions for vec2") {
  SUBCASE("converts from same vec2 type") {
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::ivec2, glm::ivec2>::convert(
            glm::ivec2(12, 76)) == glm::ivec2(12, 76));
    // float-to-float
    REQUIRE(
        MetadataConversions<glm::vec2, glm::vec2>::convert(
            glm::vec2(-3.5f, 1.234f)) == glm::vec2(-3.5f, 1.234f));
  }

  SUBCASE("converts from other vec2 types") {
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::ivec2, glm::u8vec2>::convert(
            glm::u8vec2(12, 76)) == glm::ivec2(12, 76));
    // int-to-float
    REQUIRE(
        MetadataConversions<glm::vec2, glm::ivec2>::convert(
            glm::ivec2(12, 76)) == glm::vec2(12, 76));
    // float-to-int
    REQUIRE(
        MetadataConversions<glm::i8vec2, glm::dvec2>::convert(
            glm::dvec2(-3.5, 1.23456)) == glm::i8vec2(-3, 1));
    // float-to-float
    REQUIRE(
        MetadataConversions<glm::dvec2, glm::vec2>::convert(
            glm::vec2(-3.5f, 1.234f)) == glm::dvec2(-3.5f, 1.234f));
  }

  SUBCASE("converts from vec3 types") {
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::i8vec2, glm::ivec3>::convert(
            glm::ivec3(-84, 5, 129)) == glm::i8vec2(-84, 5));
    // int-to-float
    REQUIRE(
        MetadataConversions<glm::dvec2, glm::ivec3>::convert(
            glm::ivec3(-84, 5, 25)) == glm::dvec2(-84, 5));
    // float-to-int
    REQUIRE(
        MetadataConversions<glm::uvec2, glm::vec3>::convert(
            glm::vec3(4.5f, 2.345f, 81.0f)) == glm::uvec2(4, 2));
    // float-to-float
    REQUIRE(
        MetadataConversions<glm::vec2, glm::dvec3>::convert(
            glm::dvec3(4.5, -2.345, 81.0)) ==
        glm::vec2(static_cast<float>(4.5), static_cast<float>(-2.345)));
  }

  SUBCASE("converts from vec4 types") {
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::ivec2, glm::i16vec4>::convert(
            glm::i16vec4(-42, 278, 23, 1)) == glm::ivec2(-42, 278));
    // int-to-float
    REQUIRE(
        MetadataConversions<glm::dvec2, glm::ivec4>::convert(
            glm::ivec4(-84, 5, 25, 1)) == glm::dvec2(-84, 5));
    // float-to-int
    REQUIRE(
        MetadataConversions<glm::i8vec2, glm::dvec4>::convert(
            glm::dvec4(-3.5, 1.23456, 26.0, 8.0)) == glm::i8vec2(-3, 1));
    // float-to-float
    REQUIRE(
        MetadataConversions<glm::vec2, glm::dvec4>::convert(
            glm::dvec4(4.5, -2.345, 81.0, 1.0)) ==
        glm::vec2(static_cast<float>(4.5), static_cast<float>(-2.345)));
  }

  SUBCASE("converts from boolean") {
    REQUIRE(
        MetadataConversions<glm::dvec2, bool>::convert(true) ==
        glm ::dvec2(1.0));
  }

  SUBCASE("converts from integer") {
    // int to int
    REQUIRE(
        MetadataConversions<glm::u8vec2, int32_t>::convert(45) ==
        glm::u8vec2(45));
    REQUIRE(
        MetadataConversions<glm::i16vec2, uint32_t>::convert(45) ==
        glm::i16vec2(45));
    // int to float
    REQUIRE(
        MetadataConversions<glm::dvec2, int32_t>::convert(-12345) ==
        glm::dvec2(-12345));
    REQUIRE(
        MetadataConversions<glm::vec2, uint8_t>::convert(12) == glm::vec2(12));
  }

  SUBCASE("converts from float") {
    // float to int
    REQUIRE(
        MetadataConversions<glm::u8vec2, float>::convert(45.4f) ==
        glm::u8vec2(45));
    REQUIRE(
        MetadataConversions<glm::i16vec2, double>::convert(-1.0111) ==
        glm::i16vec2(-1));
    // float to float
    REQUIRE(
        MetadataConversions<glm::dvec2, float>::convert(-1234.5f) ==
        glm::dvec2(-1234.5f));
    REQUIRE(
        MetadataConversions<glm::vec2, double>::convert(12.0) ==
        glm::vec2(12.0));
  }

  SUBCASE("returns std::nullopt if not all components can be converted") {
    // scalar
    REQUIRE(!MetadataConversions<glm::u8vec2, int16_t>::convert(-1));
    // int
    REQUIRE(!MetadataConversions<glm::u8vec2, glm::ivec3>::convert(
        glm::ivec3(0, -1, 2)));
    REQUIRE(!MetadataConversions<glm::i8vec2, glm::u8vec4>::convert(
        glm::u8vec4(0, 255, 2, 1)));
    // float
    REQUIRE(!MetadataConversions<glm::i8vec2, glm::vec2>::convert(
        glm::vec2(129.0f, -129.0f)));
    REQUIRE(!MetadataConversions<glm::vec2, glm::dvec3>::convert(
        glm::dvec3(std::numeric_limits<double>::max())));
  };

  SUBCASE("returns std::nullopt for incompatible types") {
    // matN
    REQUIRE(!MetadataConversions<glm::dvec2, glm::dmat2>::convert(
        glm::dmat2(1.0, 2.0, 3.0, 4.0)));
    // array
    PropertyArrayView<glm::ivec2> arrayView;
    REQUIRE(!MetadataConversions<glm::ivec2, PropertyArrayView<glm::ivec2>>::
                convert(arrayView));
  };
}

TEST_CASE("Test MetadataConversions for vec3") {
  SUBCASE("converts from same vec3 type") {
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::ivec3, glm::ivec3>::convert(
            glm::ivec3(12, 76, -1)) == glm::ivec3(12, 76, -1));
    // float-to-float
    REQUIRE(
        MetadataConversions<glm::vec3, glm::vec3>::convert(
            glm::vec3(-3.5f, 1.234f, 1.0f)) == glm::vec3(-3.5f, 1.234f, 1.0f));
  }

  SUBCASE("converts from other vec3 types") {
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::ivec3, glm::u8vec3>::convert(
            glm::u8vec3(12, 76, 1)) == glm::ivec3(12, 76, 1));
    // int-to-float
    REQUIRE(
        MetadataConversions<glm::vec3, glm::ivec3>::convert(
            glm::ivec3(12, 76, 1)) == glm::vec3(12, 76, 1));
    // float-to-int
    REQUIRE(
        MetadataConversions<glm::i8vec3, glm::dvec3>::convert(
            glm::dvec3(-3.5, 1.23456, -1.40)) == glm::i8vec3(-3, 1, -1));
    // float-to-float
    REQUIRE(
        MetadataConversions<glm::dvec3, glm::vec3>::convert(
            glm::vec3(-3.5f, 1.234f, 2.4f)) == glm::dvec3(-3.5f, 1.234f, 2.4f));
  }

  SUBCASE("converts from vec2 types") {
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::i8vec3, glm::ivec2>::convert(
            glm::ivec2(-84, 5)) == glm::i8vec3(-84, 5, 0));
    // int-to-float
    REQUIRE(
        MetadataConversions<glm::dvec3, glm::ivec2>::convert(
            glm::ivec2(-84, 5)) == glm::dvec3(-84, 5, 0));
    // float-to-int
    REQUIRE(
        MetadataConversions<glm::uvec3, glm::vec2>::convert(
            glm::vec2(4.5f, 2.345f)) == glm::uvec3(4, 2, 0));
    // float-to-float
    REQUIRE(
        MetadataConversions<glm::vec3, glm::dvec2>::convert(
            glm::dvec2(4.5, -2.345)) == glm::vec3(4.5, -2.345, 0));
  }

  SUBCASE("converts from vec4 types") {
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::ivec3, glm::i16vec4>::convert(
            glm::i16vec4(-42, 278, 23, 1)) == glm::ivec3(-42, 278, 23));
    // int-to-float
    REQUIRE(
        MetadataConversions<glm::dvec3, glm::ivec4>::convert(
            glm::ivec4(-84, 5, 10, 23)) == glm::dvec3(-84, 5, 10));
    // float-to-int
    REQUIRE(
        MetadataConversions<glm::i8vec3, glm::dvec4>::convert(
            glm::dvec4(-3.5, 1.23456, 26.0, 8.0)) == glm::i8vec3(-3, 1, 26));
    // float-to-float
    REQUIRE(
        MetadataConversions<glm::vec3, glm::dvec4>::convert(
            glm::dvec4(4.5, -2.345, 102.3, 1)) ==
        glm::vec3(4.5, -2.345, 102.3));
  }

  SUBCASE("converts from boolean") {
    REQUIRE(
        MetadataConversions<glm::dvec3, bool>::convert(true) ==
        glm ::dvec3(1.0));
  }

  SUBCASE("converts from integer") {
    // int to int
    REQUIRE(
        MetadataConversions<glm::u8vec3, int32_t>::convert(45) ==
        glm::u8vec3(45));
    REQUIRE(
        MetadataConversions<glm::i16vec3, uint32_t>::convert(45) ==
        glm::i16vec3(45));
    // int to float
    REQUIRE(
        MetadataConversions<glm::dvec3, int32_t>::convert(-12345) ==
        glm::dvec3(-12345));
    REQUIRE(
        MetadataConversions<glm::vec3, uint8_t>::convert(12) == glm::vec3(12));
  }

  SUBCASE("converts from float") {
    // float to int
    REQUIRE(
        MetadataConversions<glm::u8vec3, float>::convert(45.4f) ==
        glm::u8vec3(45));
    REQUIRE(
        MetadataConversions<glm::i16vec3, double>::convert(-1.0111) ==
        glm::i16vec3(-1));
    // float to float
    REQUIRE(
        MetadataConversions<glm::dvec3, float>::convert(-1234.5f) ==
        glm::dvec3(-1234.5f));
    REQUIRE(
        MetadataConversions<glm::vec3, double>::convert(12.0) ==
        glm::vec3(12.0));
  }

  SUBCASE("returns std::nullopt if not all components can be converted") {
    // scalar
    REQUIRE(!MetadataConversions<glm::u8vec3, int16_t>::convert(-1));
    // int
    REQUIRE(!MetadataConversions<glm::u8vec3, glm::ivec3>::convert(
        glm::ivec3(0, -1, 2)));
    REQUIRE(!MetadataConversions<glm::i8vec3, glm::u8vec4>::convert(
        glm::u8vec4(0, 255, 2, 1)));
    // float
    REQUIRE(!MetadataConversions<glm::i8vec3, glm::vec2>::convert(
        glm::vec2(129.0f, -129.0f)));
    REQUIRE(!MetadataConversions<glm::vec3, glm::dvec4>::convert(
        glm::dvec4(std::numeric_limits<double>::max())));
  };

  SUBCASE("returns std::nullopt for incompatible types") {
    // matN
    REQUIRE(!MetadataConversions<glm::dvec3, glm::dmat2>::convert(
        glm::dmat2(1.0, 2.0, 3.0, 4.0)));
    // array
    PropertyArrayView<glm::ivec3> arrayView;
    REQUIRE(!MetadataConversions<glm::ivec3, PropertyArrayView<glm::ivec3>>::
                convert(arrayView));
  };
}

TEST_CASE("Test MetadataConversions for vec4") {
  SUBCASE("converts from same vec4 type") {
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::ivec4, glm::ivec4>::convert(
            glm::ivec4(12, 76, -1, 1)) == glm::ivec4(12, 76, -1, 1));
    // float-to-float
    REQUIRE(
        MetadataConversions<glm::vec4, glm::vec4>::convert(
            glm::vec4(-3.5f, 1.234f, 1.0f, 1.0f)) ==
        glm::vec4(-3.5f, 1.234f, 1.0f, 1.0f));
  }

  SUBCASE("converts from other vec4 types") {
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::ivec4, glm::u8vec4>::convert(
            glm::u8vec4(12, 76, 1, 1)) == glm::ivec4(12, 76, 1, 1));
    // int-to-float
    REQUIRE(
        MetadataConversions<glm::vec4, glm::ivec4>::convert(
            glm::ivec4(12, 76, 1, 1)) == glm::vec4(12, 76, 1, 1));
    // float-to-int
    REQUIRE(
        MetadataConversions<glm::i8vec4, glm::dvec4>::convert(
            glm::dvec4(-3.5, 1.23456, -1.40, 1.0)) ==
        glm::i8vec4(-3, 1, -1, 1));
    // float-to-float
    REQUIRE(
        MetadataConversions<glm::dvec4, glm::vec4>::convert(
            glm::vec4(-3.5f, 1.234f, 2.4f, 1.0f)) ==
        glm::dvec4(-3.5f, 1.234f, 2.4f, 1.0f));
  }

  SUBCASE("converts from vec2 types") {
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::i8vec4, glm::ivec2>::convert(
            glm::ivec2(-84, 5)) == glm::i8vec4(-84, 5, 0, 0));
    // int-to-float
    REQUIRE(
        MetadataConversions<glm::dvec4, glm::ivec2>::convert(
            glm::ivec2(-84, 5)) == glm::dvec4(-84, 5, 0, 0));
    // float-to-int
    REQUIRE(
        MetadataConversions<glm::uvec4, glm::vec2>::convert(
            glm::vec2(4.5f, 2.345f)) == glm::uvec4(4, 2, 0, 0));
    // float-to-float
    REQUIRE(
        MetadataConversions<glm::vec4, glm::dvec2>::convert(
            glm::dvec2(4.5, -2.345)) == glm::vec4(4.5, -2.345, 0, 0));
  }

  SUBCASE("converts from vec3 types") {
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::ivec4, glm::i16vec3>::convert(
            glm::i16vec3(-42, 278, 23)) == glm::ivec4(-42, 278, 23, 0));
    // int-to-float
    REQUIRE(
        MetadataConversions<glm::dvec4, glm::ivec3>::convert(
            glm::ivec3(-84, 5, 1)) == glm::dvec4(-84, 5, 1, 0));
    // float-to-int
    REQUIRE(
        MetadataConversions<glm::i8vec4, glm::dvec3>::convert(
            glm::dvec3(-3.5, 1.23456, 26.0)) == glm::i8vec4(-3, 1, 26, 0));
    // float-to-float
    REQUIRE(
        MetadataConversions<glm::vec4, glm::dvec3>::convert(
            glm::dvec3(4.5, -2.345, 12.0)) == glm::vec4(4.5, -2.345, 12.0, 0));
  }

  SUBCASE("converts from boolean") {
    REQUIRE(
        MetadataConversions<glm::dvec4, bool>::convert(true) ==
        glm ::dvec4(1.0));
  }

  SUBCASE("converts from integer") {
    // int to int
    REQUIRE(
        MetadataConversions<glm::u8vec4, int32_t>::convert(45) ==
        glm::u8vec4(45));
    REQUIRE(
        MetadataConversions<glm::i16vec4, uint32_t>::convert(45) ==
        glm::i16vec4(45));
    // int to float
    REQUIRE(
        MetadataConversions<glm::dvec4, int32_t>::convert(-12345) ==
        glm::dvec4(-12345));
    REQUIRE(
        MetadataConversions<glm::vec4, uint8_t>::convert(12) == glm::vec4(12));
  }

  SUBCASE("converts from float") {
    // float to int
    REQUIRE(
        MetadataConversions<glm::u8vec4, float>::convert(45.4f) ==
        glm::u8vec4(45));
    REQUIRE(
        MetadataConversions<glm::i16vec4, double>::convert(-1.0111) ==
        glm::i16vec4(-1));
    // float to float
    REQUIRE(
        MetadataConversions<glm::dvec4, float>::convert(-1234.5f) ==
        glm::dvec4(-1234.5f));
    REQUIRE(
        MetadataConversions<glm::vec4, double>::convert(12.0) ==
        glm::vec4(12.0));
  }

  SUBCASE("returns std::nullopt if not all components can be converted") {
    // scalar
    REQUIRE(!MetadataConversions<glm::u8vec4, int16_t>::convert(-1));
    // int
    REQUIRE(!MetadataConversions<glm::u8vec4, glm::ivec3>::convert(
        glm::ivec3(0, -1, 2)));
    REQUIRE(!MetadataConversions<glm::i8vec4, glm::u8vec4>::convert(
        glm::u8vec4(0, 255, 2, 1)));
    // float
    REQUIRE(!MetadataConversions<glm::i8vec4, glm::vec2>::convert(
        glm::vec2(129.0f, -129.0f)));
    REQUIRE(!MetadataConversions<glm::vec4, glm::dvec4>::convert(
        glm::dvec4(std::numeric_limits<double>::max())));
  };

  SUBCASE("returns std::nullopt for incompatible types") {
    // matN
    REQUIRE(!MetadataConversions<glm::dvec4, glm::dmat2>::convert(
        glm::dmat2(1.0, 2.0, 3.0, 4.0)));
    // array
    PropertyArrayView<glm::ivec4> arrayView;
    REQUIRE(!MetadataConversions<glm::ivec4, PropertyArrayView<glm::ivec4>>::
                convert(arrayView));
  };
}

TEST_CASE("Test MetadataConversions for mat2") {
  SUBCASE("converts from same mat2 type") {
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::imat2x2, glm::imat2x2>::convert(
            glm::imat2x2(12, 76, -1, 1)) == glm::imat2x2(12, 76, -1, 1));
    // float-to-float
    REQUIRE(
        MetadataConversions<glm::mat2, glm::mat2>::convert(
            glm::mat2(-3.5f, 1.234f, 1.0f, 1.0f)) ==
        glm::mat2(-3.5f, 1.234f, 1.0f, 1.0f));
  }

  SUBCASE("converts from other mat2 types") {
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::imat2x2, glm::u8mat2x2>::convert(
            glm::u8mat2x2(12, 76, 1, 1)) == glm::imat2x2(12, 76, 1, 1));
    // int-to-float
    REQUIRE(
        MetadataConversions<glm::mat2, glm::imat2x2>::convert(
            glm::imat2x2(12, 76, 1, 1)) == glm::mat2(12, 76, 1, 1));
    // float-to-int
    REQUIRE(
        MetadataConversions<glm::i8mat2x2, glm::dmat2>::convert(
            glm::dmat2(-3.5, 1.23456, -1.40, 1.0)) ==
        glm::i8mat2x2(-3, 1, -1, 1));
    // float-to-float
    REQUIRE(
        MetadataConversions<glm::dmat2, glm::mat2>::convert(
            glm::mat2(-3.5f, 1.234f, 2.4f, 1.0f)) ==
        glm::dmat2(-3.5f, 1.234f, 2.4f, 1.0f));
  }

  SUBCASE("converts from mat3 types") {
    // clang-format off
    glm::imat3x3 imat3x3(
      1, 2, 3,
      4, 5, 6,
      7, 8, 9);
    // clang-format on
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::i8mat2x2, glm::imat3x3>::convert(imat3x3) ==
        glm::i8mat2x2(1, 2, 4, 5));

    // int-to-float
    REQUIRE(
        MetadataConversions<glm::dmat2, glm::imat3x3>::convert(imat3x3) ==
        glm::dmat2(1, 2, 4, 5));

    // clang-format off
    glm::mat3 mat3(
      1.0f, 2.5f, 3.0f,
      4.5f, 5.0f, 6.0f,
      -7.8f, 8.9f, -9.99f
    );
    // clang-format on
    // float-to-int
    REQUIRE(
        MetadataConversions<glm::u8mat2x2, glm::mat3>::convert(mat3) ==
        glm::u8mat2x2(1, 2, 4, 5));
    // float-to-float
    REQUIRE(
        MetadataConversions<glm::dmat2, glm::mat3>::convert(mat3) ==
        glm::dmat2(mat3[0][0], mat3[0][1], mat3[1][0], mat3[1][1]));
  }

  SUBCASE("converts from mat4 types") {
    // clang-format off
    glm::imat4x4 imat4x4(
      1, 2, 3, 0,
      4, 5, 6, 0,
      7, 8, 9, 0,
      0, 0, 0, 1);
    // clang-format on
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::u8mat2x2, glm::imat4x4>::convert(imat4x4) ==
        glm::u8mat2x2(1, 2, 4, 5));
    // int-to-float
    REQUIRE(
        MetadataConversions<glm::dmat2, glm::imat4x4>::convert(imat4x4) ==
        glm::dmat2(1, 2, 4, 5));

    // clang-format off
    glm::dmat4 dmat4(
      1.0, 2.5, 3.0, 0.0,
      4.5, 5.0, 6.0, 0.0,
      -7.8, 8.9, -9.99, 0.0,
      0.0, 0.0, 0.0, 1.0
    );
    // clang-format on
    // float-to-int
    REQUIRE(
        MetadataConversions<glm::i8mat2x2, glm::dmat4>::convert(dmat4) ==
        glm::i8mat2x2(1, 2, 4, 5));
    // float-to-float
    REQUIRE(
        MetadataConversions<glm::mat2, glm::dmat4>::convert(dmat4) ==
        glm::mat2(1.0, 2.5, 4.5, 5.0));
  }

  SUBCASE("converts from boolean") {
    REQUIRE(
        MetadataConversions<glm::dmat2, bool>::convert(true) ==
        glm ::dmat2(1.0));
  }

  SUBCASE("converts from integer") {
    // int to int
    REQUIRE(
        MetadataConversions<glm::u16mat2x2, int32_t>::convert(45) ==
        glm::u16mat2x2(45));
    REQUIRE(
        MetadataConversions<glm::i64mat2x2, uint32_t>::convert(45) ==
        glm::i64mat2x2(45));
    // int to float
    REQUIRE(
        MetadataConversions<glm::dmat2, int32_t>::convert(-12345) ==
        glm::dmat2(-12345));
    REQUIRE(
        MetadataConversions<glm::mat2, uint8_t>::convert(12) == glm::mat2(12));
  }

  SUBCASE("converts from float") {
    // float to int
    REQUIRE(
        MetadataConversions<glm::u8mat2x2, float>::convert(45.4f) ==
        glm::u8mat2x2(45));
    REQUIRE(
        MetadataConversions<glm::i16mat2x2, double>::convert(-1.0111) ==
        glm::i16mat2x2(-1));
    // float to float
    REQUIRE(
        MetadataConversions<glm::dmat2, float>::convert(-1234.5f) ==
        glm::dmat2(-1234.5f));
    REQUIRE(
        MetadataConversions<glm::mat2, double>::convert(12.0) ==
        glm::mat2(12.0));
  }

  SUBCASE("returns std::nullopt if not all components can be converted") {
    // scalar
    REQUIRE(!MetadataConversions<glm::u8mat2x2, int16_t>::convert(-1));
    // int
    REQUIRE(!MetadataConversions<glm::u16mat2x2, glm::imat2x2>::convert(
        glm::imat2x2(0, -1, 2, 1)));
    REQUIRE(!MetadataConversions<glm::i8mat2x2, glm::u8mat2x2>::convert(
        glm::u8mat2x2(0, 255, 2, 1)));
    // float
    REQUIRE(!MetadataConversions<glm::i8mat2x2, glm::mat2>::convert(
        glm::mat2(129.0f)));
    REQUIRE(!MetadataConversions<glm::mat2, glm::dmat2>::convert(
        glm::dmat2(std::numeric_limits<double>::max())));
  };

  SUBCASE("returns std::nullopt for incompatible types") {
    // vecN
    REQUIRE(!MetadataConversions<glm::dmat2, glm::dvec4>::convert(
        glm::dvec4(1.0, 2.0, 3.0, 4.0)));
    // array
    PropertyArrayView<glm::mat2> arrayView;
    REQUIRE(
        !MetadataConversions<glm::mat2, PropertyArrayView<glm::mat2>>::convert(
            arrayView));
  };
}

TEST_CASE("Test MetadataConversions for mat3") {
  SUBCASE("converts from same mat3 type") {
    // clang-format off
    glm::imat3x3 imat3x3(
      0, 1, 2,
      3, 4, 5,
      6, 7, 8
    );
    // clang-format on
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::imat3x3, glm::imat3x3>::convert(imat3x3) ==
        imat3x3);

    // clang-format off
    glm::mat3 mat3(
      1.0f, 2.4f, 3.0f,
      4.0f, 5.55f, 6.0f,
      -7.0f, 8.0f, -9.01f
    );
    // clang-format on
    // float-to-float
    REQUIRE(MetadataConversions<glm::mat3, glm::mat3>::convert(mat3) == mat3);
  }

  SUBCASE("converts from other mat3 types") {
    // clang-format off
    glm::u8mat3x3 u8mat3x3(
      0, 1, 2,
      3, 4, 5,
      6, 7, 8
    );
    // clang-format on
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::i8mat3x3, glm::u8mat3x3>::convert(u8mat3x3) ==
        glm::i8mat3x3(0, 1, 2, 3, 4, 5, 6, 7, 8));
    // int-to-float
    REQUIRE(
        MetadataConversions<glm::mat3, glm::u8mat3x3>::convert(u8mat3x3) ==
        glm::mat3(0, 1, 2, 3, 4, 5, 6, 7, 8));

    // clang-format off
    glm::mat3 mat3(
      1.0f, 2.4f, 3.0f,
      4.0f, -5.0f, 6.0f,
      7.7f, 8.01f, -9.3f
    );
    // clang-format on
    // float-to-int
    REQUIRE(
        MetadataConversions<glm::i8mat3x3, glm::mat3>::convert(mat3) ==
        glm::i8mat3x3(1, 2, 3, 4, -5, 6, 7, 8, -9));
    // float-to-float
    REQUIRE(
        MetadataConversions<glm::dmat3, glm::mat3>::convert(mat3) ==
        glm::dmat3(mat3[0], mat3[1], mat3[2]));
  }

  SUBCASE("converts from mat2 types") {
    // clang-format off
    glm::imat2x2 imat2x2(
      1, 2,
      3, 4);
    glm::u8mat3x3 expectedIntMat(
      1, 2, 0,
      3, 4, 0,
      0, 0, 0);
    // clang-format on
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::u8mat3x3, glm::imat2x2>::convert(imat2x2) ==
        expectedIntMat);

    // int-to-float
    REQUIRE(
        MetadataConversions<glm::dmat3, glm::imat2x2>::convert(imat2x2) ==
        glm::dmat3(expectedIntMat[0], expectedIntMat[1], expectedIntMat[2]));

    // clang-format off
    glm::mat2 mat2(
      1.0f, 2.5f,
      3.0f, 4.5f
    );
    glm::dmat3 expectedDoubleMat(
      1.0f, 2.5f, 0,
      3.0f, 4.5f, 0,
      0, 0, 0
    );
    // clang-format on
    // float-to-int
    REQUIRE(
        MetadataConversions<glm::u8mat3x3, glm::mat2>::convert(mat2) ==
        expectedIntMat);
    // float-to-float
    REQUIRE(
        MetadataConversions<glm::dmat3, glm::mat2>::convert(mat2) ==
        expectedDoubleMat);
  }

  SUBCASE("converts from mat4 types") {
    // clang-format off
    glm::imat4x4 imat4x4(
      1, 2, 3, 4,
      4, 5, 6, 7,
      7, 8, 9, 10,
      0, 0, 0, 1);
    glm::u8mat3x3 expectedIntMat(
      1, 2, 3,
      4, 5, 6,
      7, 8, 9);
    // clang-format on
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::u8mat3x3, glm::imat4x4>::convert(imat4x4) ==
        expectedIntMat);
    // int-to-float
    REQUIRE(
        MetadataConversions<glm::dmat3, glm::imat4x4>::convert(imat4x4) ==
        glm::dmat3(expectedIntMat[0], expectedIntMat[1], expectedIntMat[2]));

    // clang-format off
    glm::mat4 mat4(
      1.0f, 2.5f, 3.0f, -4.0f,
      4.5f, 5.0f, 6.0f, 7.0f,
      7.8f, 8.9f, 9.99f, 10.1f,
      0.0f, 0.0f, 0.0f, 1.0f
    );
    glm::dmat3 expectedDoubleMat(
      1.0f, 2.5f, 3.0f,
      4.5f, 5.0f, 6.0f,
      7.8f, 8.9f, 9.99f
    );
    // clang-format on
    // float-to-int
    REQUIRE(
        MetadataConversions<glm::u8mat3x3, glm::mat4>::convert(mat4) ==
        expectedIntMat);
    // float-to-float
    REQUIRE(
        MetadataConversions<glm::dmat3, glm::mat4>::convert(mat4) ==
        expectedDoubleMat);
  }

  SUBCASE("converts from boolean") {
    REQUIRE(
        MetadataConversions<glm::dmat3, bool>::convert(true) ==
        glm ::dmat3(1.0));
  }

  SUBCASE("converts from integer") {
    // int to int
    REQUIRE(
        MetadataConversions<glm::u16mat3x3, int32_t>::convert(45) ==
        glm::u16mat3x3(45));
    REQUIRE(
        MetadataConversions<glm::i64mat3x3, uint32_t>::convert(45) ==
        glm::i64mat3x3(45));
    // int to float
    REQUIRE(
        MetadataConversions<glm::dmat3, int32_t>::convert(-12345) ==
        glm::dmat3(-12345));
    REQUIRE(
        MetadataConversions<glm::mat3, uint8_t>::convert(12) == glm::mat3(12));
  }

  SUBCASE("converts from float") {
    // float to int
    REQUIRE(
        MetadataConversions<glm::u8mat3x3, float>::convert(45.4f) ==
        glm::u8mat3x3(45));
    REQUIRE(
        MetadataConversions<glm::i16mat3x3, double>::convert(-1.0111) ==
        glm::i16mat3x3(-1));
    // float to float
    REQUIRE(
        MetadataConversions<glm::dmat3, float>::convert(-1234.5f) ==
        glm::dmat3(-1234.5f));
    REQUIRE(
        MetadataConversions<glm::mat3, double>::convert(12.0) ==
        glm::mat3(12.0));
  }

  SUBCASE("returns std::nullopt if not all components can be converted") {
    // scalar
    REQUIRE(!MetadataConversions<glm::u8mat3x3, int16_t>::convert(-1));
    // int
    REQUIRE(!MetadataConversions<glm::u16mat3x3, glm::imat2x2>::convert(
        glm::imat2x2(0, -1, 2, 1)));
    REQUIRE(!MetadataConversions<glm::i8mat3x3, glm::u8mat2x2>::convert(
        glm::u8mat2x2(0, 255, 2, 1)));
    // float
    REQUIRE(!MetadataConversions<glm::i8mat3x3, glm::mat2>::convert(
        glm::mat2(129.0f)));
    REQUIRE(!MetadataConversions<glm::mat3, glm::dmat2>::convert(
        glm::dmat2(std::numeric_limits<double>::max())));
  };

  SUBCASE("returns std::nullopt for incompatible types") {
    // vecN
    REQUIRE(!MetadataConversions<glm::dmat3, glm::dvec3>::convert(
        glm::dvec3(1.0, 2.0, 3.0)));
    // array
    PropertyArrayView<glm::mat3> arrayView;
    REQUIRE(
        !MetadataConversions<glm::mat3, PropertyArrayView<glm::mat3>>::convert(
            arrayView));
  };
}

TEST_CASE("Test MetadataConversions for mat4") {
  SUBCASE("converts from same mat4 type") {
    // clang-format off
    glm::imat4x4 imat4x4(
      0, 1, 2, 3,
      4, 5, 6, 7,
      8, 9, -1, 1,
      0, 0, 0, 1
    );
    // clang-format on
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::imat4x4, glm::imat4x4>::convert(imat4x4) ==
        imat4x4);

    // clang-format off
    glm::mat4 mat4(
      1.0f, 2.4f, 3.0f, 0.0f,
      4.0f, 5.55f, 6.0f, 0.0f,
      -7.0f, 8.0f, -9.01f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f
    );
    // clang-format on
    // float-to-float
    REQUIRE(MetadataConversions<glm::mat4, glm::mat4>::convert(mat4) == mat4);
  }

  SUBCASE("converts from other mat4 types") {
    // clang-format off
    glm::u8mat4x4 u8mat4x4(
      0, 1, 2, 0,
      3, 4, 5, 0,
      6, 7, 8, 0,
      0, 0, 0, 1
    );
    // clang-format on
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::i8mat4x4, glm::u8mat4x4>::convert(u8mat4x4) ==
        glm::i8mat4x4(u8mat4x4[0], u8mat4x4[1], u8mat4x4[2], u8mat4x4[3]));
    // int-to-float
    REQUIRE(
        MetadataConversions<glm::mat4, glm::u8mat4x4>::convert(u8mat4x4) ==
        glm::mat4(u8mat4x4[0], u8mat4x4[1], u8mat4x4[2], u8mat4x4[3]));

    // clang-format off
    glm::mat4 mat4(
      1.0f, 2.4f, 3.0f, 0.0f,
      4.0f, -5.0f, 6.0f, 0.0f,
      7.7f, 8.01f, -9.3f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f
    );
    glm::i8mat4x4 expected(
      1, 2, 3, 0,
      4, -5, 6, 0,
      7, 8, -9, 0,
      0, 0, 0, 1
    );
    // clang-format on
    // float-to-int
    REQUIRE(
        MetadataConversions<glm::i8mat4x4, glm::mat4>::convert(mat4) ==
        expected);
    // float-to-float
    REQUIRE(
        MetadataConversions<glm::dmat4, glm::mat4>::convert(mat4) ==
        glm::dmat4(mat4[0], mat4[1], mat4[2], mat4[3]));
  }

  SUBCASE("converts from mat2 types") {
    // clang-format off
    glm::imat2x2 imat2x2(
      1, 2,
      3, 4);
    glm::u8mat4x4 expectedIntMat(
      1, 2, 0, 0,
      3, 4, 0, 0,
      0, 0, 0, 0,
      0, 0, 0, 0);
    // clang-format on
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::u8mat4x4, glm::imat2x2>::convert(imat2x2) ==
        expectedIntMat);

    // int-to-float
    REQUIRE(
        MetadataConversions<glm::dmat4, glm::imat2x2>::convert(imat2x2) ==
        glm::dmat4(
            expectedIntMat[0],
            expectedIntMat[1],
            expectedIntMat[2],
            expectedIntMat[3]));

    // clang-format off
    glm::mat2 mat2(
      1.0f, 2.5f,
      3.0f, 4.5f
    );
    glm::dmat4 expectedDoubleMat(
      1.0f, 2.5f, 0, 0,
      3.0f, 4.5f, 0, 0,
      0, 0, 0, 0,
      0, 0, 0, 0
    );
    // clang-format on
    // float-to-int
    REQUIRE(
        MetadataConversions<glm::u8mat4x4, glm::mat2>::convert(mat2) ==
        expectedIntMat);
    // float-to-float
    REQUIRE(
        MetadataConversions<glm::dmat4, glm::mat2>::convert(mat2) ==
        expectedDoubleMat);
  }

  SUBCASE("converts from mat3 types") {
    // clang-format off
    glm::imat3x3 imat3x3(
      1, 2, 3,
      4, 5, 6,
      7, 8, 9);
    glm::u8mat4x4 expectedIntMat(
      1, 2, 3, 0,
      4, 5, 6, 0,
      7, 8, 9, 0,
      0, 0, 0, 0);
    // clang-format on
    // int-to-int
    REQUIRE(
        MetadataConversions<glm::u8mat4x4, glm::imat3x3>::convert(imat3x3) ==
        expectedIntMat);
    // int-to-float
    REQUIRE(
        MetadataConversions<glm::dmat4, glm::imat3x3>::convert(imat3x3) ==
        glm::dmat4(
            expectedIntMat[0],
            expectedIntMat[1],
            expectedIntMat[2],
            expectedIntMat[3]));

    // clang-format off
    glm::mat3 mat3(
      1.0f, 2.5f, 3.0f,
      4.5f, 5.0f, 6.0f,
      7.8f, 8.9f, 9.99f
    );
    glm::dmat4 expectedDoubleMat(
      1.0f, 2.5f, 3.0f, 0.0f,
      4.5f, 5.0f, 6.0f, 0.0f,
      7.8f, 8.9f, 9.99f, 0.0f,
      0.0f, 0.0f, 0.0f, 0.0f
    );
    // clang-format on
    // float-to-int
    REQUIRE(
        MetadataConversions<glm::u8mat4x4, glm::mat3>::convert(mat3) ==
        expectedIntMat);
    // float-to-float
    REQUIRE(
        MetadataConversions<glm::dmat4, glm::mat3>::convert(mat3) ==
        expectedDoubleMat);
  }

  SUBCASE("converts from boolean") {
    REQUIRE(
        MetadataConversions<glm::dmat4, bool>::convert(true) ==
        glm ::dmat4(1.0));
  }

  SUBCASE("converts from integer") {
    // int to int
    REQUIRE(
        MetadataConversions<glm::u16mat4x4, int32_t>::convert(45) ==
        glm::u16mat4x4(45));
    REQUIRE(
        MetadataConversions<glm::i64mat4x4, uint32_t>::convert(45) ==
        glm::i64mat4x4(45));
    // int to float
    REQUIRE(
        MetadataConversions<glm::dmat4, int32_t>::convert(-12345) ==
        glm::dmat4(-12345));
    REQUIRE(
        MetadataConversions<glm::mat4, uint8_t>::convert(12) == glm::mat4(12));
  }

  SUBCASE("converts from float") {
    // float to int
    REQUIRE(
        MetadataConversions<glm::u8mat4x4, float>::convert(45.4f) ==
        glm::u8mat4x4(45));
    REQUIRE(
        MetadataConversions<glm::i16mat4x4, double>::convert(-1.0111) ==
        glm::i16mat4x4(-1));
    // float to float
    REQUIRE(
        MetadataConversions<glm::dmat4, float>::convert(-1234.5f) ==
        glm::dmat4(-1234.5f));
    REQUIRE(
        MetadataConversions<glm::mat4, double>::convert(12.0) ==
        glm::mat4(12.0));
  }

  SUBCASE("returns std::nullopt if not all components can be converted") {
    // scalar
    REQUIRE(!MetadataConversions<glm::u8mat4x4, int16_t>::convert(-1));
    // int
    REQUIRE(!MetadataConversions<glm::u16mat4x4, glm::imat2x2>::convert(
        glm::imat2x2(0, -1, 2, 1)));
    REQUIRE(!MetadataConversions<glm::i8mat4x4, glm::u8mat2x2>::convert(
        glm::u8mat2x2(0, 255, 2, 1)));
    // float
    REQUIRE(!MetadataConversions<glm::i8mat4x4, glm::mat2>::convert(
        glm::mat2(129.0f)));
    REQUIRE(!MetadataConversions<glm::mat4, glm::dmat2>::convert(
        glm::dmat2(std::numeric_limits<double>::max())));
  };

  SUBCASE("returns std::nullopt for incompatible types") {
    // vecN
    REQUIRE(!MetadataConversions<glm::dmat4, glm::dvec4>::convert(
        glm::dvec4(1.0, 2.0, 3.0, 4.0)));
    // array
    PropertyArrayView<glm::mat4> arrayView;
    REQUIRE(
        !MetadataConversions<glm::mat4, PropertyArrayView<glm::mat4>>::convert(
            arrayView));
  };
}

TEST_CASE("Test MetadataConversions for std::string") {
  SUBCASE("converts from std::string_view") {
    std::string str = "Hello";
    REQUIRE(
        MetadataConversions<std::string, std::string_view>::convert(
            std::string_view(str)) == str);
  }

  SUBCASE("converts from boolean") {
    REQUIRE(MetadataConversions<std::string, bool>::convert(true) == "true");
    REQUIRE(MetadataConversions<std::string, bool>::convert(false) == "false");
  }

  SUBCASE("converts from scalar") {
    // integer
    REQUIRE(
        MetadataConversions<std::string, uint16_t>::convert(1234) == "1234");
    // float
    REQUIRE(
        MetadataConversions<std::string, float>::convert(1.2345f) ==
        std::to_string(1.2345f).c_str());
  };
}
