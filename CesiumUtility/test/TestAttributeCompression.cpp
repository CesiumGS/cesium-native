#include <CesiumUtility/AttributeCompression.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_uint2_sized.hpp>
#include <glm/geometric.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace CesiumUtility;

TEST_CASE("AttributeCompression::octDecode") {
  const std::vector<glm::u8vec2> input{
      glm::u8vec2(128, 128),
      glm::u8vec2(255, 255),
      glm::u8vec2(128, 255),
      glm::u8vec2(128, 0),
      glm::u8vec2(255, 128),
      glm::u8vec2(0, 128),
      glm::u8vec2(170, 170),
      glm::u8vec2(170, 85),
      glm::u8vec2(85, 85),
      glm::u8vec2(85, 170),
      glm::u8vec2(213, 213),
      glm::u8vec2(213, 42),
      glm::u8vec2(42, 42),
      glm::u8vec2(42, 213),
  };
  const std::vector<glm::dvec3> expected{
      glm::dvec3(0.0, 0.0, 1.0),
      glm::dvec3(0.0, 0.0, -1.0),
      glm::dvec3(0.0, 1.0, 0.0),
      glm::dvec3(0.0, -1.0, 0.0),
      glm::dvec3(1.0, 0.0, 0.0),
      glm::dvec3(-1.0, 0.0, 0.0),
      glm::normalize(glm::dvec3(1.0, 1.0, 1.0)),
      glm::normalize(glm::dvec3(1.0, -1.0, 1.0)),
      glm::normalize(glm::dvec3(-1.0, -1.0, 1.0)),
      glm::normalize(glm::dvec3(-1.0, 1.0, 1.0)),
      glm::normalize(glm::dvec3(1.0, 1.0, -1.0)),
      glm::normalize(glm::dvec3(1.0, -1.0, -1.0)),
      glm::normalize(glm::dvec3(-1.0, -1.0, -1.0)),
      glm::normalize(glm::dvec3(-1.0, 1.0, -1.0)),
  };

  for (size_t i = 0; i < expected.size(); i++) {
    const glm::dvec3 value =
        AttributeCompression::octDecode(input[i].x, input[i].y);
    CHECK(Math::equalsEpsilon(value, expected[i], Math::Epsilon1));
  }
}

TEST_CASE("AttributeCompression::decodeRGB565") {
  const std::vector<uint16_t> input{
      0,
      // 0b00001_000001_00001
      2081,
      // 0b10000_100000_01000
      33800,
      // 0b11111_111111_11111
      65535,
  };
  const std::vector<glm::dvec3> expected{
      glm::dvec3(0.0),
      glm::dvec3(1.0 / 31.0, 1.0 / 63.0, 1.0 / 31.0),
      glm::dvec3(16.0 / 31.0, 32.0 / 63.0, 8.0 / 31.0),
      glm::dvec3(1.0)};

  for (size_t i = 0; i < expected.size(); i++) {
    const glm::dvec3 value = AttributeCompression::decodeRGB565(input[i]);
    CHECK(Math::equalsEpsilon(value, expected[i], Math::Epsilon6));
  }
}
