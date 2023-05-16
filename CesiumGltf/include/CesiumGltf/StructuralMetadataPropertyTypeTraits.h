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
template <int n, typename T, glm::qualifier P>
struct IsMetadataVecN<glm::vec<n, T, P>> : IsMetadataScalar<T> {};

/**
 * @brief Check if a C++ type can be represented as a matN type.
 */
template <typename... T> struct IsMetadataMatN;
template <typename T> struct IsMetadataMatN<T> : std::false_type {};
template <int n, typename T, glm::qualifier P>
struct IsMetadataMatN<glm::mat<n, n, T, P>> : IsMetadataScalar<T> {};

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

#pragma region Vector Property Types

template <typename T, glm::qualifier P>
struct TypeToPropertyType<glm::vec<2, T, P>> {
  static constexpr PropertyComponentType component =
      TypeToPropertyType<T>::component;
  static constexpr PropertyType value = PropertyType::Vec2;
};

template <typename T, glm::qualifier P>
struct TypeToPropertyType<glm::vec<3, T, P>> {
  static constexpr PropertyComponentType component =
      TypeToPropertyType<T>::component;
  static constexpr PropertyType value = PropertyType::Vec3;
};

template <typename T, glm::qualifier P>
struct TypeToPropertyType<glm::vec<4, T, P>> {
  static constexpr PropertyComponentType component =
      TypeToPropertyType<T>::component;
  static constexpr PropertyType value = PropertyType::Vec4;
};

#pragma endregion

#pragma region Matrix Property Types

template <typename T, glm::qualifier P>
struct TypeToPropertyType<glm::mat<2, 2, T, P>> {
  static constexpr PropertyComponentType component =
      TypeToPropertyType<T>::component;
  static constexpr PropertyType value = PropertyType::Mat2;
};

template <typename T, glm::qualifier P>
struct TypeToPropertyType<glm::mat<3, 3, T, P>> {
  static constexpr PropertyComponentType component =
      TypeToPropertyType<T>::component;
  static constexpr PropertyType value = PropertyType::Mat3;
};

template <typename T, glm::qualifier P>
struct TypeToPropertyType<glm::mat<4, 4, T, P>> {
  static constexpr PropertyComponentType component =
      TypeToPropertyType<T>::component;
  static constexpr PropertyType value = PropertyType::Mat4;
};

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

#pragma endregion

} // namespace StructuralMetadata
} // namespace CesiumGltf
