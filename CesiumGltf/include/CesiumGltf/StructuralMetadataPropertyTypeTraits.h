#pragma once

#include "CesiumGltf/StructuralMetadataArrayView.h"
#include "CesiumGltf/StructuralMetadataPropertyType.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <type_traits>

namespace CesiumGltf {
namespace StructuralMetadata {

/**
 * @brief Check if a C++ type can be represented as a scalar property type
 */
template <typename... T> struct IsMetadataScalar;
template <typename T> struct IsMetadataScalar<T> : std::false_type {};
template <> struct IsMetadataScalar<uint8_t> : std::true_type {};
template <> struct IsMetadataScalar<int8_t> : std::true_type {};
template <> struct IsMetadataScalar<uint16_t> : std::true_type {};
template <> struct IsMetadataScalar<int16_t> : std::true_type {};
template <> struct IsMetadataScalar<uint32_t> : std::true_type {};
template <> struct IsMetadataScalar<int32_t> : std::true_type {};
template <> struct IsMetadataScalar<uint64_t> : std::true_type {};
template <> struct IsMetadataScalar<int64_t> : std::true_type {};
template <> struct IsMetadataScalar<float> : std::true_type {};
template <> struct IsMetadataScalar<double> : std::true_type {};

/**
 * @brief Check if a C++ type can be represented as an integer property type
 */
template <typename... T> struct IsMetadataInteger;
template <typename T> struct IsMetadataInteger<T> : std::false_type {};
template <> struct IsMetadataInteger<uint8_t> : std::true_type {};
template <> struct IsMetadataInteger<int8_t> : std::true_type {};
template <> struct IsMetadataInteger<uint16_t> : std::true_type {};
template <> struct IsMetadataInteger<int16_t> : std::true_type {};
template <> struct IsMetadataInteger<uint32_t> : std::true_type {};
template <> struct IsMetadataInteger<int32_t> : std::true_type {};
template <> struct IsMetadataInteger<uint64_t> : std::true_type {};
template <> struct IsMetadataInteger<int64_t> : std::true_type {};

/**
 * @brief Check if a C++ type can be represented as a floating-point property
 * type.
 */
template <typename... T> struct IsMetadataFloating;
template <typename T> struct IsMetadataFloating<T> : std::false_type {};
template <> struct IsMetadataFloating<float> : std::true_type {};
template <> struct IsMetadataFloating<double> : std::true_type {};

/**
 * @brief Check if a C++ type can be represented as a vecN type.
 */
template <typename... T> struct IsMetadataVecN;
template <typename T> struct IsMetadataVecN<T> : std::false_type {};
template <> struct IsMetadataVecN<glm::u8vec2> : std::true_type {};
template <> struct IsMetadataVecN<glm::u8vec3> : std::true_type {};
template <> struct IsMetadataVecN<glm::u8vec4> : std::true_type {};
template <> struct IsMetadataVecN<glm::i8vec2> : std::true_type {};
template <> struct IsMetadataVecN<glm::i8vec3> : std::true_type {};
template <> struct IsMetadataVecN<glm::i8vec4> : std::true_type {};
template <> struct IsMetadataVecN<glm::u16vec2> : std::true_type {};
template <> struct IsMetadataVecN<glm::u16vec3> : std::true_type {};
template <> struct IsMetadataVecN<glm::u16vec4> : std::true_type {};
template <> struct IsMetadataVecN<glm::i16vec2> : std::true_type {};
template <> struct IsMetadataVecN<glm::i16vec3> : std::true_type {};
template <> struct IsMetadataVecN<glm::i16vec4> : std::true_type {};
template <> struct IsMetadataVecN<glm::uvec2> : std::true_type {};
template <> struct IsMetadataVecN<glm::uvec3> : std::true_type {};
template <> struct IsMetadataVecN<glm::uvec4> : std::true_type {};
template <> struct IsMetadataVecN<glm::ivec2> : std::true_type {};
template <> struct IsMetadataVecN<glm::ivec3> : std::true_type {};
template <> struct IsMetadataVecN<glm::ivec4> : std::true_type {};
template <> struct IsMetadataVecN<glm::u64vec2> : std::true_type {};
template <> struct IsMetadataVecN<glm::u64vec3> : std::true_type {};
template <> struct IsMetadataVecN<glm::u64vec4> : std::true_type {};
template <> struct IsMetadataVecN<glm::i64vec2> : std::true_type {};
template <> struct IsMetadataVecN<glm::i64vec3> : std::true_type {};
template <> struct IsMetadataVecN<glm::i64vec4> : std::true_type {};
template <> struct IsMetadataVecN<glm::vec2> : std::true_type {};
template <> struct IsMetadataVecN<glm::vec3> : std::true_type {};
template <> struct IsMetadataVecN<glm::vec4> : std::true_type {};
template <> struct IsMetadataVecN<glm::dvec2> : std::true_type {};
template <> struct IsMetadataVecN<glm::dvec3> : std::true_type {};
template <> struct IsMetadataVecN<glm::dvec4> : std::true_type {};

/**
 * @brief Check if a C++ type can be represented as a matN type.
 */
template <typename... T> struct IsMetadataMatN;
template <typename T> struct IsMetadataMatN<T> : std::false_type {};
template <> struct IsMetadataMatN<glm::u8mat2x2> : std::true_type {};
template <> struct IsMetadataMatN<glm::u8mat3x3> : std::true_type {};
template <> struct IsMetadataMatN<glm::u8mat4x4> : std::true_type {};
template <> struct IsMetadataMatN<glm::i8mat2x2> : std::true_type {};
template <> struct IsMetadataMatN<glm::i8mat3x3> : std::true_type {};
template <> struct IsMetadataMatN<glm::i8mat4x4> : std::true_type {};
template <> struct IsMetadataMatN<glm::u16mat2x2> : std::true_type {};
template <> struct IsMetadataMatN<glm::u16mat3x3> : std::true_type {};
template <> struct IsMetadataMatN<glm::u16mat4x4> : std::true_type {};
template <> struct IsMetadataMatN<glm::i16mat2x2> : std::true_type {};
template <> struct IsMetadataMatN<glm::i16mat3x3> : std::true_type {};
template <> struct IsMetadataMatN<glm::i16mat4x4> : std::true_type {};
template <> struct IsMetadataMatN<glm::u32mat2x2> : std::true_type {};
template <> struct IsMetadataMatN<glm::u32mat3x3> : std::true_type {};
template <> struct IsMetadataMatN<glm::u32mat4x4> : std::true_type {};
template <> struct IsMetadataMatN<glm::i32mat2x2> : std::true_type {};
template <> struct IsMetadataMatN<glm::i32mat3x3> : std::true_type {};
template <> struct IsMetadataMatN<glm::i32mat4x4> : std::true_type {};
template <> struct IsMetadataMatN<glm::u64mat2x2> : std::true_type {};
template <> struct IsMetadataMatN<glm::u64mat3x3> : std::true_type {};
template <> struct IsMetadataMatN<glm::u64mat4x4> : std::true_type {};
template <> struct IsMetadataMatN<glm::i64mat2x2> : std::true_type {};
template <> struct IsMetadataMatN<glm::i64mat3x3> : std::true_type {};
template <> struct IsMetadataMatN<glm::i64mat4x4> : std::true_type {};
template <> struct IsMetadataMatN<glm::mat2> : std::true_type {};
template <> struct IsMetadataMatN<glm::mat3> : std::true_type {};
template <> struct IsMetadataMatN<glm::mat4> : std::true_type {};
template <> struct IsMetadataMatN<glm::dmat2> : std::true_type {};
template <> struct IsMetadataMatN<glm::dmat3> : std::true_type {};
template <> struct IsMetadataMatN<glm::dmat4> : std::true_type {};

/**
 * @brief Check if a C++ type can be represented as a numeric property, i.e.
 * a scalar / vecN / matN type.
 */
template <typename... T> struct IsMetadataNumeric;
template <typename T> struct IsMetadataNumeric<T> {
  static constexpr bool value = IsMetadataScalar<T>::value ||
                                IsMetadataVecN<T>::value ||
                                IsMetadataMatN<T>::value;
};

/**
 * @brief Check if a C++ type can be represented as a boolean property type
 */
template <typename... T> struct IsMetadataBoolean;
template <typename T> struct IsMetadataBoolean<T> : std::false_type {};
template <> struct IsMetadataBoolean<bool> : std::true_type {};

/**
 * @brief Check if a C++ type can be represented as a string property type
 */
template <typename... T> struct IsMetadataString;
template <typename T> struct IsMetadataString<T> : std::false_type {};
template <> struct IsMetadataString<std::string_view> : std::true_type {};

/**
 * @brief Check if a C++ type can be represented as an array.
 */
template <typename... T> struct IsMetadataArray;
template <typename T> struct IsMetadataArray<T> : std::false_type {};
template <typename T>
struct IsMetadataArray<MetadataArrayView<T>> : std::true_type {};

/**
 * @brief Check if a C++ type can be represented as an array of numeric elements
 * property type
 */
template <typename... T> struct IsMetadataNumericArray;
template <typename T> struct IsMetadataNumericArray<T> : std::false_type {};
template <typename T> struct IsMetadataNumericArray<MetadataArrayView<T>> {
  static constexpr bool value = IsMetadataNumeric<T>::value;
};

/**
 * @brief Check if a C++ type can be represented as an array of booleans
 * property type
 */
template <typename... T> struct IsMetadataBooleanArray;
template <typename T> struct IsMetadataBooleanArray<T> : std::false_type {};
template <>
struct IsMetadataBooleanArray<MetadataArrayView<bool>> : std::true_type {};

/**
 * @brief Check if a C++ type can be represented as an array of strings property
 * type
 */
template <typename... T> struct IsMetadataStringArray;
template <typename T> struct IsMetadataStringArray<T> : std::false_type {};
template <>
struct IsMetadataStringArray<MetadataArrayView<std::string_view>>
    : std::true_type {};

/**
 * @brief Retrieve the component type of a metadata array
 */
template <typename T> struct MetadataArrayType;
template <typename T>
struct MetadataArrayType<CesiumGltf::StructuralMetadata::MetadataArrayView<T>> {
  using type = T;
};

/**
 * @brief Convert a C++ type to PropertyType and PropertyComponentType
 */
template <typename T> struct TypeToPropertyType;

#pragma region Scalar Property Types

template <> struct TypeToPropertyType<uint8_t> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint8;
  static constexpr PropertyType value = PropertyType::Scalar;
};

template <> struct TypeToPropertyType<int8_t> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int8;
  static constexpr PropertyType value = PropertyType::Scalar;
};

template <> struct TypeToPropertyType<uint16_t> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint16;
  static constexpr PropertyType value = PropertyType::Scalar;
};

template <> struct TypeToPropertyType<int16_t> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int16;
  static constexpr PropertyType value = PropertyType::Scalar;
};

template <> struct TypeToPropertyType<uint32_t> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint32;
  static constexpr PropertyType value = PropertyType::Scalar;
};

template <> struct TypeToPropertyType<int32_t> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int32;
  static constexpr PropertyType value = PropertyType::Scalar;
};

template <> struct TypeToPropertyType<uint64_t> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint64;
  static constexpr PropertyType value = PropertyType::Scalar;
};

template <> struct TypeToPropertyType<int64_t> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int64;
  static constexpr PropertyType value = PropertyType::Scalar;
};

template <> struct TypeToPropertyType<float> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Float32;
  static constexpr PropertyType value = PropertyType::Scalar;
};

template <> struct TypeToPropertyType<double> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Float64;
  static constexpr PropertyType value = PropertyType::Scalar;
};
#pragma endregion

#pragma region Vec2 Property Types

template <> struct TypeToPropertyType<glm::u8vec2> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint8;
  static constexpr PropertyType value = PropertyType::Vec2;
};

template <> struct TypeToPropertyType<glm::i8vec2> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int8;
  static constexpr PropertyType value = PropertyType::Vec2;
};

template <> struct TypeToPropertyType<glm::u16vec2> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint16;
  static constexpr PropertyType value = PropertyType::Vec2;
};

template <> struct TypeToPropertyType<glm::i16vec2> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int16;
  static constexpr PropertyType value = PropertyType::Vec2;
};

template <> struct TypeToPropertyType<glm::uvec2> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint32;
  static constexpr PropertyType value = PropertyType::Vec2;
};

template <> struct TypeToPropertyType<glm::ivec2> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int32;
  static constexpr PropertyType value = PropertyType::Vec2;
};

template <> struct TypeToPropertyType<glm::u64vec2> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint64;
  static constexpr PropertyType value = PropertyType::Vec2;
};

template <> struct TypeToPropertyType<glm::i64vec2> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int64;
  static constexpr PropertyType value = PropertyType::Vec2;
};

template <> struct TypeToPropertyType<glm::vec2> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Float32;
  static constexpr PropertyType value = PropertyType::Vec2;
};

template <> struct TypeToPropertyType<glm::dvec2> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Float64;
  static constexpr PropertyType value = PropertyType::Vec2;
};
#pragma endregion

#pragma region Vec3 Property Types

template <> struct TypeToPropertyType<glm::u8vec3> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint8;
  static constexpr PropertyType value = PropertyType::Vec3;
};

template <> struct TypeToPropertyType<glm::i8vec3> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int8;
  static constexpr PropertyType value = PropertyType::Vec3;
};

template <> struct TypeToPropertyType<glm::u16vec3> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint16;
  static constexpr PropertyType value = PropertyType::Vec3;
};

template <> struct TypeToPropertyType<glm::i16vec3> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int16;
  static constexpr PropertyType value = PropertyType::Vec3;
};

template <> struct TypeToPropertyType<glm::uvec3> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint32;
  static constexpr PropertyType value = PropertyType::Vec3;
};

template <> struct TypeToPropertyType<glm::ivec3> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int32;
  static constexpr PropertyType value = PropertyType::Vec3;
};

template <> struct TypeToPropertyType<glm::u64vec3> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint64;
  static constexpr PropertyType value = PropertyType::Vec3;
};

template <> struct TypeToPropertyType<glm::i64vec3> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int64;
  static constexpr PropertyType value = PropertyType::Vec3;
};

template <> struct TypeToPropertyType<glm::vec3> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Float32;
  static constexpr PropertyType value = PropertyType::Vec3;
};

template <> struct TypeToPropertyType<glm::dvec3> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Float64;
  static constexpr PropertyType value = PropertyType::Vec3;
};
#pragma endregion

#pragma region Vec4 Property Types

template <> struct TypeToPropertyType<glm::u8vec4> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint8;
  static constexpr PropertyType value = PropertyType::Vec4;
};

template <> struct TypeToPropertyType<glm::i8vec4> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int8;
  static constexpr PropertyType value = PropertyType::Vec4;
};

template <> struct TypeToPropertyType<glm::u16vec4> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint16;
  static constexpr PropertyType value = PropertyType::Vec4;
};

template <> struct TypeToPropertyType<glm::i16vec4> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int16;
  static constexpr PropertyType value = PropertyType::Vec4;
};

template <> struct TypeToPropertyType<glm::uvec4> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint32;
  static constexpr PropertyType value = PropertyType::Vec4;
};

template <> struct TypeToPropertyType<glm::ivec4> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int32;
  static constexpr PropertyType value = PropertyType::Vec4;
};

template <> struct TypeToPropertyType<glm::u64vec4> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint64;
  static constexpr PropertyType value = PropertyType::Vec4;
};

template <> struct TypeToPropertyType<glm::i64vec4> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int64;
  static constexpr PropertyType value = PropertyType::Vec4;
};

template <> struct TypeToPropertyType<glm::vec4> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Float32;
  static constexpr PropertyType value = PropertyType::Vec4;
};

template <> struct TypeToPropertyType<glm::dvec4> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Float64;
  static constexpr PropertyType value = PropertyType::Vec4;
};
#pragma endregion

#pragma region Mat2 Property Types

template <> struct TypeToPropertyType<glm::u8mat2x2> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint8;
  static constexpr PropertyType value = PropertyType::Mat2;
};

template <> struct TypeToPropertyType<glm::i8mat2x2> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int8;
  static constexpr PropertyType value = PropertyType::Mat2;
};

template <> struct TypeToPropertyType<glm::u16mat2x2> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint16;
  static constexpr PropertyType value = PropertyType::Mat2;
};

template <> struct TypeToPropertyType<glm::i16mat2x2> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int16;
  static constexpr PropertyType value = PropertyType::Mat2;
};

template <> struct TypeToPropertyType<glm::u32mat2x2> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint32;
  static constexpr PropertyType value = PropertyType::Mat2;
};

template <> struct TypeToPropertyType<glm::i32mat2x2> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int32;
  static constexpr PropertyType value = PropertyType::Mat2;
};

template <> struct TypeToPropertyType<glm::u64mat2x2> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint64;
  static constexpr PropertyType value = PropertyType::Mat2;
};

template <> struct TypeToPropertyType<glm::i64mat2x2> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int64;
  static constexpr PropertyType value = PropertyType::Mat2;
};

template <> struct TypeToPropertyType<glm::mat2> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Float32;
  static constexpr PropertyType value = PropertyType::Mat2;
};

template <> struct TypeToPropertyType<glm::dmat2> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Float64;
  static constexpr PropertyType value = PropertyType::Mat2;
};

#pragma endregion

#pragma region Mat3 Property Types

template <> struct TypeToPropertyType<glm::u8mat3x3> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint8;
  static constexpr PropertyType value = PropertyType::Mat3;
};

template <> struct TypeToPropertyType<glm::i8mat3x3> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int8;
  static constexpr PropertyType value = PropertyType::Mat3;
};

template <> struct TypeToPropertyType<glm::u16mat3x3> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint16;
  static constexpr PropertyType value = PropertyType::Mat3;
};

template <> struct TypeToPropertyType<glm::i16mat3x3> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int16;
  static constexpr PropertyType value = PropertyType::Mat3;
};

template <> struct TypeToPropertyType<glm::u32mat3x3> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint32;
  static constexpr PropertyType value = PropertyType::Mat3;
};

template <> struct TypeToPropertyType<glm::i32mat3x3> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int32;
  static constexpr PropertyType value = PropertyType::Mat3;
};

template <> struct TypeToPropertyType<glm::u64mat3x3> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint64;
  static constexpr PropertyType value = PropertyType::Mat3;
};

template <> struct TypeToPropertyType<glm::i64mat3x3> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int64;
  static constexpr PropertyType value = PropertyType::Mat3;
};

template <> struct TypeToPropertyType<glm::mat3> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Float32;
  static constexpr PropertyType value = PropertyType::Mat3;
};

template <> struct TypeToPropertyType<glm::dmat3> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Float64;
  static constexpr PropertyType value = PropertyType::Mat3;
};

#pragma endregion

#pragma region Mat4 Property Types

template <> struct TypeToPropertyType<glm::u8mat4x4> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint8;
  static constexpr PropertyType value = PropertyType::Mat4;
};

template <> struct TypeToPropertyType<glm::i8mat4x4> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int8;
  static constexpr PropertyType value = PropertyType::Mat4;
};

template <> struct TypeToPropertyType<glm::u16mat4x4> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint16;
  static constexpr PropertyType value = PropertyType::Mat4;
};

template <> struct TypeToPropertyType<glm::i16mat4x4> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int16;
  static constexpr PropertyType value = PropertyType::Mat4;
};

template <> struct TypeToPropertyType<glm::u32mat4x4> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint32;
  static constexpr PropertyType value = PropertyType::Mat4;
};

template <> struct TypeToPropertyType<glm::i32mat4x4> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int32;
  static constexpr PropertyType value = PropertyType::Mat4;
};

template <> struct TypeToPropertyType<glm::u64mat4x4> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Uint64;
  static constexpr PropertyType value = PropertyType::Mat4;
};

template <> struct TypeToPropertyType<glm::i64mat4x4> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Int64;
  static constexpr PropertyType value = PropertyType::Mat4;
};

template <> struct TypeToPropertyType<glm::mat4> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Float32;
  static constexpr PropertyType value = PropertyType::Mat4;
};

template <> struct TypeToPropertyType<glm::dmat4> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::Float64;
  static constexpr PropertyType value = PropertyType::Mat4;
};

#pragma endregion

template <> struct TypeToPropertyType<bool> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::None;
  static constexpr PropertyType value = PropertyType::Boolean;
};

template <> struct TypeToPropertyType<std::string_view> {
  static constexpr PropertyComponentType component =
      PropertyComponentType::None;
  static constexpr PropertyType value = PropertyType::String;
};

} // namespace StructuralMetadata
} // namespace CesiumGltf
