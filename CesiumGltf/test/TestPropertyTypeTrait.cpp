#include "CesiumGltf/PropertyTypeTraits.h"

#include <catch2/catch.hpp>

TEST_CASE("Test PropertyTypeTrait") {
  SECTION("IsNumeric") {
    REQUIRE(CesiumGltf::IsMetadataNumeric<uint8_t>::value);
    REQUIRE(CesiumGltf::IsMetadataNumeric<int8_t>::value);

    REQUIRE(CesiumGltf::IsMetadataNumeric<uint16_t>::value);
    REQUIRE(CesiumGltf::IsMetadataNumeric<int16_t>::value);

    REQUIRE(CesiumGltf::IsMetadataNumeric<uint32_t>::value);
    REQUIRE(CesiumGltf::IsMetadataNumeric<int32_t>::value);

    REQUIRE(CesiumGltf::IsMetadataNumeric<uint64_t>::value);
    REQUIRE(CesiumGltf::IsMetadataNumeric<int64_t>::value);

    REQUIRE(CesiumGltf::IsMetadataNumeric<float>::value);
    REQUIRE(CesiumGltf::IsMetadataNumeric<double>::value);

    REQUIRE(CesiumGltf::IsMetadataNumeric<bool>::value == false);
    REQUIRE(CesiumGltf::IsMetadataNumeric<std::string_view>::value == false);
  }

  SECTION("IsBoolean") {
    REQUIRE(CesiumGltf::IsMetadataBoolean<bool>::value);
    REQUIRE(CesiumGltf::IsMetadataBoolean<std::string_view>::value == false);
  }

  SECTION("IsString") {
    REQUIRE(CesiumGltf::IsMetadataString<std::string_view>::value);
    REQUIRE(CesiumGltf::IsMetadataString<std::string>::value == false);
  }

  SECTION("IsNumericArray") {
    REQUIRE(CesiumGltf::IsMetadataNumericArray<
            CesiumGltf::MetadataArrayView<uint32_t>>::value);

    REQUIRE(
        CesiumGltf::IsMetadataNumericArray<
            CesiumGltf::MetadataArrayView<bool>>::value == false);
  }

  SECTION("IsBooleanArray") {
    REQUIRE(CesiumGltf::IsMetadataBooleanArray<
            CesiumGltf::MetadataArrayView<bool>>::value);

    REQUIRE(
        CesiumGltf::IsMetadataBooleanArray<
            CesiumGltf::MetadataArrayView<uint32_t>>::value == false);
  }

  SECTION("IsStringArray") {
    REQUIRE(CesiumGltf::IsMetadataStringArray<
            CesiumGltf::MetadataArrayView<std::string_view>>::value);

    REQUIRE(
        CesiumGltf::IsMetadataStringArray<
            CesiumGltf::MetadataArrayView<uint32_t>>::value == false);
  }

  SECTION("ArrayType") {
    using type = CesiumGltf::MetadataArrayType<
        CesiumGltf::MetadataArrayView<uint32_t>>::type;
    REQUIRE(std::is_same_v<type, uint32_t>);
  }

  SECTION("TypeToPropertyType") {
    CesiumGltf::PropertyType value =
        CesiumGltf::TypeToPropertyType<uint32_t>::value;
    REQUIRE(value == CesiumGltf::PropertyType::Uint32);

    auto component = CesiumGltf::TypeToPropertyType<
        CesiumGltf::MetadataArrayView<uint32_t>>::component;
    value = CesiumGltf::TypeToPropertyType<
        CesiumGltf::MetadataArrayView<uint32_t>>::value;
    CesiumGltf::PropertyType expected = CesiumGltf::PropertyType::Array;
    REQUIRE(value == expected);
    REQUIRE(component == CesiumGltf::PropertyType::Uint32);
  }
}
