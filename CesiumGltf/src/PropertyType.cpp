#include "CesiumGltf/PropertyType.h"

#include "CesiumGltf/ExtensionExtFeatureMetadataClassProperty.h"
#include "CesiumGltf/ExtensionExtFeatureMetadataFeatureTable.h"

namespace CesiumGltf {
std::string convertPropertyTypeToString(CesiumGltf::PropertyType type) {
  switch (type) {
  case PropertyType::None:
    return "NONE";
  case PropertyType::Uint8:
    return ExtensionExtFeatureMetadataClassProperty::Type::UINT8;
  case PropertyType::Int8:
    return ExtensionExtFeatureMetadataClassProperty::Type::INT8;
  case PropertyType::Uint16:
    return ExtensionExtFeatureMetadataClassProperty::Type::UINT16;
  case PropertyType::Int16:
    return ExtensionExtFeatureMetadataClassProperty::Type::INT16;
  case PropertyType::Uint32:
    return ExtensionExtFeatureMetadataClassProperty::Type::UINT32;
  case PropertyType::Int32:
    return ExtensionExtFeatureMetadataClassProperty::Type::INT32;
  case PropertyType::Uint64:
    return ExtensionExtFeatureMetadataClassProperty::Type::UINT64;
  case PropertyType::Int64:
    return ExtensionExtFeatureMetadataClassProperty::Type::INT64;
  case PropertyType::Float32:
    return ExtensionExtFeatureMetadataClassProperty::Type::FLOAT32;
  case PropertyType::Float64:
    return ExtensionExtFeatureMetadataClassProperty::Type::FLOAT64;
  case PropertyType::Boolean:
    return ExtensionExtFeatureMetadataClassProperty::Type::BOOLEAN;
  case PropertyType::Enum:
    return ExtensionExtFeatureMetadataClassProperty::Type::ENUM;
  case PropertyType::String:
    return ExtensionExtFeatureMetadataClassProperty::Type::STRING;
  case PropertyType::Array:
    return ExtensionExtFeatureMetadataClassProperty::Type::ARRAY;
  default:
    return "NONE";
  }
}

PropertyType convertStringToPropertyType(const std::string& str) {
  if (str == ExtensionExtFeatureMetadataClassProperty::Type::UINT8) {
    return PropertyType::Uint8;
  }

  if (str == ExtensionExtFeatureMetadataClassProperty::Type::INT8) {
    return PropertyType::Int8;
  }

  if (str == ExtensionExtFeatureMetadataClassProperty::Type::UINT16) {
    return PropertyType::Uint16;
  }

  if (str == ExtensionExtFeatureMetadataClassProperty::Type::INT16) {
    return PropertyType::Int16;
  }

  if (str == ExtensionExtFeatureMetadataClassProperty::Type::UINT32) {
    return PropertyType::Uint32;
  }

  if (str == ExtensionExtFeatureMetadataClassProperty::Type::INT32) {
    return PropertyType::Int32;
  }

  if (str == ExtensionExtFeatureMetadataClassProperty::Type::UINT64) {
    return PropertyType::Uint64;
  }

  if (str == ExtensionExtFeatureMetadataClassProperty::Type::INT64) {
    return PropertyType::Int64;
  }

  if (str == ExtensionExtFeatureMetadataClassProperty::Type::FLOAT32) {
    return PropertyType::Float32;
  }

  if (str == ExtensionExtFeatureMetadataClassProperty::Type::FLOAT64) {
    return PropertyType::Float64;
  }

  if (str == ExtensionExtFeatureMetadataClassProperty::Type::BOOLEAN) {
    return PropertyType::Boolean;
  }

  if (str == ExtensionExtFeatureMetadataClassProperty::Type::STRING) {
    return PropertyType::String;
  }

  if (str == ExtensionExtFeatureMetadataClassProperty::Type::ENUM) {
    return PropertyType::Enum;
  }

  if (str == ExtensionExtFeatureMetadataClassProperty::Type::ARRAY) {
    return PropertyType::Array;
  }

  return PropertyType::None;
}

PropertyType convertOffsetStringToPropertyType(const std::string& str) {
  if (str ==
      ExtensionExtFeatureMetadataFeatureTableProperty::OffsetType::UINT8) {
    return PropertyType::Uint8;
  }

  if (str ==
      ExtensionExtFeatureMetadataFeatureTableProperty::OffsetType::UINT16) {
    return PropertyType::Uint16;
  }

  if (str ==
      ExtensionExtFeatureMetadataFeatureTableProperty::OffsetType::UINT32) {
    return PropertyType::Uint32;
  }

  if (str ==
      ExtensionExtFeatureMetadataFeatureTableProperty::OffsetType::UINT64) {
    return PropertyType::Uint64;
  }

  return PropertyType::None;
}
} // namespace CesiumGltf
