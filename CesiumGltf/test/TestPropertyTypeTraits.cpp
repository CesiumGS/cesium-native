#include <CesiumGltf/PropertyArrayView.h>
#include <CesiumGltf/PropertyType.h>
#include <CesiumGltf/PropertyTypeTraits.h>

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
#include <string>
#include <string_view>
#include <type_traits>

using namespace CesiumGltf;

TEST_CASE("Test IsMetadataScalar") {
  REQUIRE(IsMetadataScalar<uint8_t>::value);
  REQUIRE(IsMetadataScalar<int8_t>::value);

  REQUIRE(IsMetadataScalar<uint16_t>::value);
  REQUIRE(IsMetadataScalar<int16_t>::value);

  REQUIRE(IsMetadataScalar<uint32_t>::value);
  REQUIRE(IsMetadataScalar<int32_t>::value);

  REQUIRE(IsMetadataScalar<uint64_t>::value);
  REQUIRE(IsMetadataScalar<int64_t>::value);

  REQUIRE(IsMetadataScalar<float>::value);
  REQUIRE(IsMetadataScalar<double>::value);

  REQUIRE(!IsMetadataScalar<glm::vec3>::value);
  REQUIRE(!IsMetadataScalar<glm::mat3>::value);
  REQUIRE(!IsMetadataScalar<bool>::value);
  REQUIRE(!IsMetadataScalar<std::string_view>::value);
}

TEST_CASE("Test IsMetadataVecN") {
  REQUIRE(IsMetadataVecN<glm::u8vec2>::value);
  REQUIRE(IsMetadataVecN<glm::u8vec3>::value);
  REQUIRE(IsMetadataVecN<glm::u8vec4>::value);

  REQUIRE(IsMetadataVecN<glm::i8vec2>::value);
  REQUIRE(IsMetadataVecN<glm::i8vec3>::value);
  REQUIRE(IsMetadataVecN<glm::i8vec4>::value);

  REQUIRE(IsMetadataVecN<glm::u16vec2>::value);
  REQUIRE(IsMetadataVecN<glm::u16vec3>::value);
  REQUIRE(IsMetadataVecN<glm::u16vec4>::value);

  REQUIRE(IsMetadataVecN<glm::i16vec2>::value);
  REQUIRE(IsMetadataVecN<glm::i16vec3>::value);
  REQUIRE(IsMetadataVecN<glm::i16vec4>::value);

  REQUIRE(IsMetadataVecN<glm::uvec2>::value);
  REQUIRE(IsMetadataVecN<glm::uvec3>::value);
  REQUIRE(IsMetadataVecN<glm::uvec4>::value);

  REQUIRE(IsMetadataVecN<glm::ivec2>::value);
  REQUIRE(IsMetadataVecN<glm::ivec3>::value);
  REQUIRE(IsMetadataVecN<glm::ivec4>::value);

  REQUIRE(IsMetadataVecN<glm::u64vec2>::value);
  REQUIRE(IsMetadataVecN<glm::u64vec3>::value);
  REQUIRE(IsMetadataVecN<glm::u64vec4>::value);

  REQUIRE(IsMetadataVecN<glm::i64vec2>::value);
  REQUIRE(IsMetadataVecN<glm::i64vec3>::value);
  REQUIRE(IsMetadataVecN<glm::i64vec4>::value);

  REQUIRE(IsMetadataVecN<glm::vec2>::value);
  REQUIRE(IsMetadataVecN<glm::vec3>::value);
  REQUIRE(IsMetadataVecN<glm::vec4>::value);

  REQUIRE(IsMetadataVecN<glm::dvec2>::value);
  REQUIRE(IsMetadataVecN<glm::dvec3>::value);
  REQUIRE(IsMetadataVecN<glm::dvec4>::value);

  REQUIRE(!IsMetadataVecN<uint8_t>::value);
  REQUIRE(!IsMetadataVecN<glm::mat3>::value);
}

TEST_CASE("Test IsMetadataMatN") {
  REQUIRE(IsMetadataMatN<glm::u8mat2x2>::value);
  REQUIRE(IsMetadataMatN<glm::u8mat3x3>::value);
  REQUIRE(IsMetadataMatN<glm::u8mat4x4>::value);

  REQUIRE(IsMetadataMatN<glm::i8mat2x2>::value);
  REQUIRE(IsMetadataMatN<glm::i8mat3x3>::value);
  REQUIRE(IsMetadataMatN<glm::i8mat4x4>::value);

  REQUIRE(IsMetadataMatN<glm::u16mat2x2>::value);
  REQUIRE(IsMetadataMatN<glm::u16mat3x3>::value);
  REQUIRE(IsMetadataMatN<glm::u16mat4x4>::value);

  REQUIRE(IsMetadataMatN<glm::i16mat2x2>::value);
  REQUIRE(IsMetadataMatN<glm::i16mat3x3>::value);
  REQUIRE(IsMetadataMatN<glm::i16mat4x4>::value);

  REQUIRE(IsMetadataMatN<glm::u32mat2x2>::value);
  REQUIRE(IsMetadataMatN<glm::u32mat3x3>::value);
  REQUIRE(IsMetadataMatN<glm::u32mat4x4>::value);

  REQUIRE(IsMetadataMatN<glm::i32mat2x2>::value);
  REQUIRE(IsMetadataMatN<glm::i32mat3x3>::value);
  REQUIRE(IsMetadataMatN<glm::i32mat4x4>::value);

  REQUIRE(IsMetadataMatN<glm::u64mat2x2>::value);
  REQUIRE(IsMetadataMatN<glm::u64mat3x3>::value);
  REQUIRE(IsMetadataMatN<glm::u64mat4x4>::value);

  REQUIRE(IsMetadataMatN<glm::i64mat2x2>::value);
  REQUIRE(IsMetadataMatN<glm::i64mat3x3>::value);
  REQUIRE(IsMetadataMatN<glm::i64mat4x4>::value);

  REQUIRE(IsMetadataMatN<glm::mat2>::value);
  REQUIRE(IsMetadataMatN<glm::mat3>::value);
  REQUIRE(IsMetadataMatN<glm::mat4>::value);

  REQUIRE(IsMetadataMatN<glm::dmat2>::value);
  REQUIRE(IsMetadataMatN<glm::dmat3>::value);
  REQUIRE(IsMetadataMatN<glm::dmat4>::value);

  REQUIRE(!IsMetadataMatN<uint8_t>::value);
  REQUIRE(!IsMetadataMatN<glm::vec3>::value);
}

TEST_CASE("Test IsMetadataBoolean") {
  REQUIRE(IsMetadataBoolean<bool>::value);
  REQUIRE(!IsMetadataBoolean<uint8_t>::value);
  REQUIRE(!IsMetadataBoolean<std::string_view>::value);
}

TEST_CASE("Test IsMetadataString") {
  REQUIRE(IsMetadataString<std::string_view>::value);
  REQUIRE(!IsMetadataString<std::string>::value);
}

TEST_CASE("Test IsMetadataNumeric") {
  REQUIRE(IsMetadataNumeric<uint8_t>::value);
  REQUIRE(IsMetadataNumeric<float>::value);
  REQUIRE(IsMetadataNumeric<glm::i8vec2>::value);
  REQUIRE(IsMetadataNumeric<glm::dvec4>::value);
  REQUIRE(IsMetadataNumeric<glm::u32mat3x3>::value);
  REQUIRE(IsMetadataNumeric<glm::mat3>::value);
  REQUIRE(!IsMetadataNumeric<bool>::value);
  REQUIRE(!IsMetadataNumeric<std::string_view>::value);
}

TEST_CASE("Test IsMetadataNumericArray") {
  REQUIRE(IsMetadataNumericArray<PropertyArrayView<uint32_t>>::value);
  REQUIRE(IsMetadataNumericArray<PropertyArrayView<glm::vec3>>::value);
  REQUIRE(IsMetadataNumericArray<PropertyArrayView<glm::mat4>>::value);
  REQUIRE(!IsMetadataNumericArray<PropertyArrayView<bool>>::value);
}

TEST_CASE("Test IsMetadataBooleanArray") {
  REQUIRE(IsMetadataBooleanArray<PropertyArrayView<bool>>::value);
  REQUIRE(!IsMetadataBooleanArray<PropertyArrayView<uint8_t>>::value);
}

TEST_CASE("Test IsStringArray") {
  REQUIRE(IsMetadataStringArray<PropertyArrayView<std::string_view>>::value);
  REQUIRE(!IsMetadataStringArray<PropertyArrayView<std::string>>::value);
  REQUIRE(!IsMetadataStringArray<PropertyArrayView<uint32_t>>::value);
}

TEST_CASE("Test MetadataArrayType") {
  using type = MetadataArrayType<PropertyArrayView<uint32_t>>::type;
  REQUIRE(std::is_same_v<type, uint32_t>);
}

TEST_CASE("TypeToPropertyType") {
  SUBCASE("Works for scalar types") {
    REQUIRE(TypeToPropertyType<uint8_t>::value == PropertyType::Scalar);
    REQUIRE(
        TypeToPropertyType<uint8_t>::component == PropertyComponentType::Uint8);

    REQUIRE(TypeToPropertyType<int8_t>::value == PropertyType::Scalar);
    REQUIRE(
        TypeToPropertyType<int8_t>::component == PropertyComponentType::Int8);

    REQUIRE(TypeToPropertyType<uint16_t>::value == PropertyType::Scalar);
    REQUIRE(
        TypeToPropertyType<uint16_t>::component ==
        PropertyComponentType::Uint16);

    REQUIRE(TypeToPropertyType<int16_t>::value == PropertyType::Scalar);
    REQUIRE(
        TypeToPropertyType<int16_t>::component == PropertyComponentType::Int16);

    REQUIRE(TypeToPropertyType<uint32_t>::value == PropertyType::Scalar);
    REQUIRE(
        TypeToPropertyType<uint32_t>::component ==
        PropertyComponentType::Uint32);

    REQUIRE(TypeToPropertyType<int32_t>::value == PropertyType::Scalar);
    REQUIRE(
        TypeToPropertyType<int32_t>::component == PropertyComponentType::Int32);

    REQUIRE(TypeToPropertyType<uint64_t>::value == PropertyType::Scalar);
    REQUIRE(
        TypeToPropertyType<uint64_t>::component ==
        PropertyComponentType::Uint64);

    REQUIRE(TypeToPropertyType<int64_t>::value == PropertyType::Scalar);
    REQUIRE(
        TypeToPropertyType<int64_t>::component == PropertyComponentType::Int64);
  }

  SUBCASE("Works for vecN types") {
    // Vec2
    REQUIRE(TypeToPropertyType<glm::u8vec2>::value == PropertyType::Vec2);
    REQUIRE(
        TypeToPropertyType<glm::u8vec2>::component ==
        PropertyComponentType::Uint8);

    REQUIRE(TypeToPropertyType<glm::i8vec2>::value == PropertyType::Vec2);
    REQUIRE(
        TypeToPropertyType<glm::i8vec2>::component ==
        PropertyComponentType::Int8);

    REQUIRE(TypeToPropertyType<glm::u16vec2>::value == PropertyType::Vec2);
    REQUIRE(
        TypeToPropertyType<glm::u16vec2>::component ==
        PropertyComponentType::Uint16);

    REQUIRE(TypeToPropertyType<glm::i16vec2>::value == PropertyType::Vec2);
    REQUIRE(
        TypeToPropertyType<glm::i16vec2>::component ==
        PropertyComponentType::Int16);

    REQUIRE(TypeToPropertyType<glm::uvec2>::value == PropertyType::Vec2);
    REQUIRE(
        TypeToPropertyType<glm::uvec2>::component ==
        PropertyComponentType::Uint32);

    REQUIRE(TypeToPropertyType<glm::ivec2>::value == PropertyType::Vec2);
    REQUIRE(
        TypeToPropertyType<glm::ivec2>::component ==
        PropertyComponentType::Int32);

    REQUIRE(TypeToPropertyType<glm::u64vec2>::value == PropertyType::Vec2);
    REQUIRE(
        TypeToPropertyType<glm::u64vec2>::component ==
        PropertyComponentType::Uint64);

    REQUIRE(TypeToPropertyType<glm::i64vec2>::value == PropertyType::Vec2);
    REQUIRE(
        TypeToPropertyType<glm::i64vec2>::component ==
        PropertyComponentType::Int64);

    REQUIRE(TypeToPropertyType<glm::vec2>::value == PropertyType::Vec2);
    REQUIRE(
        TypeToPropertyType<glm::vec2>::component ==
        PropertyComponentType::Float32);

    REQUIRE(TypeToPropertyType<glm::dvec2>::value == PropertyType::Vec2);
    REQUIRE(
        TypeToPropertyType<glm::dvec2>::component ==
        PropertyComponentType::Float64);

    // Vec3
    REQUIRE(TypeToPropertyType<glm::u8vec3>::value == PropertyType::Vec3);
    REQUIRE(
        TypeToPropertyType<glm::u8vec3>::component ==
        PropertyComponentType::Uint8);

    REQUIRE(TypeToPropertyType<glm::i8vec3>::value == PropertyType::Vec3);
    REQUIRE(
        TypeToPropertyType<glm::i8vec3>::component ==
        PropertyComponentType::Int8);

    REQUIRE(TypeToPropertyType<glm::u16vec3>::value == PropertyType::Vec3);
    REQUIRE(
        TypeToPropertyType<glm::u16vec3>::component ==
        PropertyComponentType::Uint16);

    REQUIRE(TypeToPropertyType<glm::i16vec3>::value == PropertyType::Vec3);
    REQUIRE(
        TypeToPropertyType<glm::i16vec3>::component ==
        PropertyComponentType::Int16);

    REQUIRE(TypeToPropertyType<glm::uvec3>::value == PropertyType::Vec3);
    REQUIRE(
        TypeToPropertyType<glm::uvec3>::component ==
        PropertyComponentType::Uint32);

    REQUIRE(TypeToPropertyType<glm::ivec3>::value == PropertyType::Vec3);
    REQUIRE(
        TypeToPropertyType<glm::ivec3>::component ==
        PropertyComponentType::Int32);

    REQUIRE(TypeToPropertyType<glm::u64vec3>::value == PropertyType::Vec3);
    REQUIRE(
        TypeToPropertyType<glm::u64vec3>::component ==
        PropertyComponentType::Uint64);

    REQUIRE(TypeToPropertyType<glm::i64vec3>::value == PropertyType::Vec3);
    REQUIRE(
        TypeToPropertyType<glm::i64vec3>::component ==
        PropertyComponentType::Int64);

    REQUIRE(TypeToPropertyType<glm::vec3>::value == PropertyType::Vec3);
    REQUIRE(
        TypeToPropertyType<glm::vec3>::component ==
        PropertyComponentType::Float32);

    REQUIRE(TypeToPropertyType<glm::dvec3>::value == PropertyType::Vec3);
    REQUIRE(
        TypeToPropertyType<glm::dvec3>::component ==
        PropertyComponentType::Float64);

    // Vec4
    REQUIRE(TypeToPropertyType<glm::u8vec4>::value == PropertyType::Vec4);
    REQUIRE(
        TypeToPropertyType<glm::u8vec4>::component ==
        PropertyComponentType::Uint8);

    REQUIRE(TypeToPropertyType<glm::i8vec4>::value == PropertyType::Vec4);
    REQUIRE(
        TypeToPropertyType<glm::i8vec4>::component ==
        PropertyComponentType::Int8);

    REQUIRE(TypeToPropertyType<glm::u16vec4>::value == PropertyType::Vec4);
    REQUIRE(
        TypeToPropertyType<glm::u16vec4>::component ==
        PropertyComponentType::Uint16);

    REQUIRE(TypeToPropertyType<glm::i16vec4>::value == PropertyType::Vec4);
    REQUIRE(
        TypeToPropertyType<glm::i16vec4>::component ==
        PropertyComponentType::Int16);

    REQUIRE(TypeToPropertyType<glm::uvec4>::value == PropertyType::Vec4);
    REQUIRE(
        TypeToPropertyType<glm::uvec4>::component ==
        PropertyComponentType::Uint32);

    REQUIRE(TypeToPropertyType<glm::ivec4>::value == PropertyType::Vec4);
    REQUIRE(
        TypeToPropertyType<glm::ivec4>::component ==
        PropertyComponentType::Int32);

    REQUIRE(TypeToPropertyType<glm::u64vec4>::value == PropertyType::Vec4);
    REQUIRE(
        TypeToPropertyType<glm::u64vec4>::component ==
        PropertyComponentType::Uint64);

    REQUIRE(TypeToPropertyType<glm::i64vec4>::value == PropertyType::Vec4);
    REQUIRE(
        TypeToPropertyType<glm::i64vec4>::component ==
        PropertyComponentType::Int64);

    REQUIRE(TypeToPropertyType<glm::vec4>::value == PropertyType::Vec4);
    REQUIRE(
        TypeToPropertyType<glm::vec4>::component ==
        PropertyComponentType::Float32);

    REQUIRE(TypeToPropertyType<glm::dvec4>::value == PropertyType::Vec4);
    REQUIRE(
        TypeToPropertyType<glm::dvec4>::component ==
        PropertyComponentType::Float64);
  }

  SUBCASE("Works for matN types") {
    // Mat2
    REQUIRE(TypeToPropertyType<glm::u8mat2x2>::value == PropertyType::Mat2);
    REQUIRE(
        TypeToPropertyType<glm::u8mat2x2>::component ==
        PropertyComponentType::Uint8);

    REQUIRE(TypeToPropertyType<glm::i8mat2x2>::value == PropertyType::Mat2);
    REQUIRE(
        TypeToPropertyType<glm::i8mat2x2>::component ==
        PropertyComponentType::Int8);

    REQUIRE(TypeToPropertyType<glm::u16mat2x2>::value == PropertyType::Mat2);
    REQUIRE(
        TypeToPropertyType<glm::u16mat2x2>::component ==
        PropertyComponentType::Uint16);

    REQUIRE(TypeToPropertyType<glm::i16mat2x2>::value == PropertyType::Mat2);
    REQUIRE(
        TypeToPropertyType<glm::i16mat2x2>::component ==
        PropertyComponentType::Int16);

    REQUIRE(TypeToPropertyType<glm::u32mat2x2>::value == PropertyType::Mat2);
    REQUIRE(
        TypeToPropertyType<glm::u32mat2x2>::component ==
        PropertyComponentType::Uint32);

    REQUIRE(TypeToPropertyType<glm::i32mat2x2>::value == PropertyType::Mat2);
    REQUIRE(
        TypeToPropertyType<glm::i32mat2x2>::component ==
        PropertyComponentType::Int32);

    REQUIRE(TypeToPropertyType<glm::u64mat2x2>::value == PropertyType::Mat2);
    REQUIRE(
        TypeToPropertyType<glm::u64mat2x2>::component ==
        PropertyComponentType::Uint64);

    REQUIRE(TypeToPropertyType<glm::i64mat2x2>::value == PropertyType::Mat2);
    REQUIRE(
        TypeToPropertyType<glm::i64mat2x2>::component ==
        PropertyComponentType::Int64);

    REQUIRE(TypeToPropertyType<glm::mat2>::value == PropertyType::Mat2);
    REQUIRE(
        TypeToPropertyType<glm::mat2>::component ==
        PropertyComponentType::Float32);

    REQUIRE(TypeToPropertyType<glm::dmat2>::value == PropertyType::Mat2);
    REQUIRE(
        TypeToPropertyType<glm::dmat2>::component ==
        PropertyComponentType::Float64);

    // Mat3
    REQUIRE(TypeToPropertyType<glm::u8mat3x3>::value == PropertyType::Mat3);
    REQUIRE(
        TypeToPropertyType<glm::u8mat3x3>::component ==
        PropertyComponentType::Uint8);

    REQUIRE(TypeToPropertyType<glm::i8mat3x3>::value == PropertyType::Mat3);
    REQUIRE(
        TypeToPropertyType<glm::i8mat3x3>::component ==
        PropertyComponentType::Int8);

    REQUIRE(TypeToPropertyType<glm::u16mat3x3>::value == PropertyType::Mat3);
    REQUIRE(
        TypeToPropertyType<glm::u16mat3x3>::component ==
        PropertyComponentType::Uint16);

    REQUIRE(TypeToPropertyType<glm::i16mat3x3>::value == PropertyType::Mat3);
    REQUIRE(
        TypeToPropertyType<glm::i16mat3x3>::component ==
        PropertyComponentType::Int16);

    REQUIRE(TypeToPropertyType<glm::u32mat3x3>::value == PropertyType::Mat3);
    REQUIRE(
        TypeToPropertyType<glm::u32mat3x3>::component ==
        PropertyComponentType::Uint32);

    REQUIRE(TypeToPropertyType<glm::i32mat3x3>::value == PropertyType::Mat3);
    REQUIRE(
        TypeToPropertyType<glm::i32mat3x3>::component ==
        PropertyComponentType::Int32);

    REQUIRE(TypeToPropertyType<glm::u64mat3x3>::value == PropertyType::Mat3);
    REQUIRE(
        TypeToPropertyType<glm::u64mat3x3>::component ==
        PropertyComponentType::Uint64);

    REQUIRE(TypeToPropertyType<glm::i64mat3x3>::value == PropertyType::Mat3);
    REQUIRE(
        TypeToPropertyType<glm::i64mat3x3>::component ==
        PropertyComponentType::Int64);

    REQUIRE(TypeToPropertyType<glm::mat3>::value == PropertyType::Mat3);
    REQUIRE(
        TypeToPropertyType<glm::mat3>::component ==
        PropertyComponentType::Float32);

    REQUIRE(TypeToPropertyType<glm::dmat3>::value == PropertyType::Mat3);
    REQUIRE(
        TypeToPropertyType<glm::dmat3>::component ==
        PropertyComponentType::Float64);

    // Mat4
    REQUIRE(TypeToPropertyType<glm::u8mat4x4>::value == PropertyType::Mat4);
    REQUIRE(
        TypeToPropertyType<glm::u8mat4x4>::component ==
        PropertyComponentType::Uint8);

    REQUIRE(TypeToPropertyType<glm::i8mat4x4>::value == PropertyType::Mat4);
    REQUIRE(
        TypeToPropertyType<glm::i8mat4x4>::component ==
        PropertyComponentType::Int8);

    REQUIRE(TypeToPropertyType<glm::u16mat4x4>::value == PropertyType::Mat4);
    REQUIRE(
        TypeToPropertyType<glm::u16mat4x4>::component ==
        PropertyComponentType::Uint16);

    REQUIRE(TypeToPropertyType<glm::i16mat4x4>::value == PropertyType::Mat4);
    REQUIRE(
        TypeToPropertyType<glm::i16mat4x4>::component ==
        PropertyComponentType::Int16);

    REQUIRE(TypeToPropertyType<glm::u32mat4x4>::value == PropertyType::Mat4);
    REQUIRE(
        TypeToPropertyType<glm::u32mat4x4>::component ==
        PropertyComponentType::Uint32);

    REQUIRE(TypeToPropertyType<glm::i32mat4x4>::value == PropertyType::Mat4);
    REQUIRE(
        TypeToPropertyType<glm::i32mat4x4>::component ==
        PropertyComponentType::Int32);

    REQUIRE(TypeToPropertyType<glm::u64mat4x4>::value == PropertyType::Mat4);
    REQUIRE(
        TypeToPropertyType<glm::u64mat4x4>::component ==
        PropertyComponentType::Uint64);

    REQUIRE(TypeToPropertyType<glm::i64mat4x4>::value == PropertyType::Mat4);
    REQUIRE(
        TypeToPropertyType<glm::i64mat4x4>::component ==
        PropertyComponentType::Int64);

    REQUIRE(TypeToPropertyType<glm::mat4>::value == PropertyType::Mat4);
    REQUIRE(
        TypeToPropertyType<glm::mat4>::component ==
        PropertyComponentType::Float32);

    REQUIRE(TypeToPropertyType<glm::dmat4>::value == PropertyType::Mat4);
    REQUIRE(
        TypeToPropertyType<glm::dmat4>::component ==
        PropertyComponentType::Float64);
  }

  SUBCASE("Works for boolean") {
    REQUIRE(TypeToPropertyType<bool>::value == PropertyType::Boolean);
    REQUIRE(TypeToPropertyType<bool>::component == PropertyComponentType::None);
  }

  SUBCASE("Works for string") {
    REQUIRE(
        TypeToPropertyType<std::string_view>::value == PropertyType::String);
    REQUIRE(
        TypeToPropertyType<std::string_view>::component ==
        PropertyComponentType::None);
  }
}

TEST_CASE("CanBeNormalized") {
  SUBCASE("Works for scalars") {
    REQUIRE(CanBeNormalized<uint8_t>::value);
    REQUIRE(CanBeNormalized<int8_t>::value);
    REQUIRE(CanBeNormalized<uint16_t>::value);
    REQUIRE(CanBeNormalized<int16_t>::value);
    REQUIRE(CanBeNormalized<uint32_t>::value);
    REQUIRE(CanBeNormalized<int32_t>::value);
    REQUIRE(CanBeNormalized<uint64_t>::value);
    REQUIRE(CanBeNormalized<int64_t>::value);

    REQUIRE(!CanBeNormalized<float>::value);
    REQUIRE(!CanBeNormalized<double>::value);
    REQUIRE(!CanBeNormalized<bool>::value);
    REQUIRE(!CanBeNormalized<std::string_view>::value);
  }

  SUBCASE("Works for vecNs") {
    REQUIRE(CanBeNormalized<glm::u8vec2>::value);
    REQUIRE(CanBeNormalized<glm::u8vec3>::value);
    REQUIRE(CanBeNormalized<glm::u8vec4>::value);

    REQUIRE(CanBeNormalized<glm::i8vec2>::value);
    REQUIRE(CanBeNormalized<glm::i8vec3>::value);
    REQUIRE(CanBeNormalized<glm::i8vec4>::value);

    REQUIRE(CanBeNormalized<glm::u16vec2>::value);
    REQUIRE(CanBeNormalized<glm::u16vec3>::value);
    REQUIRE(CanBeNormalized<glm::u16vec4>::value);

    REQUIRE(CanBeNormalized<glm::i16vec2>::value);
    REQUIRE(CanBeNormalized<glm::i16vec3>::value);
    REQUIRE(CanBeNormalized<glm::i16vec4>::value);

    REQUIRE(CanBeNormalized<glm::uvec2>::value);
    REQUIRE(CanBeNormalized<glm::uvec3>::value);
    REQUIRE(CanBeNormalized<glm::uvec4>::value);

    REQUIRE(CanBeNormalized<glm::ivec2>::value);
    REQUIRE(CanBeNormalized<glm::ivec3>::value);
    REQUIRE(CanBeNormalized<glm::ivec4>::value);

    REQUIRE(CanBeNormalized<glm::u64vec2>::value);
    REQUIRE(CanBeNormalized<glm::u64vec3>::value);
    REQUIRE(CanBeNormalized<glm::u64vec4>::value);

    REQUIRE(CanBeNormalized<glm::i64vec2>::value);
    REQUIRE(CanBeNormalized<glm::i64vec3>::value);
    REQUIRE(CanBeNormalized<glm::i64vec4>::value);

    REQUIRE(!CanBeNormalized<glm::vec2>::value);
    REQUIRE(!CanBeNormalized<glm::vec3>::value);
    REQUIRE(!CanBeNormalized<glm::vec4>::value);

    REQUIRE(!CanBeNormalized<glm::dvec2>::value);
    REQUIRE(!CanBeNormalized<glm::dvec3>::value);
    REQUIRE(!CanBeNormalized<glm::dvec4>::value);
  }

  SUBCASE("Works for matN") {
    REQUIRE(CanBeNormalized<glm::u8mat2x2>::value);
    REQUIRE(CanBeNormalized<glm::u8mat3x3>::value);
    REQUIRE(CanBeNormalized<glm::u8mat4x4>::value);

    REQUIRE(CanBeNormalized<glm::i8mat2x2>::value);
    REQUIRE(CanBeNormalized<glm::i8mat3x3>::value);
    REQUIRE(CanBeNormalized<glm::i8mat4x4>::value);

    REQUIRE(CanBeNormalized<glm::u16mat2x2>::value);
    REQUIRE(CanBeNormalized<glm::u16mat3x3>::value);
    REQUIRE(CanBeNormalized<glm::u16mat4x4>::value);

    REQUIRE(CanBeNormalized<glm::i16mat2x2>::value);
    REQUIRE(CanBeNormalized<glm::i16mat3x3>::value);
    REQUIRE(CanBeNormalized<glm::i16mat4x4>::value);

    REQUIRE(CanBeNormalized<glm::umat2x2>::value);
    REQUIRE(CanBeNormalized<glm::umat3x3>::value);
    REQUIRE(CanBeNormalized<glm::umat4x4>::value);

    REQUIRE(CanBeNormalized<glm::imat2x2>::value);
    REQUIRE(CanBeNormalized<glm::imat3x3>::value);
    REQUIRE(CanBeNormalized<glm::imat4x4>::value);

    REQUIRE(CanBeNormalized<glm::u64mat2x2>::value);
    REQUIRE(CanBeNormalized<glm::u64mat3x3>::value);
    REQUIRE(CanBeNormalized<glm::u64mat4x4>::value);

    REQUIRE(CanBeNormalized<glm::i64mat2x2>::value);
    REQUIRE(CanBeNormalized<glm::i64mat3x3>::value);
    REQUIRE(CanBeNormalized<glm::i64mat4x4>::value);

    REQUIRE(!CanBeNormalized<glm::mat2>::value);
    REQUIRE(!CanBeNormalized<glm::mat3>::value);
    REQUIRE(!CanBeNormalized<glm::mat4>::value);

    REQUIRE(!CanBeNormalized<glm::dmat2>::value);
    REQUIRE(!CanBeNormalized<glm::dmat3>::value);
    REQUIRE(!CanBeNormalized<glm::dmat4>::value);
  }

  SUBCASE("Works for arrays") {
    REQUIRE(CanBeNormalized<PropertyArrayView<int32_t>>::value);
    REQUIRE(CanBeNormalized<PropertyArrayView<glm::uvec2>>::value);
    REQUIRE(CanBeNormalized<PropertyArrayView<glm::i64mat2x2>>::value);
  }
}

TEST_CASE("TypeToNormalizedType") {
  SUBCASE("Works for scalars") {
    REQUIRE(std::is_same_v<TypeToNormalizedType<uint8_t>::type, double>);
    REQUIRE(std::is_same_v<TypeToNormalizedType<int8_t>::type, double>);
    REQUIRE(std::is_same_v<TypeToNormalizedType<uint16_t>::type, double>);
    REQUIRE(std::is_same_v<TypeToNormalizedType<int16_t>::type, double>);
    REQUIRE(std::is_same_v<TypeToNormalizedType<uint16_t>::type, double>);
    REQUIRE(std::is_same_v<TypeToNormalizedType<int32_t>::type, double>);
    REQUIRE(std::is_same_v<TypeToNormalizedType<uint32_t>::type, double>);
    REQUIRE(std::is_same_v<TypeToNormalizedType<int64_t>::type, double>);
    REQUIRE(std::is_same_v<TypeToNormalizedType<uint64_t>::type, double>);
  }

  SUBCASE("Works for vecNs") {
    using ExpectedVec2Type = glm::dvec2;
    using ExpectedVec3Type = glm::dvec3;
    using ExpectedVec4Type = glm::dvec4;

    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::u8vec2>::type,
            ExpectedVec2Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::u8vec3>::type,
            ExpectedVec3Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::u8vec4>::type,
            ExpectedVec4Type>);

    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::i8vec2>::type,
            ExpectedVec2Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::i8vec3>::type,
            ExpectedVec3Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::i8vec4>::type,
            ExpectedVec4Type>);

    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::u16vec2>::type,
            ExpectedVec2Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::u16vec3>::type,
            ExpectedVec3Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::u16vec4>::type,
            ExpectedVec4Type>);

    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::i16vec2>::type,
            ExpectedVec2Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::i16vec3>::type,
            ExpectedVec3Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::i16vec4>::type,
            ExpectedVec4Type>);

    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::uvec2>::type,
            ExpectedVec2Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::uvec3>::type,
            ExpectedVec3Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::uvec4>::type,
            ExpectedVec4Type>);

    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::ivec2>::type,
            ExpectedVec2Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::ivec3>::type,
            ExpectedVec3Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::ivec4>::type,
            ExpectedVec4Type>);

    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::u64vec2>::type,
            ExpectedVec2Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::u64vec3>::type,
            ExpectedVec3Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::u64vec4>::type,
            ExpectedVec4Type>);

    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::i64vec2>::type,
            ExpectedVec2Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::i64vec3>::type,
            ExpectedVec3Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::i64vec4>::type,
            ExpectedVec4Type>);
  }

  SUBCASE("Works for matNs") {
    using ExpectedMat2Type = glm::dmat2;
    using ExpectedMat3Type = glm::dmat3;
    using ExpectedMat4Type = glm::dmat4;

    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::u8mat2x2>::type,
            ExpectedMat2Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::u8mat3x3>::type,
            ExpectedMat3Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::u8mat4x4>::type,
            ExpectedMat4Type>);

    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::i8mat2x2>::type,
            ExpectedMat2Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::i8mat3x3>::type,
            ExpectedMat3Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::i8mat4x4>::type,
            ExpectedMat4Type>);

    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::u16mat2x2>::type,
            ExpectedMat2Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::u16mat3x3>::type,
            ExpectedMat3Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::u16mat4x4>::type,
            ExpectedMat4Type>);

    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::i16mat2x2>::type,
            ExpectedMat2Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::i16mat3x3>::type,
            ExpectedMat3Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::i16mat4x4>::type,
            ExpectedMat4Type>);

    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::umat2x2>::type,
            ExpectedMat2Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::umat3x3>::type,
            ExpectedMat3Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::umat4x4>::type,
            ExpectedMat4Type>);

    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::imat2x2>::type,
            ExpectedMat2Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::imat3x3>::type,
            ExpectedMat3Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::imat4x4>::type,
            ExpectedMat4Type>);

    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::u64mat2x2>::type,
            ExpectedMat2Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::u64mat3x3>::type,
            ExpectedMat3Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::u64mat4x4>::type,
            ExpectedMat4Type>);

    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::i64mat2x2>::type,
            ExpectedMat2Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::i64mat3x3>::type,
            ExpectedMat3Type>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<glm::i64mat4x4>::type,
            ExpectedMat4Type>);
  }

  SUBCASE("Works for arrays") {
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<PropertyArrayView<int64_t>>::type,
            PropertyArrayView<double>>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<PropertyArrayView<glm::u8vec4>>::type,
            PropertyArrayView<glm::dvec4>>);
    REQUIRE(std::is_same_v<
            TypeToNormalizedType<PropertyArrayView<glm::imat2x2>>::type,
            PropertyArrayView<glm::dmat2>>);
  }
}
