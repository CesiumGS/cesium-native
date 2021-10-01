#include "CesiumGltf/PropertyType.h"

#include "catch2/catch.hpp"

TEST_CASE("Test PropertyType utilities function") {
  SECTION("Convert string to PropertyType") {
    REQUIRE(
        CesiumGltf::convertStringToPropertyType("UINT8") ==
        CesiumGltf::PropertyType::Uint8);

    REQUIRE(
        CesiumGltf::convertStringToPropertyType("INT8") ==
        CesiumGltf::PropertyType::Int8);

    REQUIRE(
        CesiumGltf::convertStringToPropertyType("UINT16") ==
        CesiumGltf::PropertyType::Uint16);

    REQUIRE(
        CesiumGltf::convertStringToPropertyType("INT16") ==
        CesiumGltf::PropertyType::Int16);

    REQUIRE(
        CesiumGltf::convertStringToPropertyType("UINT32") ==
        CesiumGltf::PropertyType::Uint32);

    REQUIRE(
        CesiumGltf::convertStringToPropertyType("INT32") ==
        CesiumGltf::PropertyType::Int32);

    REQUIRE(
        CesiumGltf::convertStringToPropertyType("UINT64") ==
        CesiumGltf::PropertyType::Uint64);

    REQUIRE(
        CesiumGltf::convertStringToPropertyType("INT64") ==
        CesiumGltf::PropertyType::Int64);

    REQUIRE(
        CesiumGltf::convertStringToPropertyType("FLOAT32") ==
        CesiumGltf::PropertyType::Float32);

    REQUIRE(
        CesiumGltf::convertStringToPropertyType("FLOAT64") ==
        CesiumGltf::PropertyType::Float64);

    REQUIRE(
        CesiumGltf::convertStringToPropertyType("STRING") ==
        CesiumGltf::PropertyType::String);

    REQUIRE(
        CesiumGltf::convertStringToPropertyType("BOOLEAN") ==
        CesiumGltf::PropertyType::Boolean);

    REQUIRE(
        CesiumGltf::convertStringToPropertyType("ARRAY") ==
        CesiumGltf::PropertyType::Array);

    REQUIRE(
        CesiumGltf::convertStringToPropertyType("NONESENSE") ==
        CesiumGltf::PropertyType::None);
  }

  SECTION("PropertyType to String") {
    REQUIRE(
        CesiumGltf::convertPropertyTypeToString(
            CesiumGltf::PropertyType::Uint8) == "UINT8");

    REQUIRE(
        CesiumGltf::convertPropertyTypeToString(
            CesiumGltf::PropertyType::Int8) == "INT8");

    REQUIRE(
        CesiumGltf::convertPropertyTypeToString(
            CesiumGltf::PropertyType::Uint16) == "UINT16");

    REQUIRE(
        CesiumGltf::convertPropertyTypeToString(
            CesiumGltf::PropertyType::Int16) == "INT16");

    REQUIRE(
        CesiumGltf::convertPropertyTypeToString(
            CesiumGltf::PropertyType::Uint32) == "UINT32");

    REQUIRE(
        CesiumGltf::convertPropertyTypeToString(
            CesiumGltf::PropertyType::Int32) == "INT32");

    REQUIRE(
        CesiumGltf::convertPropertyTypeToString(
            CesiumGltf::PropertyType::Uint64) == "UINT64");

    REQUIRE(
        CesiumGltf::convertPropertyTypeToString(
            CesiumGltf::PropertyType::Int64) == "INT64");

    REQUIRE(
        CesiumGltf::convertPropertyTypeToString(
            CesiumGltf::PropertyType::Float32) == "FLOAT32");

    REQUIRE(
        CesiumGltf::convertPropertyTypeToString(
            CesiumGltf::PropertyType::Float64) == "FLOAT64");

    REQUIRE(
        CesiumGltf::convertPropertyTypeToString(
            CesiumGltf::PropertyType::String) == "STRING");

    REQUIRE(
        CesiumGltf::convertPropertyTypeToString(
            CesiumGltf::PropertyType::Boolean) == "BOOLEAN");

    REQUIRE(
        CesiumGltf::convertPropertyTypeToString(
            CesiumGltf::PropertyType::Array) == "ARRAY");
  }

  SECTION("OffsetString to PropertyType") {
    REQUIRE(
        CesiumGltf::convertOffsetStringToPropertyType("UINT8") ==
        CesiumGltf::PropertyType::Uint8);

    REQUIRE(
        CesiumGltf::convertOffsetStringToPropertyType("UINT16") ==
        CesiumGltf::PropertyType::Uint16);

    REQUIRE(
        CesiumGltf::convertOffsetStringToPropertyType("UINT32") ==
        CesiumGltf::PropertyType::Uint32);

    REQUIRE(
        CesiumGltf::convertOffsetStringToPropertyType("UINT64") ==
        CesiumGltf::PropertyType::Uint64);

    REQUIRE(
        CesiumGltf::convertOffsetStringToPropertyType("BOOLEAN") ==
        CesiumGltf::PropertyType::None);

    REQUIRE(
        CesiumGltf::convertOffsetStringToPropertyType("ARRAY") ==
        CesiumGltf::PropertyType::None);

    REQUIRE(
        CesiumGltf::convertOffsetStringToPropertyType("NONESENSE") ==
        CesiumGltf::PropertyType::None);
  }
}
