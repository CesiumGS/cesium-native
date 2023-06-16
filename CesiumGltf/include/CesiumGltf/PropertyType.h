#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace CesiumGltf {
enum class PropertyType {
  Invalid,
  Scalar,
  Vec2,
  Vec3,
  Vec4,
  Mat2,
  Mat3,
  Mat4,
  String,
  Boolean,
  Enum
};

enum class PropertyComponentType {
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
};

std::string convertPropertyTypeToString(PropertyType type);

PropertyType convertStringToPropertyType(const std::string& str);

std::string
convertPropertyComponentTypeToString(PropertyComponentType componentType);

PropertyComponentType
convertStringToPropertyComponentType(const std::string& str);

PropertyComponentType
convertArrayOffsetTypeStringToPropertyComponentType(const std::string& str);

PropertyComponentType
convertStringOffsetTypeStringToPropertyComponentType(const std::string& str);

bool isPropertyTypeVecN(PropertyType type);

bool isPropertyTypeMatN(PropertyType type);
} // namespace CesiumGltf
