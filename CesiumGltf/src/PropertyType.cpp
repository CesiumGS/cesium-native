#include "CesiumGltf/PropertyType.h"

namespace CesiumGltf {
std::string convertPropertyTypeToString(CesiumGltf::PropertyType type) {
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

PropertyType convertStringToPropertyType(const std::string& str) {
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

PropertyType convertOffsetStringToPropertyType(const std::string& str) {
  if (str == "UINT8") {
    return PropertyType::Uint8;
  }

  if (str == "UINT16") {
    return PropertyType::Uint16;
  }

  if (str == "UINT32") {
    return PropertyType::Uint32;
  }

  if (str == "UINT64") {
    return PropertyType::Uint64;
  }

  return PropertyType::None;
}
} // namespace CesiumGltf
