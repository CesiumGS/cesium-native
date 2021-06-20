#include "CesiumGltf/PropertyTypeTraits.h"
#include "catch2/catch.hpp"

TEST_CASE("Test PropertyTypeTrait") {
  SECTION("IsNumeric") {
    REQUIRE(CesiumGltf::IsNumeric<uint8_t>::value);
    REQUIRE(CesiumGltf::IsNumeric<int8_t>::value);

    REQUIRE(CesiumGltf::IsNumeric<uint16_t>::value);
    REQUIRE(CesiumGltf::IsNumeric<int16_t>::value);

    REQUIRE(CesiumGltf::IsNumeric<uint32_t>::value);
    REQUIRE(CesiumGltf::IsNumeric<int32_t>::value);

    REQUIRE(CesiumGltf::IsNumeric<uint64_t>::value);
    REQUIRE(CesiumGltf::IsNumeric<int64_t>::value);

    REQUIRE(CesiumGltf::IsNumeric<float>::value);
    REQUIRE(CesiumGltf::IsNumeric<double>::value);

    REQUIRE(CesiumGltf::IsNumeric<bool>::value == false);
    REQUIRE(CesiumGltf::IsNumeric<std::string_view>::value == false);
  }

  SECTION("IsBoolean") {
    REQUIRE(CesiumGltf::IsBoolean<bool>::value);
    REQUIRE(CesiumGltf::IsBoolean<std::string_view>::value == false);
  }

  SECTION("IsString") {
    REQUIRE(CesiumGltf::IsString<std::string_view>::value);
    REQUIRE(CesiumGltf::IsString<std::string>::value == false);
  }

  SECTION("IsNumericArray") {
    REQUIRE(CesiumGltf::IsNumericArray<
            CesiumGltf::MetadataArrayView<uint32_t>>::value);

    REQUIRE(
        CesiumGltf::IsNumericArray<
            CesiumGltf::MetadataArrayView<bool>>::value == false);
  }

  SECTION("IsBooleanArray") {
    REQUIRE(
        CesiumGltf::IsBooleanArray<CesiumGltf::MetadataArrayView<bool>>::value);

    REQUIRE(
        CesiumGltf::IsBooleanArray<
            CesiumGltf::MetadataArrayView<uint32_t>>::value == false);
  }

  SECTION("IsStringArray") {
    REQUIRE(CesiumGltf::IsStringArray<
            CesiumGltf::MetadataArrayView<std::string_view>>::value);

    REQUIRE(
        CesiumGltf::IsStringArray<
            CesiumGltf::MetadataArrayView<uint32_t>>::value == false);
  }

  SECTION("ArrayType") {
    using type =
        CesiumGltf::ArrayType<CesiumGltf::MetadataArrayView<uint32_t>>::type;
    REQUIRE(std::is_same_v<type, uint32_t>);
  }

  SECTION("TypeToPropertyType") {
    uint32_t value = CesiumGltf::TypeToPropertyType<uint32_t>::value;
    REQUIRE(value == static_cast<uint32_t>(CesiumGltf::PropertyType::Uint32));

    value = CesiumGltf::TypeToPropertyType<
        CesiumGltf::MetadataArrayView<uint32_t>>::value;
    uint32_t expected =
        static_cast<uint32_t>(CesiumGltf::PropertyType::Uint32) |
        static_cast<uint32_t>(CesiumGltf::PropertyType::Array);
    REQUIRE(value == expected);
  }
}
