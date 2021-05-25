#pragma once

#include <cstdint>
#include <gsl/span>
#include <string>
#include <string_view>

namespace CesiumGltf {
enum class PropertyType {
  None = 1 << 0,
  Uint8 = 1 << 1,
  Int8 = 1 << 2,
  Uint16 = 1 << 3,
  Int16 = 1 << 4,
  Uint32 = 1 << 5,
  Int32 = 1 << 6,
  Uint64 = 1 << 7,
  Int64 = 1 << 8,
  Float32 = 1 << 9,
  Float64 = 1 << 10,
  Boolean = 1 << 11,
  String = 1 << 12,
  Enum = 1 << 13,
  Array = 1 << 14,
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

template <typename T> struct TypeToPropertyType<gsl::span<const T>> {
  static constexpr uint32_t value =
      static_cast<uint32_t>(PropertyType::Array) | TypeToPropertyType<T>::value;
};

inline std::string convertProperttTypeToString(CesiumGltf::PropertyType type) {
  switch (type) {
  case PropertyType::None:
    return "NONE";
  case PropertyType::Uint8:
    return "UINT8";
  case PropertyType::Int8:
    return "INT8";
  case PropertyType::Uint16:
    return "UINT16";
  case PropertyType::Int16:
    return "INT16";
  case PropertyType::Uint32:
    return "UINT32";
  case PropertyType::Int32:
    return "INT32";
  case PropertyType::Uint64:
    return "UINT64";
  case PropertyType::Int64:
    return "INT64";
  case PropertyType::Float32:
    return "FLOAT32";
  case PropertyType::Float64:
    return "FLOAT64";
  case PropertyType::Boolean:
    return "BOOLEAN";
  case PropertyType::Enum:
    return "ENUM";
  case PropertyType::String:
    return "STRING";
  case PropertyType::Array:
    return "ARRAY";
  default:
    return "NONE";
  }
}

inline PropertyType convertStringToPropertyType(const std::string& str) {
  if (str == "UINT8") {
    return PropertyType::Uint8;
  }

  if (str == "INT8") {
    return PropertyType::Int8;
  }

  if (str == "UINT16") {
    return PropertyType::Uint16;
  }

  if (str == "INT16") {
    return PropertyType::Int16;
  }

  if (str == "UINT32") {
    return PropertyType::Uint32;
  }

  if (str == "INT32") {
    return PropertyType::Int32;
  }

  if (str == "UINT64") {
    return PropertyType::Uint64;
  }

  if (str == "INT64") {
    return PropertyType::Int64;
  }

  if (str == "FLOAT32") {
    return PropertyType::Float32;
  }

  if (str == "FLOAT64") {
    return PropertyType::Float64;
  }

  if (str == "BOOLEAN") {
    return PropertyType::Boolean;
  }

  if (str == "STRING") {
    return PropertyType::String;
  }

  if (str == "ENUM") {
    return PropertyType::Enum;
  }

  if (str == "ARRAY") {
    return PropertyType::Array;
  }

  return PropertyType::None;
}
} // namespace CesiumGltf