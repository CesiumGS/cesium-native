#pragma once

#include "CesiumGltf/MetadataArrayView.h"
#include "CesiumGltf/PropertyType.h"

#include <cstdint>
#include <type_traits>

namespace CesiumGltf {
/**
 * @brief Check if a C++ type can be represented as a numeric property type
 */
template <typename... T> struct IsMetadataNumeric;
template <typename T> struct IsMetadataNumeric<T> : std::false_type {};
template <> struct IsMetadataNumeric<uint8_t> : std::true_type {};
template <> struct IsMetadataNumeric<int8_t> : std::true_type {};
template <> struct IsMetadataNumeric<uint16_t> : std::true_type {};
template <> struct IsMetadataNumeric<int16_t> : std::true_type {};
template <> struct IsMetadataNumeric<uint32_t> : std::true_type {};
template <> struct IsMetadataNumeric<int32_t> : std::true_type {};
template <> struct IsMetadataNumeric<uint64_t> : std::true_type {};
template <> struct IsMetadataNumeric<int64_t> : std::true_type {};
template <> struct IsMetadataNumeric<float> : std::true_type {};
template <> struct IsMetadataNumeric<double> : std::true_type {};

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
 * type
 */
template <typename... T> struct IsMetadataFloating;
template <typename T> struct IsMetadataFloating<T> : std::false_type {};
template <> struct IsMetadataFloating<float> : std::true_type {};
template <> struct IsMetadataFloating<double> : std::true_type {};

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
 * @brief Check if a C++ type can be represented as an array of number property
 * type
 */
template <typename... T> struct IsMetadataNumericArray;
template <typename T> struct IsMetadataNumericArray<T> : std::false_type {};
template <typename T> struct IsMetadataNumericArray<MetadataArrayView<T>> {
  static constexpr bool value = IsMetadataNumeric<T>::value;
};

/**
 * @brief Check if a C++ type can be represented as an array of boolean property
 * type
 */
template <typename... T> struct IsMetadataBooleanArray;
template <typename T> struct IsMetadataBooleanArray<T> : std::false_type {};
template <>
struct IsMetadataBooleanArray<MetadataArrayView<bool>> : std::true_type {};

/**
 * @brief Check if a C++ type can be represented as an array of string property
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
struct MetadataArrayType<CesiumGltf::MetadataArrayView<T>> {
  using type = T;
};

/**
 * @brief Convert a C++ type to PropertyType
 */
template <typename T> struct TypeToPropertyType;

template <> struct TypeToPropertyType<uint8_t> {
  static constexpr PropertyType component = PropertyType::None;
  static constexpr PropertyType value = PropertyType::Uint8;
};

template <> struct TypeToPropertyType<int8_t> {
  static constexpr PropertyType component = PropertyType::None;
  static constexpr PropertyType value = PropertyType::Int8;
};

template <> struct TypeToPropertyType<uint16_t> {
  static constexpr PropertyType component = PropertyType::None;
  static constexpr PropertyType value = PropertyType::Uint16;
};

template <> struct TypeToPropertyType<int16_t> {
  static constexpr PropertyType component = PropertyType::None;
  static constexpr PropertyType value = PropertyType::Int16;
};

template <> struct TypeToPropertyType<uint32_t> {
  static constexpr PropertyType component = PropertyType::None;
  static constexpr PropertyType value = PropertyType::Uint32;
};

template <> struct TypeToPropertyType<int32_t> {
  static constexpr PropertyType component = PropertyType::None;
  static constexpr PropertyType value = PropertyType::Int32;
};

template <> struct TypeToPropertyType<uint64_t> {
  static constexpr PropertyType component = PropertyType::None;
  static constexpr PropertyType value = PropertyType::Uint64;
};

template <> struct TypeToPropertyType<int64_t> {
  static constexpr PropertyType component = PropertyType::None;
  static constexpr PropertyType value = PropertyType::Int64;
};

template <> struct TypeToPropertyType<float> {
  static constexpr PropertyType component = PropertyType::None;
  static constexpr PropertyType value = PropertyType::Float32;
};

template <> struct TypeToPropertyType<double> {
  static constexpr PropertyType component = PropertyType::None;
  static constexpr PropertyType value = PropertyType::Float64;
};

template <> struct TypeToPropertyType<bool> {
  static constexpr PropertyType component = PropertyType::None;
  static constexpr PropertyType value = PropertyType::Boolean;
};

template <> struct TypeToPropertyType<std::string_view> {
  static constexpr PropertyType component = PropertyType::None;
  static constexpr PropertyType value = PropertyType::String;
};

template <typename T>
struct TypeToPropertyType<CesiumGltf::MetadataArrayView<T>> {
  static constexpr PropertyType component = TypeToPropertyType<T>::value;
  static constexpr PropertyType value = PropertyType::Array;
};

} // namespace CesiumGltf
