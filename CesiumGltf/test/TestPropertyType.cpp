#include <CesiumGltf/AccessorSpec.h>
#include <CesiumGltf/ClassProperty.h>
#include <CesiumGltf/PropertyTableProperty.h>
#include <CesiumGltf/PropertyType.h>

#include <doctest/doctest.h>

#include <cstdint>

using namespace CesiumGltf;

TEST_CASE("Test convertStringToPropertyType") {
  REQUIRE(
      convertStringToPropertyType(ClassProperty::Type::SCALAR) ==
      PropertyType::Scalar);

  REQUIRE(
      convertStringToPropertyType(ClassProperty::Type::VEC2) ==
      PropertyType::Vec2);

  REQUIRE(
      convertStringToPropertyType(ClassProperty::Type::VEC3) ==
      PropertyType::Vec3);

  REQUIRE(
      convertStringToPropertyType(ClassProperty::Type::VEC4) ==
      PropertyType::Vec4);

  REQUIRE(
      convertStringToPropertyType(ClassProperty::Type::MAT2) ==
      PropertyType::Mat2);

  REQUIRE(
      convertStringToPropertyType(ClassProperty::Type::MAT3) ==
      PropertyType::Mat3);

  REQUIRE(
      convertStringToPropertyType(ClassProperty::Type::MAT4) ==
      PropertyType::Mat4);

  REQUIRE(
      convertStringToPropertyType(ClassProperty::Type::BOOLEAN) ==
      PropertyType::Boolean);

  REQUIRE(
      convertStringToPropertyType(ClassProperty::Type::STRING) ==
      PropertyType::String);

  REQUIRE(
      convertStringToPropertyType(ClassProperty::Type::ENUM) ==
      PropertyType::Enum);

  REQUIRE(convertStringToPropertyType("invalid") == PropertyType::Invalid);
}

TEST_CASE("Test convertStringToPropertyComponentType") {
  REQUIRE(
      convertStringToPropertyComponentType(
          ClassProperty::ComponentType::UINT8) == PropertyComponentType::Uint8);

  REQUIRE(
      convertStringToPropertyComponentType(
          ClassProperty::ComponentType::INT8) == PropertyComponentType::Int8);

  REQUIRE(
      convertStringToPropertyComponentType(
          ClassProperty::ComponentType::UINT16) ==
      PropertyComponentType::Uint16);
  REQUIRE(
      convertStringToPropertyComponentType(
          ClassProperty::ComponentType::INT16) == PropertyComponentType::Int16);

  REQUIRE(
      convertStringToPropertyComponentType(
          ClassProperty::ComponentType::UINT32) ==
      PropertyComponentType::Uint32);

  REQUIRE(
      convertStringToPropertyComponentType(
          ClassProperty::ComponentType::INT32) == PropertyComponentType::Int32);
  REQUIRE(
      convertStringToPropertyComponentType(
          ClassProperty::ComponentType::UINT64) ==
      PropertyComponentType::Uint64);

  REQUIRE(
      convertStringToPropertyComponentType(
          ClassProperty::ComponentType::INT64) == PropertyComponentType::Int64);

  REQUIRE(
      convertStringToPropertyComponentType(
          ClassProperty::ComponentType::FLOAT32) ==
      PropertyComponentType::Float32);
  REQUIRE(
      convertStringToPropertyComponentType(
          ClassProperty::ComponentType::FLOAT64) ==
      PropertyComponentType::Float64);

  REQUIRE(
      convertStringToPropertyComponentType("invalid") ==
      PropertyComponentType::None);
}

TEST_CASE("Test convertPropertyTypeToString") {
  REQUIRE(
      convertPropertyTypeToString(PropertyType::Scalar) ==
      ClassProperty::Type::SCALAR);

  REQUIRE(
      convertPropertyTypeToString(PropertyType::Vec2) ==
      ClassProperty::Type::VEC2);

  REQUIRE(
      convertPropertyTypeToString(PropertyType::Vec3) ==
      ClassProperty::Type::VEC3);

  REQUIRE(
      convertPropertyTypeToString(PropertyType::Vec4) ==
      ClassProperty::Type::VEC4);

  REQUIRE(
      convertPropertyTypeToString(PropertyType::Mat2) ==
      ClassProperty::Type::MAT2);

  REQUIRE(
      convertPropertyTypeToString(PropertyType::Mat3) ==
      ClassProperty::Type::MAT3);

  REQUIRE(
      convertPropertyTypeToString(PropertyType::Mat4) ==
      ClassProperty::Type::MAT4);

  REQUIRE(
      convertPropertyTypeToString(PropertyType::Boolean) ==
      ClassProperty::Type::BOOLEAN);

  REQUIRE(
      convertPropertyTypeToString(PropertyType::String) ==
      ClassProperty::Type::STRING);

  REQUIRE(
      convertPropertyTypeToString(PropertyType::Enum) ==
      ClassProperty::Type::ENUM);
}

TEST_CASE("Test convertPropertyComponentTypeToString") {
  REQUIRE(
      convertPropertyComponentTypeToString(PropertyComponentType::Uint8) ==
      ClassProperty::ComponentType::UINT8);

  REQUIRE(
      convertPropertyComponentTypeToString(PropertyComponentType::Int8) ==
      ClassProperty::ComponentType::INT8);

  REQUIRE(
      convertPropertyComponentTypeToString(PropertyComponentType::Uint16) ==
      ClassProperty::ComponentType::UINT16);

  REQUIRE(
      convertPropertyComponentTypeToString(PropertyComponentType::Int16) ==
      ClassProperty::ComponentType::INT16);

  REQUIRE(
      convertPropertyComponentTypeToString(PropertyComponentType::Uint32) ==
      ClassProperty::ComponentType::UINT32);

  REQUIRE(
      convertPropertyComponentTypeToString(PropertyComponentType::Int32) ==
      ClassProperty::ComponentType::INT32);

  REQUIRE(
      convertPropertyComponentTypeToString(PropertyComponentType::Uint64) ==
      ClassProperty::ComponentType::UINT64);

  REQUIRE(
      convertPropertyComponentTypeToString(PropertyComponentType::Int64) ==
      ClassProperty::ComponentType::INT64);

  REQUIRE(
      convertPropertyComponentTypeToString(PropertyComponentType::Float32) ==
      ClassProperty::ComponentType::FLOAT32);

  REQUIRE(
      convertPropertyComponentTypeToString(PropertyComponentType::Float64) ==
      ClassProperty::ComponentType::FLOAT64);
}

TEST_CASE("Test convertArrayOffsetTypeStringToPropertyComponentType") {
  REQUIRE(
      convertArrayOffsetTypeStringToPropertyComponentType(
          PropertyTableProperty::ArrayOffsetType::UINT8) ==
      PropertyComponentType::Uint8);

  REQUIRE(
      convertArrayOffsetTypeStringToPropertyComponentType(
          PropertyTableProperty::ArrayOffsetType::UINT16) ==
      PropertyComponentType::Uint16);

  REQUIRE(
      convertArrayOffsetTypeStringToPropertyComponentType(
          PropertyTableProperty::ArrayOffsetType::UINT32) ==
      PropertyComponentType::Uint32);

  REQUIRE(
      convertArrayOffsetTypeStringToPropertyComponentType(
          PropertyTableProperty::ArrayOffsetType::UINT64) ==
      PropertyComponentType::Uint64);

  REQUIRE(
      convertArrayOffsetTypeStringToPropertyComponentType("invalid") ==
      PropertyComponentType::None);
}

TEST_CASE("Test convertStringOffsetTypeStringToPropertyComponentType") {
  REQUIRE(
      convertStringOffsetTypeStringToPropertyComponentType(
          PropertyTableProperty::StringOffsetType::UINT8) ==
      PropertyComponentType::Uint8);

  REQUIRE(
      convertStringOffsetTypeStringToPropertyComponentType(
          PropertyTableProperty::StringOffsetType::UINT16) ==
      PropertyComponentType::Uint16);

  REQUIRE(
      convertStringOffsetTypeStringToPropertyComponentType(
          PropertyTableProperty::StringOffsetType::UINT32) ==
      PropertyComponentType::Uint32);

  REQUIRE(
      convertStringOffsetTypeStringToPropertyComponentType(
          PropertyTableProperty::StringOffsetType::UINT64) ==
      PropertyComponentType::Uint64);

  REQUIRE(
      convertStringOffsetTypeStringToPropertyComponentType("invalid") ==
      PropertyComponentType::None);
}

TEST_CASE("Test convertAccessorComponentTypeToPropertyComponentType") {
  REQUIRE(
      convertAccessorComponentTypeToPropertyComponentType(
          AccessorSpec::ComponentType::BYTE) == PropertyComponentType::Int8);
  REQUIRE(
      convertAccessorComponentTypeToPropertyComponentType(
          AccessorSpec::ComponentType::UNSIGNED_BYTE) ==
      PropertyComponentType::Uint8);
  REQUIRE(
      convertAccessorComponentTypeToPropertyComponentType(
          AccessorSpec::ComponentType::SHORT) == PropertyComponentType::Int16);
  REQUIRE(
      convertAccessorComponentTypeToPropertyComponentType(
          AccessorSpec::ComponentType::UNSIGNED_SHORT) ==
      PropertyComponentType::Uint16);
  REQUIRE(
      convertAccessorComponentTypeToPropertyComponentType(
          AccessorSpec::ComponentType::INT) == PropertyComponentType::Int32);
  REQUIRE(
      convertAccessorComponentTypeToPropertyComponentType(
          AccessorSpec::ComponentType::UNSIGNED_INT) ==
      PropertyComponentType::Uint32);
  REQUIRE(
      convertAccessorComponentTypeToPropertyComponentType(
          AccessorSpec::ComponentType::INT64) == PropertyComponentType::Int64);
  REQUIRE(
      convertAccessorComponentTypeToPropertyComponentType(
          AccessorSpec::ComponentType::UNSIGNED_INT64) ==
      PropertyComponentType::Uint64);
  REQUIRE(
      convertAccessorComponentTypeToPropertyComponentType(
          AccessorSpec::ComponentType::FLOAT) ==
      PropertyComponentType::Float32);
  REQUIRE(
      convertAccessorComponentTypeToPropertyComponentType(
          AccessorSpec::ComponentType::DOUBLE) ==
      PropertyComponentType::Float64);
  REQUIRE(
      convertAccessorComponentTypeToPropertyComponentType(-1) ==
      PropertyComponentType::None);
}

TEST_CASE("Test convertPropertyComponentTypeToAccessorComponentType") {
  REQUIRE(
      convertPropertyComponentTypeToAccessorComponentType(
          PropertyComponentType::Int8) == AccessorSpec::ComponentType::BYTE);
  REQUIRE(
      convertPropertyComponentTypeToAccessorComponentType(
          PropertyComponentType::Uint8) ==
      AccessorSpec::ComponentType::UNSIGNED_BYTE);
  REQUIRE(
      convertPropertyComponentTypeToAccessorComponentType(
          PropertyComponentType::Int16) == AccessorSpec::ComponentType::SHORT);
  REQUIRE(
      convertPropertyComponentTypeToAccessorComponentType(
          PropertyComponentType::Uint16) ==
      AccessorSpec::ComponentType::UNSIGNED_SHORT);
  REQUIRE(
      convertPropertyComponentTypeToAccessorComponentType(
          PropertyComponentType::Int32) == AccessorSpec::ComponentType::INT);
  REQUIRE(
      convertPropertyComponentTypeToAccessorComponentType(
          PropertyComponentType::Uint32) ==
      AccessorSpec::ComponentType::UNSIGNED_INT);
  REQUIRE(
      convertPropertyComponentTypeToAccessorComponentType(
          PropertyComponentType::Int64) == AccessorSpec::ComponentType::INT64);
  REQUIRE(
      convertPropertyComponentTypeToAccessorComponentType(
          PropertyComponentType::Uint64) ==
      AccessorSpec::ComponentType::UNSIGNED_INT64);
  REQUIRE(
      convertPropertyComponentTypeToAccessorComponentType(
          PropertyComponentType::Float32) ==
      AccessorSpec::ComponentType::FLOAT);
  REQUIRE(
      convertPropertyComponentTypeToAccessorComponentType(
          PropertyComponentType::Float64) ==
      AccessorSpec::ComponentType::DOUBLE);
  REQUIRE(
      convertPropertyComponentTypeToAccessorComponentType(
          PropertyComponentType::None) == -1);
}

TEST_CASE("Test isPropertyTypeVecN") {
  REQUIRE(isPropertyTypeVecN(PropertyType::Vec2));
  REQUIRE(isPropertyTypeVecN(PropertyType::Vec3));
  REQUIRE(isPropertyTypeVecN(PropertyType::Vec4));

  REQUIRE(!isPropertyTypeVecN(PropertyType::Scalar));
  REQUIRE(!isPropertyTypeVecN(PropertyType::Mat2));
  REQUIRE(!isPropertyTypeVecN(PropertyType::Mat3));
  REQUIRE(!isPropertyTypeVecN(PropertyType::Mat4));
  REQUIRE(!isPropertyTypeVecN(PropertyType::Boolean));
  REQUIRE(!isPropertyTypeVecN(PropertyType::Enum));
  REQUIRE(!isPropertyTypeVecN(PropertyType::String));
  REQUIRE(!isPropertyTypeVecN(PropertyType::Invalid));
}

TEST_CASE("Test isPropertyTypeMatN") {
  REQUIRE(isPropertyTypeMatN(PropertyType::Mat2));
  REQUIRE(isPropertyTypeMatN(PropertyType::Mat3));
  REQUIRE(isPropertyTypeMatN(PropertyType::Mat4));

  REQUIRE(!isPropertyTypeVecN(PropertyType::Scalar));
  REQUIRE(!isPropertyTypeMatN(PropertyType::Vec2));
  REQUIRE(!isPropertyTypeMatN(PropertyType::Vec3));
  REQUIRE(!isPropertyTypeMatN(PropertyType::Vec4));
  REQUIRE(!isPropertyTypeMatN(PropertyType::Boolean));
  REQUIRE(!isPropertyTypeMatN(PropertyType::Enum));
  REQUIRE(!isPropertyTypeMatN(PropertyType::String));
}

TEST_CASE("Test isPropertyComponentTypeInteger") {
  REQUIRE(isPropertyComponentTypeInteger(PropertyComponentType::Int8));
  REQUIRE(isPropertyComponentTypeInteger(PropertyComponentType::Uint8));
  REQUIRE(isPropertyComponentTypeInteger(PropertyComponentType::Int16));
  REQUIRE(isPropertyComponentTypeInteger(PropertyComponentType::Uint16));
  REQUIRE(isPropertyComponentTypeInteger(PropertyComponentType::Int32));
  REQUIRE(isPropertyComponentTypeInteger(PropertyComponentType::Uint32));
  REQUIRE(isPropertyComponentTypeInteger(PropertyComponentType::Int64));
  REQUIRE(isPropertyComponentTypeInteger(PropertyComponentType::Uint64));

  REQUIRE(!isPropertyComponentTypeInteger(PropertyComponentType::None));
  REQUIRE(!isPropertyComponentTypeInteger(PropertyComponentType::Float32));
  REQUIRE(!isPropertyComponentTypeInteger(PropertyComponentType::Float64));
}

TEST_CASE("Test getDimensionsFromPropertyType") {
  REQUIRE(getDimensionsFromPropertyType(PropertyType::Scalar) == 1);

  REQUIRE(getDimensionsFromPropertyType(PropertyType::Vec2) == 2);
  REQUIRE(getDimensionsFromPropertyType(PropertyType::Vec3) == 3);
  REQUIRE(getDimensionsFromPropertyType(PropertyType::Vec4) == 4);

  REQUIRE(getDimensionsFromPropertyType(PropertyType::Mat2) == 2);
  REQUIRE(getDimensionsFromPropertyType(PropertyType::Mat3) == 3);
  REQUIRE(getDimensionsFromPropertyType(PropertyType::Mat4) == 4);

  REQUIRE(getDimensionsFromPropertyType(PropertyType::Boolean) == 0);
  REQUIRE(getDimensionsFromPropertyType(PropertyType::String) == 0);
  REQUIRE(getDimensionsFromPropertyType(PropertyType::Enum) == 0);
  REQUIRE(getDimensionsFromPropertyType(PropertyType::Invalid) == 0);
}

TEST_CASE("Test getComponentCountFromPropertyType") {
  REQUIRE(getComponentCountFromPropertyType(PropertyType::Scalar) == 1);

  REQUIRE(getComponentCountFromPropertyType(PropertyType::Vec2) == 2);
  REQUIRE(getComponentCountFromPropertyType(PropertyType::Vec3) == 3);
  REQUIRE(getComponentCountFromPropertyType(PropertyType::Vec4) == 4);

  REQUIRE(getComponentCountFromPropertyType(PropertyType::Mat2) == 4);
  REQUIRE(getComponentCountFromPropertyType(PropertyType::Mat3) == 9);
  REQUIRE(getComponentCountFromPropertyType(PropertyType::Mat4) == 16);

  REQUIRE(getComponentCountFromPropertyType(PropertyType::Boolean) == 0);
  REQUIRE(getComponentCountFromPropertyType(PropertyType::String) == 0);
  REQUIRE(getComponentCountFromPropertyType(PropertyType::Enum) == 0);
  REQUIRE(getComponentCountFromPropertyType(PropertyType::Invalid) == 0);
}

TEST_CASE("Test getSizeOfComponentType") {
  REQUIRE(
      getSizeOfComponentType(PropertyComponentType::Int8) == sizeof(int8_t));
  REQUIRE(
      getSizeOfComponentType(PropertyComponentType::Uint8) == sizeof(uint8_t));
  REQUIRE(
      getSizeOfComponentType(PropertyComponentType::Int16) == sizeof(int16_t));
  REQUIRE(
      getSizeOfComponentType(PropertyComponentType::Uint16) ==
      sizeof(uint16_t));
  REQUIRE(
      getSizeOfComponentType(PropertyComponentType::Int32) == sizeof(int32_t));
  REQUIRE(
      getSizeOfComponentType(PropertyComponentType::Uint32) ==
      sizeof(uint32_t));
  REQUIRE(
      getSizeOfComponentType(PropertyComponentType::Int64) == sizeof(int64_t));
  REQUIRE(
      getSizeOfComponentType(PropertyComponentType::Uint64) ==
      sizeof(uint64_t));
  REQUIRE(
      getSizeOfComponentType(PropertyComponentType::Float32) == sizeof(float));
  REQUIRE(
      getSizeOfComponentType(PropertyComponentType::Float64) == sizeof(double));
}
