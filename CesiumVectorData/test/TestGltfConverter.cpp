#include <Cesium3DTilesReader/ExtensionSchemaMaxarContentGeoJsonReader.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGltf/AccessorUtility.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/ExtensionExtMeshFeatures.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/Node.h>
#include <CesiumNativeTests/readFile.h>
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

TEST_CASE("Conversion from GeoJSON to glTF") {
  SUBCASE("Feature ids") {
    std::filesystem::path dir(
        std::filesystem::path(CesiumVectorData_TEST_DATA_DIR) / "geojson");
    Result<GeoJsonDocument> doc =
        GeoJsonDocument::fromGeoJson(readFile(dir / "ship-trajectories.json"));
    REQUIRE(doc.value);
    ConverterResult converterResult =
        GltfConverter::convert(*doc.value, Ellipsoid::WGS84);
    REQUIRE(converterResult.value);
    const Model& model = *converterResult.value;
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
    REQUIRE(schemaResult.value);
    const Schema& schema = *schemaResult.value;
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
