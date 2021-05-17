#include "Batched3DModelContent.h"
#include "CesiumGltf/MeshPrimitiveEXT_feature_metadata.h"
#include "CesiumGltf/ModelEXT_feature_metadata.h"
#include "catch2/catch.hpp"
#include "readFile.h"
#include <filesystem>
#include <set>
#include <spdlog/spdlog.h>

using namespace CesiumGltf;
using namespace Cesium3DTiles;

TEST_CASE("Converts simple batch table to EXT_feature_metadata") {
  std::filesystem::path testFilePath = Cesium3DTiles_TEST_DATA_DIR;
  testFilePath = testFilePath / "Tileset" / "ll.b3dm";
  std::vector<std::byte> b3dm = readFile(testFilePath);

  std::unique_ptr<TileContentLoadResult> pResult =
      Batched3DModelContent::load(spdlog::default_logger(), "test.url", b3dm);

  REQUIRE(pResult->model);

  Model& gltf = *pResult->model;

  ModelEXT_feature_metadata* pExtension =
      gltf.getExtension<ModelEXT_feature_metadata>();
  REQUIRE(pExtension);

  // Check the schema
  REQUIRE(pExtension->schema);
  REQUIRE(pExtension->schema->classes.size() == 1);

  auto firstClassIt = pExtension->schema->classes.begin();
  CHECK(firstClassIt->first == "default");

  CesiumGltf::Class& classObject = firstClassIt->second;
  REQUIRE(classObject.properties.size() == 4);

  auto idIt = classObject.properties.find("id");
  REQUIRE(idIt != classObject.properties.end());
  auto longitudeIt = classObject.properties.find("Longitude");
  REQUIRE(longitudeIt != classObject.properties.end());
  auto latitudeIt = classObject.properties.find("Latitude");
  REQUIRE(latitudeIt != classObject.properties.end());
  auto heightIt = classObject.properties.find("Height");
  REQUIRE(heightIt != classObject.properties.end());

  CHECK(idIt->second.type == "INT8");
  CHECK(longitudeIt->second.type == "FLOAT64");
  CHECK(latitudeIt->second.type == "FLOAT64");
  CHECK(heightIt->second.type == "FLOAT64");

  // Check the feature table
  auto firstFeatureTableIt = pExtension->featureTables.begin();
  REQUIRE(firstFeatureTableIt != pExtension->featureTables.end());

  FeatureTable& featureTable = firstFeatureTableIt->second;
  CHECK(featureTable.classProperty == "default");
  REQUIRE(featureTable.properties.size() == 4);

  auto idIt2 = featureTable.properties.find("id");
  REQUIRE(idIt2 != featureTable.properties.end());
  auto longitudeIt2 = featureTable.properties.find("Longitude");
  REQUIRE(longitudeIt2 != featureTable.properties.end());
  auto latitudeIt2 = featureTable.properties.find("Latitude");
  REQUIRE(latitudeIt2 != featureTable.properties.end());
  auto heightIt2 = featureTable.properties.find("Height");
  REQUIRE(heightIt2 != featureTable.properties.end());

  CHECK(idIt2->second.bufferView >= 0);
  CHECK(idIt2->second.bufferView < gltf.bufferViews.size());
  CHECK(longitudeIt2->second.bufferView >= 0);
  CHECK(longitudeIt2->second.bufferView < gltf.bufferViews.size());
  CHECK(latitudeIt2->second.bufferView >= 0);
  CHECK(latitudeIt2->second.bufferView < gltf.bufferViews.size());
  CHECK(heightIt2->second.bufferView >= 0);
  CHECK(heightIt2->second.bufferView < gltf.bufferViews.size());

  // Make sure all property bufferViews are unique
  std::set<int32_t> bufferViews{
      idIt2->second.bufferView,
      longitudeIt2->second.bufferView,
      latitudeIt2->second.bufferView,
      heightIt2->second.bufferView};
  CHECK(bufferViews.size() == 4);

  // Check the mesh primitives
  CHECK(!gltf.meshes.empty());

  for (Mesh& mesh : gltf.meshes) {
    CHECK(!mesh.primitives.empty());
    for (MeshPrimitive& primitive : mesh.primitives) {
      CHECK(
          primitive.attributes.find("_FEATURE_ID_0") !=
          primitive.attributes.end());
      CHECK(
          primitive.attributes.find("_FEATURE_ID_1") ==
          primitive.attributes.end());

      MeshPrimitiveEXT_feature_metadata* pPrimitiveExtension =
          primitive.getExtension<MeshPrimitiveEXT_feature_metadata>();
      REQUIRE(pPrimitiveExtension);
      REQUIRE(pPrimitiveExtension->featureIdAttributes.size() == 1);

      FeatureIDAttribute& attribute =
          pPrimitiveExtension->featureIdAttributes[0];
      CHECK(attribute.featureIds.attribute == "_FEATURE_ID_0");
      CHECK(attribute.featureTable == "default");
    }
  }
}
