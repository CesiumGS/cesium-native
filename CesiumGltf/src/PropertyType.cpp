#include <CesiumGltf/AccessorSpec.h>
#include <CesiumGltf/ClassProperty.h>
#include <CesiumGltf/PropertyTableProperty.h>
#include <CesiumGltf/PropertyType.h>

#include <glm/detail/qualifier.hpp>

#include <cstddef>
#include <cstdint>
#include <string>

namespace CesiumGltf {
std::string convertPropertyTypeToString(PropertyType type) {
  switch (type) {
  case PropertyType::Scalar:
    return ClassProperty::Type::SCALAR;
  case PropertyType::Vec2:
    return ClassProperty::Type::VEC2;
  case PropertyType::Vec3:
    return ClassProperty::Type::VEC3;
  case PropertyType::Vec4:
    return ClassProperty::Type::VEC4;
  case PropertyType::Mat2:
    return ClassProperty::Type::MAT2;
  case PropertyType::Mat3:
    return ClassProperty::Type::MAT3;
  case PropertyType::Mat4:
    return ClassProperty::Type::MAT4;
  case PropertyType::Boolean:
    return ClassProperty::Type::BOOLEAN;
  case PropertyType::Enum:
    return ClassProperty::Type::ENUM;
  case PropertyType::String:
    return ClassProperty::Type::STRING;
  default:
    return "INVALID";
  }
}

PropertyType convertStringToPropertyType(const std::string& str) {
  if (str == ClassProperty::Type::SCALAR) {
    return PropertyType::Scalar;
  }

  if (str == ClassProperty::Type::VEC2) {
    return PropertyType::Vec2;
  }

  if (str == ClassProperty::Type::VEC3) {
    return PropertyType::Vec3;
  }

  if (str == ClassProperty::Type::VEC4) {
    return PropertyType::Vec4;
  }

  if (str == ClassProperty::Type::MAT2) {
    return PropertyType::Mat2;
  }

  if (str == ClassProperty::Type::MAT3) {
    return PropertyType::Mat3;
  }

  if (str == ClassProperty::Type::MAT4) {
    return PropertyType::Mat4;
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

  return PropertyType::Invalid;
}

std::string convertPropertyComponentTypeToString(PropertyComponentType type) {
  switch (type) {
  case PropertyComponentType::None:
    return "NONE";
  case PropertyComponentType::Uint8:
    return ClassProperty::ComponentType::UINT8;
  case PropertyComponentType::Int8:
    return ClassProperty::ComponentType::INT8;
  case PropertyComponentType::Uint16:
    return ClassProperty::ComponentType::UINT16;
  case PropertyComponentType::Int16:
    return ClassProperty::ComponentType::INT16;
  case PropertyComponentType::Uint32:
    return ClassProperty::ComponentType::UINT32;
  case PropertyComponentType::Int32:
    return ClassProperty::ComponentType::INT32;
  case PropertyComponentType::Uint64:
    return ClassProperty::ComponentType::UINT64;
  case PropertyComponentType::Int64:
    return ClassProperty::ComponentType::INT64;
  case PropertyComponentType::Float32:
    return ClassProperty::ComponentType::FLOAT32;
  case PropertyComponentType::Float64:
    return ClassProperty::ComponentType::FLOAT64;
  default:
    return "NONE";
  }
}

PropertyComponentType
convertStringToPropertyComponentType(const std::string& str) {
  if (str == ClassProperty::ComponentType::UINT8) {
    return PropertyComponentType::Uint8;
  }

  if (str == ClassProperty::ComponentType::INT8) {
    return PropertyComponentType::Int8;
  }

  if (str == ClassProperty::ComponentType::UINT16) {
    return PropertyComponentType::Uint16;
  }

  if (str == ClassProperty::ComponentType::INT16) {
    return PropertyComponentType::Int16;
  }

  if (str == ClassProperty::ComponentType::UINT32) {
    return PropertyComponentType::Uint32;
  }

  if (str == ClassProperty::ComponentType::INT32) {
    return PropertyComponentType::Int32;
  }

  if (str == ClassProperty::ComponentType::UINT64) {
    return PropertyComponentType::Uint64;
  }

  if (str == ClassProperty::ComponentType::INT64) {
    return PropertyComponentType::Int64;
  }

  if (str == ClassProperty::ComponentType::FLOAT32) {
    return PropertyComponentType::Float32;
  }

  if (str == ClassProperty::ComponentType::FLOAT64) {
    return PropertyComponentType::Float64;
  }

  return PropertyComponentType::None;
}

PropertyComponentType
convertArrayOffsetTypeStringToPropertyComponentType(const std::string& str) {
  if (str == PropertyTableProperty::ArrayOffsetType::UINT8) {
    return PropertyComponentType::Uint8;
  }

  if (str == PropertyTableProperty::ArrayOffsetType::UINT16) {
    return PropertyComponentType::Uint16;
  }

  if (str == PropertyTableProperty::ArrayOffsetType::UINT32) {
    return PropertyComponentType::Uint32;
  }

  if (str == PropertyTableProperty::ArrayOffsetType::UINT64) {
    return PropertyComponentType::Uint64;
  }

  return PropertyComponentType::None;
}

PropertyComponentType
convertStringOffsetTypeStringToPropertyComponentType(const std::string& str) {
  if (str == PropertyTableProperty::StringOffsetType::UINT8) {
    return PropertyComponentType::Uint8;
  }

  if (str == PropertyTableProperty::StringOffsetType::UINT16) {
    return PropertyComponentType::Uint16;
  }

  if (str == PropertyTableProperty::StringOffsetType::UINT32) {
    return PropertyComponentType::Uint32;
  }

  if (str == PropertyTableProperty::StringOffsetType::UINT64) {
    return PropertyComponentType::Uint64;
  }

  return PropertyComponentType::None;
}

PropertyComponentType
convertAccessorComponentTypeToPropertyComponentType(int componentType) {
  switch (componentType) {
  case AccessorSpec::ComponentType::BYTE:
    return PropertyComponentType::Int8;
  case AccessorSpec::ComponentType::UNSIGNED_BYTE:
    return PropertyComponentType::Uint8;
  case AccessorSpec::ComponentType::SHORT:
    return PropertyComponentType::Int16;
  case AccessorSpec::ComponentType::UNSIGNED_SHORT:
    return PropertyComponentType::Uint16;
  case AccessorSpec::ComponentType::INT:
    return PropertyComponentType::Int32;
  case AccessorSpec::ComponentType::UNSIGNED_INT:
    return PropertyComponentType::Uint32;
  case AccessorSpec::ComponentType::INT64:
    return PropertyComponentType::Int64;
  case AccessorSpec::ComponentType::UNSIGNED_INT64:
    return PropertyComponentType::Uint64;
  case AccessorSpec::ComponentType::FLOAT:
    return PropertyComponentType::Float32;
  case AccessorSpec::ComponentType::DOUBLE:
    return PropertyComponentType::Float64;
  default:
    return PropertyComponentType::None;
  }
}

int32_t convertPropertyComponentTypeToAccessorComponentType(
    PropertyComponentType componentType) {
  switch (componentType) {
  case PropertyComponentType::Int8:
    return AccessorSpec::ComponentType::BYTE;
  case PropertyComponentType::Uint8:
    return AccessorSpec::ComponentType::UNSIGNED_BYTE;
  case PropertyComponentType::Int16:
    return AccessorSpec::ComponentType::SHORT;
  case PropertyComponentType::Uint16:
    return AccessorSpec::ComponentType::UNSIGNED_SHORT;
  case PropertyComponentType::Int32:
    return AccessorSpec::ComponentType::INT;
  case PropertyComponentType::Uint32:
    return AccessorSpec::ComponentType::UNSIGNED_INT;
  case PropertyComponentType::Int64:
    return AccessorSpec::ComponentType::INT64;
  case PropertyComponentType::Uint64:
    return AccessorSpec::ComponentType::UNSIGNED_INT64;
  case PropertyComponentType::Float32:
    return AccessorSpec::ComponentType::FLOAT;
  case PropertyComponentType::Float64:
    return AccessorSpec::ComponentType::DOUBLE;
  default:
    return -1;
  }
}

bool isPropertyTypeVecN(PropertyType type) {
  return type == PropertyType::Vec2 || type == PropertyType::Vec3 ||
         type == PropertyType::Vec4;
}

bool isPropertyTypeMatN(PropertyType type) {
  return type == PropertyType::Mat2 || type == PropertyType::Mat3 ||
         type == PropertyType::Mat4;
}

bool isPropertyComponentTypeInteger(PropertyComponentType componentType) {
  return componentType == PropertyComponentType::Int8 ||
         componentType == PropertyComponentType::Uint8 ||
         componentType == PropertyComponentType::Int16 ||
         componentType == PropertyComponentType::Uint16 ||
         componentType == PropertyComponentType::Int32 ||
         componentType == PropertyComponentType::Uint32 ||
         componentType == PropertyComponentType::Int64 ||
         componentType == PropertyComponentType::Uint64;
}

glm::length_t getDimensionsFromPropertyType(PropertyType type) {
  switch (type) {
  case PropertyType::Scalar:
    return 1;
  case PropertyType::Vec2:
  case PropertyType::Mat2:
    return 2;
  case PropertyType::Vec3:
  case PropertyType::Mat3:
    return 3;
  case PropertyType::Vec4:
  case PropertyType::Mat4:
    return 4;
  default:
    return 0;
  }
}

glm::length_t getComponentCountFromPropertyType(PropertyType type) {
  switch (type) {
  case PropertyType::Scalar:
    return 1;
  case PropertyType::Vec2:
    return 2;
  case PropertyType::Vec3:
    return 3;
  case PropertyType::Vec4:
  case PropertyType::Mat2:
    return 4;
  case PropertyType::Mat3:
    return 9;
  case PropertyType::Mat4:
    return 16;
  default:
    return 0;
  }
}

size_t getSizeOfComponentType(PropertyComponentType componentType) {
  switch (componentType) {
  case PropertyComponentType::Int8:
  case PropertyComponentType::Uint8:
    return 1;
  case PropertyComponentType::Int16:
  case PropertyComponentType::Uint16:
    return 2;
  case PropertyComponentType::Int32:
  case PropertyComponentType::Uint32:
  case PropertyComponentType::Float32:
    return 4;
  case PropertyComponentType::Int64:
  case PropertyComponentType::Uint64:
  case PropertyComponentType::Float64:
    return 8;
  default:
    return 0;
  }
}
} // namespace CesiumGltf
