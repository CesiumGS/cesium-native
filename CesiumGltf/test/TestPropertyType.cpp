#include "CesiumGltf/ExtensionExtFeatureMetadataClassProperty.h"
#include "CesiumGltf/ExtensionExtFeatureMetadataFeatureTableProperty.h"
#include "CesiumGltf/PropertyType.h"

#include <catch2/catch.hpp>

using namespace CesiumGltf;

TEST_CASE("Test PropertyType utilities function") {
  SECTION("Convert string to PropertyType") {
    REQUIRE(
        convertStringToPropertyType(
            ExtensionExtFeatureMetadataClassProperty::Type::UINT8) ==
        PropertyType::Uint8);

    REQUIRE(
        convertStringToPropertyType(
            ExtensionExtFeatureMetadataClassProperty::Type::INT8) ==
        PropertyType::Int8);

    REQUIRE(
        convertStringToPropertyType(
            ExtensionExtFeatureMetadataClassProperty::Type::UINT16) ==
        PropertyType::Uint16);

    REQUIRE(
        convertStringToPropertyType(
            ExtensionExtFeatureMetadataClassProperty::Type::INT16) ==
        PropertyType::Int16);

    REQUIRE(
        convertStringToPropertyType(
            ExtensionExtFeatureMetadataClassProperty::Type::UINT32) ==
        PropertyType::Uint32);

    REQUIRE(
        convertStringToPropertyType(
            ExtensionExtFeatureMetadataClassProperty::Type::INT32) ==
        PropertyType::Int32);

    REQUIRE(
        convertStringToPropertyType(
            ExtensionExtFeatureMetadataClassProperty::Type::UINT64) ==
        PropertyType::Uint64);

    REQUIRE(
        convertStringToPropertyType(
            ExtensionExtFeatureMetadataClassProperty::Type::INT64) ==
        PropertyType::Int64);

    REQUIRE(
        convertStringToPropertyType(
            ExtensionExtFeatureMetadataClassProperty::Type::FLOAT32) ==
        PropertyType::Float32);

    REQUIRE(
        convertStringToPropertyType(
            ExtensionExtFeatureMetadataClassProperty::Type::FLOAT64) ==
        PropertyType::Float64);

    REQUIRE(
        convertStringToPropertyType(
            ExtensionExtFeatureMetadataClassProperty::Type::STRING) ==
        PropertyType::String);

    REQUIRE(
        convertStringToPropertyType(
            ExtensionExtFeatureMetadataClassProperty::Type::BOOLEAN) ==
        PropertyType::Boolean);

    REQUIRE(
        convertStringToPropertyType(
            ExtensionExtFeatureMetadataClassProperty::Type::ARRAY) ==
        PropertyType::Array);

    REQUIRE(convertStringToPropertyType("NONESENSE") == PropertyType::None);
  }

  SECTION("PropertyType to String") {
    REQUIRE(
        convertPropertyTypeToString(PropertyType::Uint8) ==
        ExtensionExtFeatureMetadataClassProperty::Type::UINT8);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::Int8) ==
        ExtensionExtFeatureMetadataClassProperty::Type::INT8);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::Uint16) ==
        ExtensionExtFeatureMetadataClassProperty::Type::UINT16);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::Int16) ==
        ExtensionExtFeatureMetadataClassProperty::Type::INT16);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::Uint32) ==
        ExtensionExtFeatureMetadataClassProperty::Type::UINT32);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::Int32) ==
        ExtensionExtFeatureMetadataClassProperty::Type::INT32);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::Uint64) ==
        ExtensionExtFeatureMetadataClassProperty::Type::UINT64);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::Int64) ==
        ExtensionExtFeatureMetadataClassProperty::Type::INT64);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::Float32) ==
        ExtensionExtFeatureMetadataClassProperty::Type::FLOAT32);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::Float64) ==
        ExtensionExtFeatureMetadataClassProperty::Type::FLOAT64);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::String) ==
        ExtensionExtFeatureMetadataClassProperty::Type::STRING);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::Boolean) ==
        ExtensionExtFeatureMetadataClassProperty::Type::BOOLEAN);

    REQUIRE(
        convertPropertyTypeToString(PropertyType::Array) ==
        ExtensionExtFeatureMetadataClassProperty::Type::ARRAY);
  }

  SECTION("OffsetString to PropertyType") {
    REQUIRE(
        convertOffsetStringToPropertyType(
            ExtensionExtFeatureMetadataFeatureTableProperty::OffsetType::
                UINT8) == PropertyType::Uint8);

    REQUIRE(
        convertOffsetStringToPropertyType(
            ExtensionExtFeatureMetadataFeatureTableProperty::OffsetType::
                UINT16) == PropertyType::Uint16);

    REQUIRE(
        convertOffsetStringToPropertyType(
            ExtensionExtFeatureMetadataFeatureTableProperty::OffsetType::
                UINT32) == PropertyType::Uint32);

    REQUIRE(
        convertOffsetStringToPropertyType(
            ExtensionExtFeatureMetadataFeatureTableProperty::OffsetType::
                UINT64) == PropertyType::Uint64);

    REQUIRE(
        convertOffsetStringToPropertyType("NONESENSE") == PropertyType::None);
  }
}
