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
  static constexpr uint32_t value = static_cast<uint32_t>(PropertyType::Uint8);
};

template <> struct TypeToPropertyType<int8_t> {
  static constexpr uint32_t value = static_cast<uint32_t>(PropertyType::Int8);
};

template <> struct TypeToPropertyType<uint16_t> {
  static constexpr uint32_t value = static_cast<uint32_t>(PropertyType::Uint16);
};

template <> struct TypeToPropertyType<int16_t> {
  static constexpr uint32_t value = static_cast<uint32_t>(PropertyType::Int16);
};

template <> struct TypeToPropertyType<uint32_t> {
  static constexpr uint32_t value = static_cast<uint32_t>(PropertyType::Uint32);
};

template <> struct TypeToPropertyType<int32_t> {
  static constexpr uint32_t value = static_cast<uint32_t>(PropertyType::Int32);
};

template <> struct TypeToPropertyType<uint64_t> {
  static constexpr uint32_t value = static_cast<uint32_t>(PropertyType::Uint64);
};

template <> struct TypeToPropertyType<int64_t> {
  static constexpr uint32_t value = static_cast<uint32_t>(PropertyType::Int64);
};

template <> struct TypeToPropertyType<float> {
  static constexpr uint32_t value =
      static_cast<uint32_t>(PropertyType::Float32);
};

template <> struct TypeToPropertyType<double> {
  static constexpr uint32_t value =
      static_cast<uint32_t>(PropertyType::Float64);
};

template <> struct TypeToPropertyType<bool> {
  static constexpr uint32_t value =
      static_cast<uint32_t>(PropertyType::Boolean);
};

template <> struct TypeToPropertyType<std::string_view> {
  static constexpr uint32_t value = static_cast<uint32_t>(PropertyType::String);
};

template <typename T>
struct TypeToPropertyType<CesiumGltf::MetadataArrayView<T>> {
  static constexpr uint32_t value =
      static_cast<uint32_t>(PropertyType::Array) | TypeToPropertyType<T>::value;
};

} // namespace CesiumGltf
