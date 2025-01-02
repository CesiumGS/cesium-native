#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>

namespace Cesium3DTilesContent {
struct MetadataProperty {
public:
  enum ComponentType {
    BYTE,
    UNSIGNED_BYTE,
    SHORT,
    UNSIGNED_SHORT,
    INT,
    UNSIGNED_INT,
    FLOAT,
    DOUBLE
  };

  enum Type { SCALAR, VEC2, VEC3, VEC4 };

  static size_t getSizeOfComponentType(ComponentType componentType) {
    switch (componentType) {
    case ComponentType::BYTE:
    case ComponentType::UNSIGNED_BYTE:
      return sizeof(uint8_t);
    case ComponentType::SHORT:
    case ComponentType::UNSIGNED_SHORT:
      return sizeof(uint16_t);
    case ComponentType::INT:
    case ComponentType::UNSIGNED_INT:
      return sizeof(uint32_t);
    case ComponentType::FLOAT:
      return sizeof(float);
    case ComponentType::DOUBLE:
      return sizeof(double);
    default:
      return 0;
    }
  };

  static std::optional<Type>
  getTypeFromNumberOfComponents(int8_t numComponents) {
    switch (numComponents) {
    case 1:
      return Type::SCALAR;
    case 2:
      return Type::VEC2;
    case 3:
      return Type::VEC3;
    case 4:
      return Type::VEC4;
    default:
      return std::nullopt;
    }
  }
  static const std::map<std::string, MetadataProperty::ComponentType>
      stringToMetadataComponentType;
  static const std::map<std::string, MetadataProperty::Type>
      stringToMetadataType;
};
} // namespace Cesium3DTilesContent
