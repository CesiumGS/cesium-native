#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace CesiumGltf {
enum class PropertyType {
  None = 0,
  Int8 = 1 << 0,
  Uint8 = 1 << 1,
  Int16 = 1 << 2,
  Uint16 = 1 << 3,
  Int32 = 1 << 4,
  Uint32 = 1 << 5,
  Int64 = 1 << 6,
  Uint64 = 1 << 7,
  Float32 = 1 << 8,
  Float64 = 1 << 9,
  Boolean = 1 << 10,
  Enum = 1 << 11,
  String = 1 << 12,
  Array = 1 << 13,
};

std::string convertPropertyTypeToString(CesiumGltf::PropertyType type);

PropertyType convertStringToPropertyType(const std::string& str);

PropertyType convertOffsetStringToPropertyType(const std::string& str);
} // namespace CesiumGltf
