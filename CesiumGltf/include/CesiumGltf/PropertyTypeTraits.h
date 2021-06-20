#pragma once

#include "CesiumGltf/MetadataArrayView.h"
#include "CesiumGltf/PropertyType.h"
#include <cstdint>
#include <type_traits>

namespace CesiumGltf {
template <typename... T> struct IsNumeric;
template <typename T> struct IsNumeric<T> : std::false_type {};
template <> struct IsNumeric<uint8_t> : std::true_type {};
template <> struct IsNumeric<int8_t> : std::true_type {};
template <> struct IsNumeric<uint16_t> : std::true_type {};
template <> struct IsNumeric<int16_t> : std::true_type {};
template <> struct IsNumeric<uint32_t> : std::true_type {};
template <> struct IsNumeric<int32_t> : std::true_type {};
template <> struct IsNumeric<uint64_t> : std::true_type {};
template <> struct IsNumeric<int64_t> : std::true_type {};
template <> struct IsNumeric<float> : std::true_type {};
template <> struct IsNumeric<double> : std::true_type {};

template <typename... T> struct IsBoolean;
template <typename T> struct IsBoolean<T> : std::false_type {};
template <> struct IsBoolean<bool> : std::true_type {};

template <typename... T> struct IsString;
template <typename T> struct IsString<T> : std::false_type {};
template <> struct IsString<std::string_view> : std::true_type {};

template <typename... T> struct IsNumericArray;
template <typename T> struct IsNumericArray<T> : std::false_type {};
template <typename T> struct IsNumericArray<MetadataArrayView<T>> {
  static constexpr bool value = IsNumeric<T>::value;
};

template <typename... T> struct IsBooleanArray;
template <typename T> struct IsBooleanArray<T> : std::false_type {};
template <> struct IsBooleanArray<MetadataArrayView<bool>> : std::true_type {};

template <typename... T> struct IsStringArray;
template <typename T> struct IsStringArray<T> : std::false_type {};
template <>
struct IsStringArray<MetadataArrayView<std::string_view>> : std::true_type {};

template <typename T> struct ArrayType;
template <typename T> struct ArrayType<CesiumGltf::MetadataArrayView<T>> {
  using type = T;
};

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

template <typename T> struct TypeToPropertyType<CesiumGltf::MetadataArrayView<T>> {
  static constexpr uint32_t value =
      static_cast<uint32_t>(PropertyType::Array) | TypeToPropertyType<T>::value;
};

} // namespace CesiumGltf
