#include <Cesium3DTilesReader/ExtensionSchemaMaxarContentGeoJsonReader.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGltf/AccessorUtility.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/ExtensionExtMeshFeatures.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/Node.h>
#include <CesiumGltf/PropertyTableView.h>
#include <CesiumNativeTests/readFile.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/Result.h>
#include <CesiumVectorData/GeoJsonDocument.h>
#include <CesiumVectorData/GeoJsonObject.h>
#include <CesiumVectorData/GeoJsonObjectTypes.h>
#include <CesiumVectorData/GltfConverter.h>

#include <doctest/doctest.h>
#include <glm/common.hpp>

#include <filesystem>
#include <set>

using namespace CesiumGeospatial;
using namespace CesiumGltf;
using namespace CesiumUtility;
using namespace CesiumVectorData;

TEST_CASE("Conversion from geoJSON to glTF") {
  std::filesystem::path dir(
      std::filesystem::path(CesiumVectorData_TEST_DATA_DIR) / "geojson");
  Result<GeoJsonDocument> doc =
      GeoJsonDocument::fromGeoJson(readFile(dir / "ship-trajectories.json"));
  REQUIRE(doc.value);
  IntrusivePointer<Schema> pSchema;
  Schema& schema = pSchema.emplace();
  schema.id = "default";
  Class geoJsonClass;
  geoJsonClass.name = "ShipTrajectory";
  ClassProperty trajIdClassProperty;
  trajIdClassProperty.type = ClassProperty::Type::SCALAR;
  trajIdClassProperty.componentType = ClassProperty::ComponentType::INT64;
  geoJsonClass.properties["traj_id"] = std::move(trajIdClassProperty);
  ClassProperty shipTypeClassProperty;
  shipTypeClassProperty.type = ClassProperty::Type::STRING;
  geoJsonClass.properties["ShipType"] = std::move(shipTypeClassProperty);
  schema.classes["ShipRoute"] = std::move(geoJsonClass);
  ConverterResult converterResult =
      GltfConverter::convert(*doc.value, Ellipsoid::WGS84, pSchema);
  REQUIRE(converterResult.value);
  REQUIRE(converterResult.errors.errors.empty());
  REQUIRE(converterResult.errors.warnings.empty());
  const Model& model = *converterResult.value;
  SUBCASE("Feature ids") {
    model.forEachPrimitiveInScene(
        -1,
        [](const Model& model,
           const Node&,
           const Mesh&,
           const MeshPrimitive& prim,
           const glm::dmat4&) {
          const ExtensionExtMeshFeatures* pPrimitiveExtension =
              prim.getExtension<ExtensionExtMeshFeatures>();
          REQUIRE(pPrimitiveExtension);
          REQUIRE(!pPrimitiveExtension->featureIds.empty());
          const FeatureId& featureId = pPrimitiveExtension->featureIds[0];
          REQUIRE(featureId.attribute);
          int64_t attribute = *featureId.attribute;
          int32_t positionAccessor = prim.attributes.at("POSITION");
          int64_t positionCount =
              model.accessors[uint32_t(positionAccessor)].count;
          FeatureIdAccessorType featureIdView =
              getFeatureIdAccessorView(model, prim, int32_t(attribute));
          std::visit(
              [positionCount, &featureId](auto&& view) {
                REQUIRE(view.status() == AccessorViewStatus::Valid);
                CHECK(view.size() == positionCount);
                std::set<uint32_t> uniqueFeatures;
                for (int64_t i = 0; i < view.size(); ++i) {
                  uniqueFeatures.insert(uint32_t(view[i]));
                }
                CHECK(uniqueFeatures.size() == size_t(featureId.featureCount));
              },
              featureIdView);
        });
  }
  SUBCASE("GeoJSON metadata") {
    const auto* pMetadata =
        model.getExtension<ExtensionModelExtStructuralMetadata>();
    REQUIRE(pMetadata);
    REQUIRE(pMetadata->schema);
    const std::unordered_map<std::string, Class>& classes =
        pMetadata->schema->classes;
    auto classItr = classes.find("ShipRoute");
    REQUIRE(classItr != classes.end());
    const Class& routeClass = classItr->second;
    const std::unordered_map<std::string, ClassProperty>& properties =
        routeClass.properties;
    auto classPropertyItr = properties.find("traj_id");
    REQUIRE(classPropertyItr != properties.end());
    const ClassProperty& trajId = classPropertyItr->second;
    REQUIRE(trajId.type == ClassProperty::Type::SCALAR);
    REQUIRE(trajId.componentType == ClassProperty::ComponentType::INT64);
    auto propertyTableItr = std::find_if(
        pMetadata->propertyTables.begin(),
        pMetadata->propertyTables.end(),
        [](const PropertyTable& propTable) {
          return propTable.classProperty == "ShipRoute";
        });
    REQUIRE(propertyTableItr != pMetadata->propertyTables.end());
    PropertyTableView view(model, *propertyTableItr);
    REQUIRE(view.status() == PropertyTableViewStatus::Valid);
    auto trajIdPropertyView = view.getPropertyView<int64_t>("traj_id");
    REQUIRE(
        trajIdPropertyView.status() == PropertyTablePropertyViewStatus::Valid);
    bool missingValue = !trajIdPropertyView.get(0);
    bool invalidValue = false;
    // feature id 0 is reserved for missing features.
    for (int64_t i = 1; i < trajIdPropertyView.size(); ++i) {
      auto val = trajIdPropertyView.get(i);
      if (!val) {
        missingValue = true;
      } else {
        // We know the values in the GeoJSON file.
        invalidValue = invalidValue || *val > 538002553 || *val < 200000000;
      }
    }
    REQUIRE(!missingValue);
    REQUIRE(!invalidValue);
    auto shipTypePropertyView =
        view.getPropertyView<std::string_view>("ShipType");
    REQUIRE(
        shipTypePropertyView.status() ==
        PropertyTablePropertyViewStatus::Valid);
    missingValue = !shipTypePropertyView.get(0);
    invalidValue = false;
    for (int64_t i = 1; i < shipTypePropertyView.size(); ++i) {
      auto val = shipTypePropertyView.get(i);
      if (!val) {
        missingValue = true;
      } else {
        invalidValue = invalidValue || val->empty();
      }
    }
    REQUIRE(!missingValue);
    REQUIRE(!invalidValue);
    REQUIRE(*shipTypePropertyView.get(1) == "Cargo");
  }
}

TEST_CASE("Convert GeoJSON schema") {
  SUBCASE("Verify schema") {
    std::filesystem::path dir(
        std::filesystem::path(CesiumVectorData_TEST_DATA_DIR));
    Cesium3DTilesReader::ExtensionSchemaMaxarContentGeoJsonReader
        maxarSchemaReader;
    auto schemaReadResult =
        maxarSchemaReader.readFromJson(readFile(dir / "sample-schema.json"));
    REQUIRE(schemaReadResult.value);
    REQUIRE(schemaReadResult.errors.empty());
    CesiumVectorData::ConvertSchemaResult schemaResult =
        CesiumVectorData::GltfConverter::convertSchema(*schemaReadResult.value);
    REQUIRE(schemaResult.pValue);
    const Schema& schema = *schemaResult.pValue;
    auto classIt = schema.classes.find("test_point");
    REQUIRE(classIt != schema.classes.end());
    const Class& schemaClass = classIt->second;
    auto propertiesIt = schemaClass.properties.find("FULL_NAME");
    REQUIRE(propertiesIt != schemaClass.properties.end());
    REQUIRE(propertiesIt->second.type == ClassProperty::Type::STRING);
    propertiesIt = schemaClass.properties.find("HGT");
    REQUIRE(propertiesIt != schemaClass.properties.end());
    REQUIRE(propertiesIt->second.type == ClassProperty::Type::SCALAR);
    REQUIRE(
        propertiesIt->second.componentType ==
        ClassProperty::ComponentType::FLOAT64);
    propertiesIt = schemaClass.properties.find("AT005_CAB");
    REQUIRE(propertiesIt != schemaClass.properties.end());
    REQUIRE(propertiesIt->second.type == ClassProperty::Type::SCALAR);
    REQUIRE(
        propertiesIt->second.componentType ==
        ClassProperty::ComponentType::INT64);
  }
}
