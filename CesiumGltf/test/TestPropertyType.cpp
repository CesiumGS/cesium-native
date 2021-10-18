#include "CesiumGltf/ClassProperty.h"
#include "CesiumGltf/FeatureTableProperty.h"
#include "CesiumGltf/PropertyType.h"

#include <catch2/catch.hpp>

using namespace CesiumGltf;

TEST_CASE("Test PropertyType utilities function") {
  SECTION("Convert string to PropertyType") {
    REQUIRE(
        convertStringToPropertyType(ClassProperty::Type::UINT8) ==
        PropertyType::Uint8);

    REQUIRE(
        convertStringToPropertyType(ClassProperty::Type::INT8) ==
        PropertyType::Int8);

    REQUIRE(
        convertStringToPropertyType(ClassProperty::Type::UINT16) ==
        PropertyType::Uint16);

    REQUIRE(
        convertStringToPropertyType(ClassProperty::Type::INT16) ==
        PropertyType::Int16);

    REQUIRE(
        convertStringToPropertyType(ClassProperty::Type::UINT32) ==
        PropertyType::Uint32);

    REQUIRE(
        convertStringToPropertyType(ClassProperty::Type::INT32) ==
        PropertyType::Int32);

    REQUIRE(
        convertStringToPropertyType(ClassProperty::Type::UINT64) ==
        PropertyType::Uint64);

    REQUIRE(
        convertStringToPropertyType(ClassProperty::Type::INT64) ==
        PropertyType::Int64);

    REQUIRE(
        convertStringToPropertyType(ClassProperty::Type::FLOAT32) ==
        PropertyType::Float32);

    REQUIRE(
        convertStringToPropertyType(ClassProperty::Type::FLOAT64) ==
        PropertyType::Float64);

    REQUIRE(
        convertStringToPropertyType(ClassProperty::Type::STRING) ==
        PropertyType::String);

    REQUIRE(
        convertStringToPropertyType(ClassProperty::Type::BOOLEAN) ==
        PropertyType::Boolean);

    REQUIRE(
        convertStringToPropertyType(ClassProperty::Type::ARRAY) ==
        PropertyType::Array);

    REQUIRE(convertStringToPropertyType("NONESENSE") == PropertyType::None);
  }

  SECTION("PropertyType to String") {
    REQUIRE(
        convertPropertyTypeToString(PropertyType::Uint8) ==
        ClassProperty::Type::UINT8);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::Int8) ==
        ClassProperty::Type::INT8);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::Uint16) ==
        ClassProperty::Type::UINT16);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::Int16) ==
        ClassProperty::Type::INT16);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::Uint32) ==
        ClassProperty::Type::UINT32);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::Int32) ==
        ClassProperty::Type::INT32);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::Uint64) ==
        ClassProperty::Type::UINT64);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::Int64) ==
        ClassProperty::Type::INT64);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::Float32) ==
        ClassProperty::Type::FLOAT32);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::Float64) ==
        ClassProperty::Type::FLOAT64);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::String) ==
        ClassProperty::Type::STRING);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::Boolean) ==
        ClassProperty::Type::BOOLEAN);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::Array) ==
        ClassProperty::Type::ARRAY);
  }

  SECTION("OffsetString to PropertyType") {
    REQUIRE(
        convertOffsetStringToPropertyType(
            FeatureTableProperty::OffsetType::UINT8) == PropertyType::Uint8);

    REQUIRE(
        convertOffsetStringToPropertyType(
            FeatureTableProperty::OffsetType::UINT16) == PropertyType::Uint16);

    REQUIRE(
        convertOffsetStringToPropertyType(
            FeatureTableProperty::OffsetType::UINT32) == PropertyType::Uint32);

    REQUIRE(
        convertOffsetStringToPropertyType(
            FeatureTableProperty::OffsetType::UINT64) == PropertyType::Uint64);

    REQUIRE(
        convertOffsetStringToPropertyType("NONESENSE") == PropertyType::None);
  }
}
