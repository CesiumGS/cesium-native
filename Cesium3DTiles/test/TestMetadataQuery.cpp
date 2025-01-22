#include <Cesium3DTiles/Class.h>
#include <Cesium3DTiles/ClassProperty.h>
#include <Cesium3DTiles/MetadataEntity.h>
#include <Cesium3DTiles/MetadataQuery.h>
#include <Cesium3DTiles/Schema.h>
#include <CesiumUtility/JsonValue.h>

#include <doctest/doctest.h>

#include <optional>

using namespace Cesium3DTiles;
using namespace CesiumUtility;

TEST_CASE("MetadataQuery") {
  SUBCASE("findFirstPropertyWithSemantic") {
    Schema schema{};
    Class& classDefinition =
        schema.classes.emplace("someClass", Class()).first->second;

    ClassProperty& classProperty1 =
        classDefinition.properties.emplace("someProperty", ClassProperty())
            .first->second;
    classProperty1.type = ClassProperty::Type::SCALAR;
    classProperty1.componentType = ClassProperty::ComponentType::FLOAT64;

    ClassProperty& classProperty2 =
        classDefinition.properties
            .emplace("somePropertyWithSemantic", ClassProperty())
            .first->second;
    classProperty2.type = ClassProperty::Type::STRING;
    classProperty2.semantic = "SOME_SEMANTIC";

    MetadataEntity withoutSemantic;
    withoutSemantic.classProperty = "someClass";
    withoutSemantic.properties.emplace("someProperty", JsonValue(3.0));

    MetadataEntity withSemantic = withoutSemantic;
    withSemantic.properties.emplace(
        "somePropertyWithSemantic",
        JsonValue("the value"));

    std::optional<FoundMetadataProperty> foundProperty1 =
        MetadataQuery::findFirstPropertyWithSemantic(
            schema,
            withoutSemantic,
            "SOME_SEMANTIC");
    CHECK(!foundProperty1);

    std::optional<FoundMetadataProperty> foundProperty2 =
        MetadataQuery::findFirstPropertyWithSemantic(
            schema,
            withSemantic,
            "SOME_SEMANTIC");
    REQUIRE(foundProperty2);
    CHECK(foundProperty2->classIdentifier == "someClass");
    CHECK(&foundProperty2->classDefinition == &classDefinition);
    CHECK(foundProperty2->propertyIdentifier == "somePropertyWithSemantic");
    CHECK(&foundProperty2->propertyDefinition == &classProperty2);
    CHECK(foundProperty2->propertyValue.getStringOrDefault("") == "the value");
  }
}
