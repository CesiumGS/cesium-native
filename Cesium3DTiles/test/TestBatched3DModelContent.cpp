#include "Batched3DModelContent.h"
#include "CesiumGltf/MeshPrimitiveEXT_feature_metadata.h"
#include "CesiumGltf/MetadataPropertyView.h"
#include "CesiumGltf/ModelEXT_feature_metadata.h"
#include "catch2/catch.hpp"
#include "readFile.h"
#include <filesystem>
#include <set>
#include <spdlog/spdlog.h>

using namespace CesiumGltf;
using namespace Cesium3DTiles;

template <typename T>
static void checkScalarProperty(
    const Model& model,
    const FeatureTable& featureTable,
    const Class& metaClass,
    const std::string& propertyName,
    const std::string& expectedPropertyType,
    const std::vector<T>& expected) {
  const ClassProperty& property = metaClass.properties.at(propertyName);
  REQUIRE(property.type == expectedPropertyType);

  const FeatureTableProperty& values = featureTable.properties.at(propertyName);
  const BufferView& valueBufferView = model.bufferViews[values.bufferView];
  const Buffer& valueBuffer = model.buffers[valueBufferView.buffer];
  MetadataPropertyView<T> propertyView(
      gsl::span<const std::byte>(
          valueBuffer.cesium.data.data() + valueBufferView.byteOffset,
          valueBufferView.byteLength),
      gsl::span<const std::byte>(),
      gsl::span<const std::byte>(),
      PropertyType::None,
      0,
      featureTable.count);

  REQUIRE(propertyView.size() == static_cast<size_t>(featureTable.count));
  for (size_t i = 0; i < propertyView.size(); ++i) {
    REQUIRE(propertyView[i] == Approx(expected[i]));
  }
}

template <typename T>
static void checkArrayProperty(
    const Model& model,
    const FeatureTable& featureTable,
    const Class& metaClass,
    const std::string& propertyName,
    size_t componentCount,
    const std::string& expectedComponentType,
    const std::vector<T>& expected) {
  const ClassProperty& property = metaClass.properties.at(propertyName);
  REQUIRE(property.type == "ARRAY");
  REQUIRE(property.componentType.getString() == expectedComponentType);
  REQUIRE(property.componentCount == componentCount);

  const FeatureTableProperty& values = featureTable.properties.at(propertyName);
  const BufferView& valueBufferView = model.bufferViews[values.bufferView];
  const Buffer& valueBuffer = model.buffers[valueBufferView.buffer];
  MetadataPropertyView<MetaArrayView<T>> propertyView(
      gsl::span<const std::byte>(
          valueBuffer.cesium.data.data() + valueBufferView.byteOffset,
          valueBufferView.byteLength),
      gsl::span<const std::byte>(),
      gsl::span<const std::byte>(),
      PropertyType::None,
      componentCount,
      featureTable.count);

  REQUIRE(propertyView.size() == static_cast<size_t>(featureTable.count));
  for (size_t i = 0; i < expected.size() / componentCount; ++i) {
    MetaArrayView<T> val = propertyView[i];
    for (size_t j = 0; j < componentCount; ++j) {
      REQUIRE(val[j] == expected[i * componentCount + j]);
    }
  }
}

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

TEST_CASE("Convert binary batch table to EXT_feature_metadata") {
  std::filesystem::path testFilePath = Cesium3DTiles_TEST_DATA_DIR;
  testFilePath = testFilePath / "B3dm" / "batchedWithBatchTableBinary.b3dm";
  std::vector<std::byte> b3dm = readFile(testFilePath);

  std::unique_ptr<TileContentLoadResult> pResult =
      Batched3DModelContent::load(spdlog::default_logger(), "test.url", b3dm);
  REQUIRE(pResult != nullptr);
  REQUIRE(pResult->model != std::nullopt);

  ModelEXT_feature_metadata* metadata =
      pResult->model->getExtension<ModelEXT_feature_metadata>();
  REQUIRE(metadata != nullptr);

  std::optional<Schema> schema = metadata->schema;
  REQUIRE(schema != std::nullopt);

  const std::unordered_map<std::string, Class>& classes = schema->classes;
  REQUIRE(classes.size() == 1);

  const Class& defaultClass = classes.at("default");
  const std::unordered_map<std::string, ClassProperty>& properties =
      defaultClass.properties;
  REQUIRE(properties.size() == 6);

  const FeatureTable& featureTable = metadata->featureTables["default"];

  {
    const ClassProperty& id = properties.at("id");
    REQUIRE(id.type == "INT8");

    const FeatureTableProperty& idValues = featureTable.properties.at("id");
    const BufferView& valueBufferView =
        pResult->model->bufferViews[idValues.bufferView];
    const Buffer& valueBuffer = pResult->model->buffers[valueBufferView.buffer];
    MetadataPropertyView<uint8_t> idView(
        gsl::span<const std::byte>(
            valueBuffer.cesium.data.data() + valueBufferView.byteOffset,
            valueBufferView.byteLength),
        gsl::span<const std::byte>(),
        gsl::span<const std::byte>(),
        PropertyType::None,
        0,
        featureTable.count);

    REQUIRE(idView.size() == static_cast<size_t>(featureTable.count));
    for (size_t i = 0; i < idView.size(); ++i) {
      REQUIRE(idView[i] == i);
    }
  }

  {
    std::vector<double> expected = {
        6.155801922082901,
        13.410263679921627,
        6.1022464875131845,
        6.742499912157655,
        6.869888566434383,
        10.701326800510287,
        6.163868889212608,
        12.224825594574213,
        12.546202838420868,
        7.632075032219291};
    checkScalarProperty<double>(
        *pResult->model,
        featureTable,
        defaultClass,
        "Height",
        "FLOAT64",
        expected);
  }

  {
    std::vector<double> expected = {
        -1.31968,
        -1.3196832683949145,
        -1.3196637662080655,
        -1.3196656317210846,
        -1.319679266890895,
        -1.319693717777418,
        -1.3196607462778132,
        -1.3196940116311096,
        -1.319683648959897,
        -1.3196959060375169};
    checkScalarProperty<double>(
        *pResult->model,
        featureTable,
        defaultClass,
        "Longitude",
        "FLOAT64",
        expected);
  }

  {
    std::vector<double> expected = {
        0.698874,
        0.6988615321420496,
        0.6988736012180136,
        0.6988863062831799,
        0.6988864387845588,
        0.6988814788613282,
        0.6988618972526105,
        0.6988590050687061,
        0.6988690935212543,
        0.6988854945986224};
    checkScalarProperty<double>(
        *pResult->model,
        featureTable,
        defaultClass,
        "Latitude",
        "FLOAT64",
        expected);
  }

  {
    std::vector<uint8_t> expected(10, 255);
    checkScalarProperty<uint8_t>(
        *pResult->model,
        featureTable,
        defaultClass,
        "code",
        "UINT8",
        expected);
  }

  {
    // clang-format off
    std::vector<double> expected{ 
        -1.31968, 0.698874, 6.155801922082901,
        -1.3196832683949145, 0.6988615321420496, 13.410263679921627,
        -1.3196637662080655, 0.6988736012180136, 6.1022464875131845,
        -1.3196656317210846, 0.6988863062831799, 6.742499912157655,
        -1.319679266890895,  0.6988864387845588, 6.869888566434383,
        -1.319693717777418, 0.6988814788613282, 10.701326800510287,
        -1.3196607462778132, 0.6988618972526105, 6.163868889212608,
        -1.3196940116311096, 0.6988590050687061, 12.224825594574213,
        -1.319683648959897, 0.6988690935212543, 12.546202838420868,
        -1.3196959060375169, 0.6988854945986224, 7.632075032219291
    };
    // clang-format on
    checkArrayProperty<double>(
        *pResult->model,
        featureTable,
        defaultClass,
        "cartographic",
        3,
        "FLOAT64",
        expected);
  }
}
