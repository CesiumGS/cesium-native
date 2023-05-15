#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace CesiumGltf {
namespace StructuralMetadata {

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

std::string
convertPropertyTypeToString(CesiumGltf::StructuralMetadata::PropertyType type);

CesiumGltf::StructuralMetadata::PropertyType
convertStringToPropertyType(const std::string& str);

std::string convertPropertyComponentTypeToString(
    CesiumGltf::StructuralMetadata::PropertyComponentType componentType);

CesiumGltf::StructuralMetadata::PropertyComponentType
convertStringToPropertyComponentType(const std::string& str);

CesiumGltf::StructuralMetadata::PropertyComponentType
convertArrayOffsetTypeStringToPropertyComponentType(const std::string& str);

CesiumGltf::StructuralMetadata::PropertyComponentType
convertStringOffsetTypeStringToPropertyComponentType(const std::string& str);

} // namespace StructuralMetadata
} // namespace CesiumGltf
