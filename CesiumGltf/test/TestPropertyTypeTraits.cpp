#include "CesiumGltf/PropertyTypeTraits.h"

#include <catch2/catch.hpp>

using namespace CesiumGltf;

TEST_CASE("Test PropertyTypeTraits") {
  SECTION("IsScalar") {
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

  SECTION("IsVecN") {
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

  SECTION("IsMatN") {
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

  SECTION("IsBoolean") {
    REQUIRE(IsMetadataBoolean<bool>::value);
    REQUIRE(!IsMetadataBoolean<uint8_t>::value);
    REQUIRE(!IsMetadataBoolean<std::string_view>::value);
  }

  SECTION("IsString") {
    REQUIRE(IsMetadataString<std::string_view>::value);
    REQUIRE(!IsMetadataString<std::string>::value);
  }

  SECTION("IsNumeric") {
    REQUIRE(IsMetadataNumeric<uint8_t>::value);
    REQUIRE(IsMetadataNumeric<float>::value);
    REQUIRE(IsMetadataNumeric<glm::i8vec2>::value);
    REQUIRE(IsMetadataNumeric<glm::dvec4>::value);
    REQUIRE(IsMetadataNumeric<glm::u32mat3x3>::value);
    REQUIRE(IsMetadataNumeric<glm::mat3>::value);
    REQUIRE(!IsMetadataNumeric<bool>::value);
    REQUIRE(!IsMetadataNumeric<std::string_view>::value);
  }

  SECTION("IsNumericArray") {
    REQUIRE(IsMetadataNumericArray<PropertyArrayView<uint32_t>>::value);
    REQUIRE(IsMetadataNumericArray<PropertyArrayView<glm::vec3>>::value);
    REQUIRE(IsMetadataNumericArray<PropertyArrayView<glm::mat4>>::value);
    REQUIRE(!IsMetadataNumericArray<PropertyArrayView<bool>>::value);
  }

  SECTION("IsBooleanArray") {
    REQUIRE(IsMetadataBooleanArray<PropertyArrayView<bool>>::value);
    REQUIRE(!IsMetadataBooleanArray<PropertyArrayView<uint8_t>>::value);
  }

  SECTION("IsStringArray") {
    REQUIRE(IsMetadataStringArray<PropertyArrayView<std::string_view>>::value);
    REQUIRE(!IsMetadataStringArray<PropertyArrayView<std::string>>::value);
    REQUIRE(!IsMetadataStringArray<PropertyArrayView<uint32_t>>::value);
  }

  SECTION("ArrayType") {
    using type = MetadataArrayType<PropertyArrayView<uint32_t>>::type;
    REQUIRE(std::is_same_v<type, uint32_t>);
  }

  SECTION("TypeToPropertyType scalar") {
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

  SECTION("TypeToPropertyType vecN") {
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

  SECTION("TypeToPropertyType matN") {
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

  SECTION("TypeToPropertyType boolean") {
    REQUIRE(TypeToPropertyType<bool>::value == PropertyType::Boolean);
    REQUIRE(TypeToPropertyType<bool>::component == PropertyComponentType::None);
  }

  SECTION("TypeToPropertyType string") {
    REQUIRE(
        TypeToPropertyType<std::string_view>::value == PropertyType::String);
    REQUIRE(
        TypeToPropertyType<std::string_view>::component ==
        PropertyComponentType::None);
  }
}
