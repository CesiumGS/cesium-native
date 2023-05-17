#include "CesiumGltf/ExtensionExtStructuralMetadataClassProperty.h"
#include "CesiumGltf/ExtensionExtStructuralMetadataPropertyTableProperty.h"
#include "CesiumGltf/StructuralMetadataPropertyType.h"

#include <catch2/catch.hpp>

using namespace CesiumGltf;
using namespace StructuralMetadata;

TEST_CASE("Test StructuralMetadata PropertyType utilities function") {
  SECTION("Convert string to PropertyType") {
    REQUIRE(
        convertStringToPropertyType(
            ExtensionExtStructuralMetadataClassProperty::Type::SCALAR) ==
        PropertyType::Scalar);

    REQUIRE(
        convertStringToPropertyType(
            ExtensionExtStructuralMetadataClassProperty::Type::VEC2) ==
        PropertyType::Vec2);

    REQUIRE(
        convertStringToPropertyType(
            ExtensionExtStructuralMetadataClassProperty::Type::VEC3) ==
        PropertyType::Vec3);

    REQUIRE(
        convertStringToPropertyType(
            ExtensionExtStructuralMetadataClassProperty::Type::VEC4) ==
        PropertyType::Vec4);

    REQUIRE(
        convertStringToPropertyType(
            ExtensionExtStructuralMetadataClassProperty::Type::MAT2) ==
        PropertyType::Mat2);

    REQUIRE(
        convertStringToPropertyType(
            ExtensionExtStructuralMetadataClassProperty::Type::MAT3) ==
        PropertyType::Mat3);

    REQUIRE(
        convertStringToPropertyType(
            ExtensionExtStructuralMetadataClassProperty::Type::MAT4) ==
        PropertyType::Mat4);

    REQUIRE(
        convertStringToPropertyType(
            ExtensionExtStructuralMetadataClassProperty::Type::BOOLEAN) ==
        PropertyType::Boolean);

    REQUIRE(
        convertStringToPropertyType(
            ExtensionExtStructuralMetadataClassProperty::Type::STRING) ==
        PropertyType::String);

    REQUIRE(
        convertStringToPropertyType(
            ExtensionExtStructuralMetadataClassProperty::Type::ENUM) ==
        PropertyType::Enum);

    REQUIRE(convertStringToPropertyType("invalid") == PropertyType::Invalid);
  }

  SECTION("Convert string to PropertyComponentType") {
    REQUIRE(
        convertStringToPropertyComponentType(
            ExtensionExtStructuralMetadataClassProperty::ComponentType::
                UINT8) == PropertyComponentType::Uint8);

    REQUIRE(
        convertStringToPropertyComponentType(
            ExtensionExtStructuralMetadataClassProperty::ComponentType::INT8) ==
        PropertyComponentType::Int8);

    REQUIRE(
        convertStringToPropertyComponentType(
            ExtensionExtStructuralMetadataClassProperty::ComponentType::
                UINT16) == PropertyComponentType::Uint16);
    REQUIRE(
        convertStringToPropertyComponentType(
            ExtensionExtStructuralMetadataClassProperty::ComponentType::
                INT16) == PropertyComponentType::Int16);

    REQUIRE(
        convertStringToPropertyComponentType(
            ExtensionExtStructuralMetadataClassProperty::ComponentType::
                UINT32) == PropertyComponentType::Uint32);

    REQUIRE(
        convertStringToPropertyComponentType(
            ExtensionExtStructuralMetadataClassProperty::ComponentType::
                INT32) == PropertyComponentType::Int32);
    REQUIRE(
        convertStringToPropertyComponentType(
            ExtensionExtStructuralMetadataClassProperty::ComponentType::
                UINT64) == PropertyComponentType::Uint64);

    REQUIRE(
        convertStringToPropertyComponentType(
            ExtensionExtStructuralMetadataClassProperty::ComponentType::
                INT64) == PropertyComponentType::Int64);

    REQUIRE(
        convertStringToPropertyComponentType(
            ExtensionExtStructuralMetadataClassProperty::ComponentType::
                FLOAT32) == PropertyComponentType::Float32);
    REQUIRE(
        convertStringToPropertyComponentType(
            ExtensionExtStructuralMetadataClassProperty::ComponentType::
                FLOAT64) == PropertyComponentType::Float64);

    REQUIRE(
        convertStringToPropertyComponentType("invalid") ==
        PropertyComponentType::None);
  }

  SECTION("Convert PropertyType to string") {
    REQUIRE(
        convertPropertyTypeToString(PropertyType::Scalar) ==
        ExtensionExtStructuralMetadataClassProperty::Type::SCALAR);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::Vec2) ==
        ExtensionExtStructuralMetadataClassProperty::Type::VEC2);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::Vec3) ==
        ExtensionExtStructuralMetadataClassProperty::Type::VEC3);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::Vec4) ==
        ExtensionExtStructuralMetadataClassProperty::Type::VEC4);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::Mat2) ==
        ExtensionExtStructuralMetadataClassProperty::Type::MAT2);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::Mat3) ==
        ExtensionExtStructuralMetadataClassProperty::Type::MAT3);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::Mat4) ==
        ExtensionExtStructuralMetadataClassProperty::Type::MAT4);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::Boolean) ==
        ExtensionExtStructuralMetadataClassProperty::Type::BOOLEAN);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::String) ==
        ExtensionExtStructuralMetadataClassProperty::Type::STRING);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::Enum) ==
        ExtensionExtStructuralMetadataClassProperty::Type::ENUM);
  }

  SECTION("Convert PropertyComponentType to string") {
    REQUIRE(
        convertPropertyComponentTypeToString(PropertyComponentType::Uint8) ==
        ExtensionExtStructuralMetadataClassProperty::ComponentType::UINT8);

    REQUIRE(
        convertPropertyComponentTypeToString(PropertyComponentType::Int8) ==
        ExtensionExtStructuralMetadataClassProperty::ComponentType::INT8);

    REQUIRE(
        convertPropertyComponentTypeToString(PropertyComponentType::Uint16) ==
        ExtensionExtStructuralMetadataClassProperty::ComponentType::UINT16);

    REQUIRE(
        convertPropertyComponentTypeToString(PropertyComponentType::Int16) ==
        ExtensionExtStructuralMetadataClassProperty::ComponentType::INT16);

    REQUIRE(
        convertPropertyComponentTypeToString(PropertyComponentType::Uint32) ==
        ExtensionExtStructuralMetadataClassProperty::ComponentType::UINT32);

    REQUIRE(
        convertPropertyComponentTypeToString(PropertyComponentType::Int32) ==
        ExtensionExtStructuralMetadataClassProperty::ComponentType::INT32);

    REQUIRE(
        convertPropertyComponentTypeToString(PropertyComponentType::Uint64) ==
        ExtensionExtStructuralMetadataClassProperty::ComponentType::UINT64);

    REQUIRE(
        convertPropertyComponentTypeToString(PropertyComponentType::Int64) ==
        ExtensionExtStructuralMetadataClassProperty::ComponentType::INT64);

    REQUIRE(
        convertPropertyComponentTypeToString(PropertyComponentType::Float32) ==
        ExtensionExtStructuralMetadataClassProperty::ComponentType::FLOAT32);

    REQUIRE(
        convertPropertyComponentTypeToString(PropertyComponentType::Float64) ==
        ExtensionExtStructuralMetadataClassProperty::ComponentType::FLOAT64);
  }

  SECTION("Convert array offset type string to PropertyComponentType") {
    REQUIRE(
        convertArrayOffsetTypeStringToPropertyComponentType(
            ExtensionExtStructuralMetadataPropertyTableProperty::
                ArrayOffsetType::UINT8) == PropertyComponentType::Uint8);

    REQUIRE(
        convertArrayOffsetTypeStringToPropertyComponentType(
            ExtensionExtStructuralMetadataPropertyTableProperty::
                ArrayOffsetType::UINT16) == PropertyComponentType::Uint16);

    REQUIRE(
        convertArrayOffsetTypeStringToPropertyComponentType(
            ExtensionExtStructuralMetadataPropertyTableProperty::
                ArrayOffsetType::UINT32) == PropertyComponentType::Uint32);

    REQUIRE(
        convertArrayOffsetTypeStringToPropertyComponentType(
            ExtensionExtStructuralMetadataPropertyTableProperty::
                ArrayOffsetType::UINT64) == PropertyComponentType::Uint64);

    REQUIRE(
        convertArrayOffsetTypeStringToPropertyComponentType("invalid") ==
        PropertyComponentType::None);
  }

  SECTION("Convert string offset type string to PropertyComponentType") {
    REQUIRE(
        convertStringOffsetTypeStringToPropertyComponentType(
            ExtensionExtStructuralMetadataPropertyTableProperty::
                StringOffsetType::UINT8) == PropertyComponentType::Uint8);

    REQUIRE(
        convertStringOffsetTypeStringToPropertyComponentType(
            ExtensionExtStructuralMetadataPropertyTableProperty::
                StringOffsetType::UINT16) == PropertyComponentType::Uint16);

    REQUIRE(
        convertStringOffsetTypeStringToPropertyComponentType(
            ExtensionExtStructuralMetadataPropertyTableProperty::
                StringOffsetType::UINT32) == PropertyComponentType::Uint32);

    REQUIRE(
        convertStringOffsetTypeStringToPropertyComponentType(
            ExtensionExtStructuralMetadataPropertyTableProperty::
                StringOffsetType::UINT64) == PropertyComponentType::Uint64);

    REQUIRE(
        convertStringOffsetTypeStringToPropertyComponentType("invalid") ==
        PropertyComponentType::None);
  }
}
