#include "CesiumGltf/PropertyType.h"

#include "CesiumGltf/ClassProperty.h"
#include "CesiumGltf/FeatureTable.h"

namespace CesiumGltf {
std::string convertPropertyTypeToString(CesiumGltf::PropertyType type) {
  switch (type) {
  case PropertyType::None:
    return "NONE";
  case PropertyType::Uint8:
    return ClassProperty::Type::UINT8;
  case PropertyType::Int8:
    return ClassProperty::Type::INT8;
  case PropertyType::Uint16:
    return ClassProperty::Type::UINT16;
  case PropertyType::Int16:
    return ClassProperty::Type::INT16;
  case PropertyType::Uint32:
    return ClassProperty::Type::UINT32;
  case PropertyType::Int32:
    return ClassProperty::Type::INT32;
  case PropertyType::Uint64:
    return ClassProperty::Type::UINT64;
  case PropertyType::Int64:
    return ClassProperty::Type::INT64;
  case PropertyType::Float32:
    return ClassProperty::Type::FLOAT32;
  case PropertyType::Float64:
    return ClassProperty::Type::FLOAT64;
  case PropertyType::Boolean:
    return ClassProperty::Type::BOOLEAN;
  case PropertyType::Enum:
    return ClassProperty::Type::ENUM;
  case PropertyType::String:
    return ClassProperty::Type::STRING;
  case PropertyType::Array:
    return ClassProperty::Type::ARRAY;
  default:
    return "NONE";
  }
}

PropertyType convertStringToPropertyType(const std::string& str) {
  if (str == ClassProperty::Type::UINT8) {
    return PropertyType::Uint8;
  }

  if (str == ClassProperty::Type::INT8) {
    return PropertyType::Int8;
  }

  if (str == ClassProperty::Type::UINT16) {
    return PropertyType::Uint16;
  }

  if (str == ClassProperty::Type::INT16) {
    return PropertyType::Int16;
  }

  if (str == ClassProperty::Type::UINT32) {
    return PropertyType::Uint32;
  }

  if (str == ClassProperty::Type::INT32) {
    return PropertyType::Int32;
  }

  if (str == ClassProperty::Type::UINT64) {
    return PropertyType::Uint64;
  }

  if (str == ClassProperty::Type::INT64) {
    return PropertyType::Int64;
  }

  if (str == ClassProperty::Type::FLOAT32) {
    return PropertyType::Float32;
  }

  if (str == ClassProperty::Type::FLOAT64) {
    return PropertyType::Float64;
  }

  if (str == ClassProperty::Type::BOOLEAN) {
    return PropertyType::Boolean;
  }

  if (str == ClassProperty::Type::STRING) {
    return PropertyType::String;
  }

  if (str == ClassProperty::Type::ENUM) {
    return PropertyType::Enum;
  }

  if (str == ClassProperty::Type::ARRAY) {
    return PropertyType::Array;
  }

  return PropertyType::None;
}

PropertyType convertOffsetStringToPropertyType(const std::string& str) {
  if (str == FeatureTableProperty::OffsetType::UINT8) {
    return PropertyType::Uint8;
  }

  if (str == FeatureTableProperty::OffsetType::UINT16) {
    return PropertyType::Uint16;
  }

  if (str == FeatureTableProperty::OffsetType::UINT32) {
    return PropertyType::Uint32;
  }

  if (str == FeatureTableProperty::OffsetType::UINT64) {
    return PropertyType::Uint64;
  }

  return PropertyType::None;
}
} // namespace CesiumGltf
