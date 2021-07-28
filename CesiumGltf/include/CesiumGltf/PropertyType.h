#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace CesiumGltf {
enum class PropertyType {
  None,
  Int8,
  Uint8,
  Int16,
  Uint16,
  Int32,
  Uint32,
  Int64,
  Uint64,
  Float32,
  Float64,
  Boolean,
  Enum,
  String,
  Array,
};

std::string convertPropertyTypeToString(CesiumGltf::PropertyType type);

PropertyType convertStringToPropertyType(const std::string& str);

PropertyType convertOffsetStringToPropertyType(const std::string& str);
} // namespace CesiumGltf
