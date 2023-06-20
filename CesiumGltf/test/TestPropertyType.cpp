#include "CesiumGltf/ClassProperty.h"
#include "CesiumGltf/PropertyTableProperty.h"
#include "CesiumGltf/PropertyType.h"

#include <catch2/catch.hpp>

using namespace CesiumGltf;

TEST_CASE("Test PropertyType utilities function") {
  SECTION("Convert string to PropertyType") {
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

  SECTION("Convert string to PropertyComponentType") {
    REQUIRE(
        convertStringToPropertyComponentType(
            ClassProperty::ComponentType::UINT8) ==
        PropertyComponentType::Uint8);

    REQUIRE(
        convertStringToPropertyComponentType(
            ClassProperty::ComponentType::INT8) == PropertyComponentType::Int8);

    REQUIRE(
        convertStringToPropertyComponentType(
            ClassProperty::ComponentType::UINT16) ==
        PropertyComponentType::Uint16);
    REQUIRE(
        convertStringToPropertyComponentType(
            ClassProperty::ComponentType::INT16) ==
        PropertyComponentType::Int16);

    REQUIRE(
        convertStringToPropertyComponentType(
            ClassProperty::ComponentType::UINT32) ==
        PropertyComponentType::Uint32);

    REQUIRE(
        convertStringToPropertyComponentType(
            ClassProperty::ComponentType::INT32) ==
        PropertyComponentType::Int32);
    REQUIRE(
        convertStringToPropertyComponentType(
            ClassProperty::ComponentType::UINT64) ==
        PropertyComponentType::Uint64);

    REQUIRE(
        convertStringToPropertyComponentType(
            ClassProperty::ComponentType::INT64) ==
        PropertyComponentType::Int64);

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

  SECTION("Convert PropertyType to string") {
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

  SECTION("Convert PropertyComponentType to string") {
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

  SECTION("Convert array offset type string to PropertyComponentType") {
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

  SECTION("Convert string offset type string to PropertyComponentType") {
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
}
