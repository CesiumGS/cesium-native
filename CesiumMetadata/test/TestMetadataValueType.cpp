#include <Cesium3DTiles/ClassProperty.h>
#include <CesiumGltf/ClassProperty.h>
#include <CesiumMetadata/MetadataValueType.h>

#include <doctest/doctest.h>

#include <cstdint>

using namespace CesiumMetadata;

TEST_CASE("Test constructor") {
  SUBCASE("for empty value") {
    MetadataValueType valueType;
    REQUIRE_EQ(valueType.type, PropertyType::Invalid);
    REQUIRE_EQ(valueType.componentType, PropertyComponentType::None);
    REQUIRE_FALSE(valueType.array);
  }

  SUBCASE("from Cesium3DTiles::ClassProperty") {
    Cesium3DTiles::ClassProperty property;
    property.type = Cesium3DTiles::ClassProperty::Type::STRING;
    property.array = true;

    MetadataValueType valueType(property);
    REQUIRE_EQ(valueType.type, PropertyType::String);
    REQUIRE_EQ(valueType.componentType, PropertyComponentType::None);
    REQUIRE(valueType.array);
  }

  SUBCASE("from CesiumGltf::ClassProperty") {
    CesiumGltf::ClassProperty property;
    property.type = CesiumGltf::ClassProperty::Type::VEC2;
    property.componentType = CesiumGltf ::ClassProperty::ComponentType::FLOAT32;

    MetadataValueType valueType(property);
    REQUIRE_EQ(valueType.type, PropertyType::Vec2);
    REQUIRE_EQ(valueType.componentType, PropertyComponentType::Float32);
    REQUIRE_FALSE(valueType.array);
  }
}
