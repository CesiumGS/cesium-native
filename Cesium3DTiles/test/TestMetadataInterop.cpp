#include <Cesium3DTiles/MetadataInterop.h>
#include <CesiumGltf/Class.h>
#include <CesiumGltf/ClassProperty.h>
#include <CesiumGltf/ClassStatistics.h>
#include <CesiumGltf/Enum.h>
#include <CesiumGltf/EnumValue.h>
#include <CesiumGltf/PropertyStatistics.h>
#include <CesiumGltf/Schema.h>
#include <CesiumGltf/Statistics.h>
#include <CesiumTilesetJson/Class.h>
#include <CesiumTilesetJson/ClassProperty.h>
#include <CesiumTilesetJson/ClassStatistics.h>
#include <CesiumTilesetJson/Enum.h>
#include <CesiumTilesetJson/EnumValue.h>
#include <CesiumTilesetJson/PropertyStatistics.h>
#include <CesiumTilesetJson/Schema.h>
#include <CesiumTilesetJson/Statistics.h>
#include <CesiumUtility/JsonValue.h>

#include <doctest/doctest.h>

#include <cstdint>
#include <string>
#include <utility>

using namespace Cesium3DTiles;
using namespace CesiumUtility;

TEST_CASE("Test MetadataInterop") {
  SUBCASE("convertClassProperty") {
    SUBCASE("from 3D Tiles 1.1 to glTF") {
      CesiumTilesetJson::ClassProperty property;
      property.name = "name";
      property.description = "description";
      property.type = CesiumTilesetJson::ClassProperty::Type::VEC2;
      property.componentType =
          CesiumTilesetJson::ClassProperty::ComponentType::FLOAT32;
      property.array = true;
      property.count = 3;
      property.normalized = true;
      property.offset = JsonValue(1.0);
      property.scale = JsonValue(2.0);
      property.min = JsonValue(0.0);
      property.max = JsonValue(10.0);
      property.required = true;
      property.noData = JsonValue(-1.0);
      property.defaultProperty = JsonValue(5.0);
      property.semantic = "SEMANTIC";
      property.extras["key"] = "value";

      CesiumGltf::ClassProperty result =
          MetadataInterop::convertClassProperty(std::move(property));

      CHECK_EQ(result.name, "name");
      CHECK_EQ(result.description, "description");
      CHECK_EQ(result.type, CesiumGltf::ClassProperty::Type::VEC2);
      REQUIRE_UNARY(result.componentType);
      CHECK_EQ(
          *result.componentType,
          CesiumGltf::ClassProperty::ComponentType::FLOAT32);
      CHECK_EQ(result.array, true);
      REQUIRE_UNARY(result.count);
      CHECK_EQ(*result.count, 3);
      CHECK_EQ(result.normalized, true);
      REQUIRE_UNARY(result.offset);
      CHECK_EQ(result.offset->getSafeNumber<double>(), 1.0);
      REQUIRE_UNARY(result.scale);
      CHECK_EQ(result.scale->getSafeNumber<double>(), 2.0);
      REQUIRE_UNARY(result.min);
      CHECK_EQ(result.min->getSafeNumber<double>(), 0.0);
      REQUIRE_UNARY(result.max);
      CHECK_EQ(result.max->getSafeNumber<double>(), 10.0);
      CHECK_EQ(result.required, true);
      REQUIRE_UNARY(result.noData);
      CHECK_EQ(result.noData->getSafeNumber<double>(), -1.0);
      REQUIRE_UNARY(result.defaultProperty);
      CHECK_EQ(result.defaultProperty->getSafeNumber<double>(), 5.0);
      CHECK_EQ(result.semantic, "SEMANTIC");
      REQUIRE_EQ(result.extras.size(), 1);
      CHECK_EQ(result.extras.at("key").getString(), "value");
    }

    SUBCASE("from glTF to 3D Tiles 1.1") {
      CesiumGltf::ClassProperty property;
      property.name = "name";
      property.description = "description";
      property.type = CesiumGltf::ClassProperty::Type::ENUM;
      property.enumType = "myEnum";
      property.array = true;
      property.count = 2;
      property.required = true;
      property.noData = JsonValue(-1.0);
      property.defaultProperty = JsonValue(5.0);
      property.semantic = "SEMANTIC";
      property.extras["key"] = "value";

      CesiumTilesetJson::ClassProperty result =
          MetadataInterop::convertClassProperty(std::move(property));

      CHECK_EQ(result.name, "name");
      CHECK_EQ(result.description, "description");
      CHECK_EQ(result.type, CesiumTilesetJson::ClassProperty::Type::ENUM);
      CHECK_EQ(result.enumType, "myEnum");
      CHECK_EQ(result.array, true);
      REQUIRE_UNARY(result.count);
      CHECK_EQ(*result.count, 2);
      CHECK_EQ(result.required, true);
      REQUIRE_UNARY(result.noData);
      CHECK_EQ(result.noData->getSafeNumber<double>(), -1.0);
      REQUIRE_UNARY(result.defaultProperty);
      CHECK_EQ(result.defaultProperty->getSafeNumber<double>(), 5.0);
      CHECK_EQ(result.semantic, "SEMANTIC");
      REQUIRE_EQ(result.extras.size(), 1);
      CHECK_EQ(result.extras.at("key").getString(), "value");
    }
  }

  SUBCASE("convertClass") {
    SUBCASE("from 3D Tiles 1.1 to glTF") {
      CesiumTilesetJson::Class class_;
      class_.name = "className";
      class_.description = "classDescription";
      class_.parent = "parentClass";
      class_.properties["a"].type =
          CesiumTilesetJson::ClassProperty::Type::STRING;
      class_.properties["b"].type =
          CesiumTilesetJson::ClassProperty::Type::BOOLEAN;

      CesiumGltf::Class result = MetadataInterop::convertClass(std::move(class_));

      CHECK_EQ(result.name, "className");
      CHECK_EQ(result.description, "classDescription");
      CHECK_EQ(result.parent, "parentClass");
      REQUIRE_EQ(result.properties.size(), 2);
      CHECK_EQ(
          result.properties.at("a").type,
          CesiumGltf::ClassProperty::Type::STRING);
      CHECK_EQ(
          result.properties.at("b").type,
          CesiumGltf::ClassProperty::Type::BOOLEAN);
    }

    SUBCASE("from glTF to 3D Tiles 1.1") {
      CesiumGltf::Class class_;
      class_.name = "className";
      class_.description = "classDescription";
      class_.parent = "parentClass";
      class_.properties["a"].type = CesiumGltf::ClassProperty::Type::STRING;
      class_.properties["b"].type = CesiumGltf::ClassProperty::Type::BOOLEAN;

      CesiumTilesetJson::Class result =
          MetadataInterop::convertClass(std::move(class_));

      CHECK_EQ(result.name, "className");
      CHECK_EQ(result.description, "classDescription");
      CHECK_EQ(result.parent, "parentClass");
      REQUIRE_EQ(result.properties.size(), 2);
      CHECK_EQ(
          result.properties.at("a").type,
          CesiumTilesetJson::ClassProperty::Type::STRING);
      CHECK_EQ(
          result.properties.at("b").type,
          CesiumTilesetJson::ClassProperty::Type::BOOLEAN);
    }
  }

  SUBCASE("convertEnumValue") {
    SUBCASE("from 3D Tiles 1.1 to glTF") {
      CesiumTilesetJson::EnumValue enumValue;
      enumValue.name = "valueName";
      enumValue.description = "valueDescription";
      enumValue.value = 42;

      CesiumGltf::EnumValue result =
          MetadataInterop::convertEnumValue(std::move(enumValue));

      CHECK_EQ(result.name, "valueName");
      CHECK_EQ(result.description, "valueDescription");
      CHECK_EQ(result.value, 42);
    }

    SUBCASE("from glTF to 3D Tiles 1.1") {
      CesiumGltf::EnumValue enumValue;
      enumValue.name = "valueName";
      enumValue.description = "valueDescription";
      enumValue.value = 42;

      CesiumTilesetJson::EnumValue result =
          MetadataInterop::convertEnumValue(std::move(enumValue));

      CHECK_EQ(result.name, "valueName");
      CHECK_EQ(result.description, "valueDescription");
      CHECK_EQ(result.value, 42);
    }
  }

  SUBCASE("convertEnum") {
    SUBCASE("from 3D Tiles 1.1 to glTF") {
      CesiumTilesetJson::Enum enum_;
      enum_.name = "enumName";
      enum_.description = "enumDescription";
      enum_.valueType = CesiumTilesetJson::Enum::ValueType::UINT8;
      enum_.values.resize(2);
      enum_.values[0].name = "first";
      enum_.values[0].value = 0;
      enum_.values[1].name = "second";
      enum_.values[1].value = 1;

      CesiumGltf::Enum result = MetadataInterop::convertEnum(std::move(enum_));

      CHECK_EQ(result.name, "enumName");
      CHECK_EQ(result.description, "enumDescription");
      CHECK_EQ(result.valueType, CesiumGltf::Enum::ValueType::UINT8);
      REQUIRE_EQ(result.values.size(), 2);
      CHECK_EQ(result.values[0].name, "first");
      CHECK_EQ(result.values[0].value, 0);
      CHECK_EQ(result.values[1].name, "second");
      CHECK_EQ(result.values[1].value, 1);
    }

    SUBCASE("from glTF to 3D Tiles 1.1") {
      CesiumGltf::Enum enum_;
      enum_.name = "enumName";
      enum_.description = "enumDescription";
      enum_.valueType = CesiumGltf::Enum::ValueType::UINT8;
      enum_.values.resize(2);
      enum_.values[0].name = "first";
      enum_.values[0].value = 0;
      enum_.values[1].name = "second";
      enum_.values[1].value = 1;

      CesiumTilesetJson::Enum result =
          MetadataInterop::convertEnum(std::move(enum_));

      CHECK_EQ(result.name, "enumName");
      CHECK_EQ(result.description, "enumDescription");
      CHECK_EQ(result.valueType, CesiumTilesetJson::Enum::ValueType::UINT8);
      REQUIRE_EQ(result.values.size(), 2);
      CHECK_EQ(result.values[0].name, "first");
      CHECK_EQ(result.values[0].value, 0);
      CHECK_EQ(result.values[1].name, "second");
      CHECK_EQ(result.values[1].value, 1);
    }
  }

  SUBCASE("convertSchema") {
    SUBCASE("from 3D Tiles 1.1 to glTF") {
      CesiumTilesetJson::Schema schema;
      schema.id = "schemaId";
      schema.name = "schemaName";
      schema.description = "schemaDescription";
      schema.version = "1.0.0";
      schema.classes["tile"].name = "Tile";
      schema.enums["color"].name = "Color";
      schema.enums["color"].values.resize(1);
      schema.enums["color"].values[0].name = "red";

      CesiumGltf::Schema result =
          MetadataInterop::convertSchema(std::move(schema));

      CHECK_EQ(result.id, "schemaId");
      CHECK_EQ(result.name, "schemaName");
      CHECK_EQ(result.description, "schemaDescription");
      CHECK_EQ(result.version, "1.0.0");
      REQUIRE_EQ(result.classes.size(), 1);
      CHECK_EQ(result.classes.at("tile").name, "Tile");
      REQUIRE_EQ(result.enums.size(), 1);
      CHECK_EQ(result.enums.at("color").name, "Color");
      REQUIRE_EQ(result.enums.at("color").values.size(), 1);
      CHECK_EQ(result.enums.at("color").values[0].name, "red");
    }

    SUBCASE("from glTF to 3D Tiles 1.1") {
      CesiumGltf::Schema schema;
      schema.id = "schemaId";
      schema.name = "schemaName";
      schema.description = "schemaDescription";
      schema.version = "1.0.0";
      schema.classes["tile"].name = "Tile";
      schema.enums["color"].name = "Color";
      schema.enums["color"].values.resize(1);
      schema.enums["color"].values[0].name = "red";

      CesiumTilesetJson::Schema result =
          MetadataInterop::convertSchema(std::move(schema));

      CHECK_EQ(result.id, "schemaId");
      CHECK_EQ(result.name, "schemaName");
      CHECK_EQ(result.description, "schemaDescription");
      CHECK_EQ(result.version, "1.0.0");
      REQUIRE_EQ(result.classes.size(), 1);
      CHECK_EQ(result.classes.at("tile").name, "Tile");
      REQUIRE_EQ(result.enums.size(), 1);
      CHECK_EQ(result.enums.at("color").name, "Color");
      REQUIRE_EQ(result.enums.at("color").values.size(), 1);
      CHECK_EQ(result.enums.at("color").values[0].name, "red");
    }
  }

  SUBCASE("convertPropertyStatistics") {
    SUBCASE("from 3D Tiles 1.1 to glTF") {
      CesiumTilesetJson::PropertyStatistics stats;
      stats.min = JsonValue(1.0);
      stats.max = JsonValue(9.0);
      stats.mean = JsonValue(5.0);
      stats.median = JsonValue(4.0);
      stats.standardDeviation = JsonValue(2.0);
      stats.variance = JsonValue(4.0);
      stats.sum = JsonValue(50.0);
      stats.occurrences["a"] = JsonValue(3.0);

      CesiumGltf::PropertyStatistics result =
          MetadataInterop::convertPropertyStatistics(std::move(stats));

      REQUIRE_UNARY(result.min);
      CHECK_EQ(result.min->getSafeNumber<double>(), 1.0);
      REQUIRE_UNARY(result.max);
      CHECK_EQ(result.max->getSafeNumber<double>(), 9.0);
      REQUIRE_UNARY(result.mean);
      CHECK_EQ(result.mean->getSafeNumber<double>(), 5.0);
      REQUIRE_UNARY(result.median);
      CHECK_EQ(result.median->getSafeNumber<double>(), 4.0);
      REQUIRE_UNARY(result.standardDeviation);
      CHECK_EQ(result.standardDeviation->getSafeNumber<double>(), 2.0);
      REQUIRE_UNARY(result.variance);
      CHECK_EQ(result.variance->getSafeNumber<double>(), 4.0);
      REQUIRE_UNARY(result.sum);
      CHECK_EQ(result.sum->getSafeNumber<double>(), 50.0);
      REQUIRE_EQ(result.occurrences.size(), 1);
      CHECK_EQ(result.occurrences.at("a").getSafeNumber<double>(), 3.0);
    }

    SUBCASE("from glTF to 3D Tiles 1.1") {
      CesiumGltf::PropertyStatistics stats;
      stats.min = JsonValue(1.0);
      stats.max = JsonValue(9.0);
      stats.mean = JsonValue(5.0);
      stats.median = JsonValue(4.0);
      stats.standardDeviation = JsonValue(2.0);
      stats.variance = JsonValue(4.0);
      stats.sum = JsonValue(50.0);
      stats.occurrences["a"] = JsonValue(3.0);

      CesiumTilesetJson::PropertyStatistics result =
          MetadataInterop::convertPropertyStatistics(std::move(stats));

      REQUIRE_UNARY(result.min);
      CHECK_EQ(result.min->getSafeNumber<double>(), 1.0);
      REQUIRE_UNARY(result.max);
      CHECK_EQ(result.max->getSafeNumber<double>(), 9.0);
      REQUIRE_UNARY(result.mean);
      CHECK_EQ(result.mean->getSafeNumber<double>(), 5.0);
      REQUIRE_UNARY(result.median);
      CHECK_EQ(result.median->getSafeNumber<double>(), 4.0);
      REQUIRE_UNARY(result.standardDeviation);
      CHECK_EQ(result.standardDeviation->getSafeNumber<double>(), 2.0);
      REQUIRE_UNARY(result.variance);
      CHECK_EQ(result.variance->getSafeNumber<double>(), 4.0);
      REQUIRE_UNARY(result.sum);
      CHECK_EQ(result.sum->getSafeNumber<double>(), 50.0);
      REQUIRE_EQ(result.occurrences.size(), 1);
      CHECK_EQ(result.occurrences.at("a").getSafeNumber<double>(), 3.0);
    }
  }

  SUBCASE("convertClassStatistics") {
    SUBCASE("from 3D Tiles 1.1 to glTF") {
      CesiumTilesetJson::ClassStatistics stats;
      stats.count = 100;
      stats.properties["p"].max = JsonValue(7.0);

      CesiumGltf::ClassStatistics result =
          MetadataInterop::convertClassStatistics(std::move(stats));

      REQUIRE_UNARY(result.count);
      CHECK_EQ(*result.count, 100);
      REQUIRE_EQ(result.properties.size(), 1);
      REQUIRE_UNARY(result.properties.at("p").max);
      CHECK_EQ(result.properties.at("p").max->getSafeNumber<double>(), 7.0);
    }

    SUBCASE("from glTF to 3D Tiles 1.1") {
      CesiumGltf::ClassStatistics stats;
      stats.count = 100;
      stats.properties["p"].max = JsonValue(7.0);

      CesiumTilesetJson::ClassStatistics result =
          MetadataInterop::convertClassStatistics(std::move(stats));

      REQUIRE_UNARY(result.count);
      CHECK_EQ(*result.count, 100);
      REQUIRE_EQ(result.properties.size(), 1);
      REQUIRE_UNARY(result.properties.at("p").max);
      CHECK_EQ(result.properties.at("p").max->getSafeNumber<double>(), 7.0);
    }
  }

  SUBCASE("convertStatistics") {
    SUBCASE("from 3D Tiles 1.1 to glTF") {
      CesiumTilesetJson::Statistics stats;
      stats.classes["c"].count = 10;
      stats.classes["c"].properties["p"].min = JsonValue(1.0);

      CesiumGltf::Statistics result =
          MetadataInterop::convertStatistics(std::move(stats));

      REQUIRE_EQ(result.classes.size(), 1);
      REQUIRE_UNARY(result.classes.at("c").count);
      CHECK_EQ(*result.classes.at("c").count, 10);
      REQUIRE_EQ(result.classes.at("c").properties.size(), 1);
      REQUIRE_UNARY(result.classes.at("c").properties.at("p").min);
      CHECK_EQ(
          result.classes.at("c")
              .properties.at("p")
              .min->getSafeNumber<double>(),
          1.0);
    }

    SUBCASE("from glTF to 3D Tiles 1.1") {
      CesiumGltf::Statistics stats;
      stats.classes["c"].count = 10;
      stats.classes["c"].properties["p"].min = JsonValue(1.0);

      CesiumTilesetJson::Statistics result =
          MetadataInterop::convertStatistics(std::move(stats));

      REQUIRE_EQ(result.classes.size(), 1);
      REQUIRE_UNARY(result.classes.at("c").count);
      CHECK_EQ(*result.classes.at("c").count, 10);
      REQUIRE_EQ(result.classes.at("c").properties.size(), 1);
      REQUIRE_UNARY(result.classes.at("c").properties.at("p").min);
      CHECK_EQ(
          result.classes.at("c")
              .properties.at("p")
              .min->getSafeNumber<double>(),
          1.0);
    }
  }
}
