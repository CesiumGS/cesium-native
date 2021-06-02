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
  REQUIRE(
      property.componentCount.value() == static_cast<int64_t>(componentCount));

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

static void checkStringProperty(const Model& model,
    const FeatureTable& featureTable,
    const Class& metaClass,
    const std::string& propertyName,
    const std::vector<std::string>& expected) 
{
  const ClassProperty& property = metaClass.properties.at(propertyName);
  REQUIRE(property.type == "STRING");

  const FeatureTableProperty& values = featureTable.properties.at(propertyName);
  const BufferView& valueBufferView = model.bufferViews[values.bufferView];
  const Buffer& valueBuffer = model.buffers[valueBufferView.buffer];

  const BufferView& offsetBufferView = model.bufferViews[values.stringOffsetBufferView];
  const Buffer& offsetBuffer = model.buffers[offsetBufferView.buffer];
  MetadataPropertyView<std::string_view> propertyView(
      gsl::span<const std::byte>(
          valueBuffer.cesium.data.data() + valueBufferView.byteOffset,
          valueBufferView.byteLength),
      gsl::span<const std::byte>(),
      gsl::span<const std::byte>(
          offsetBuffer.cesium.data.data() + offsetBufferView.byteOffset,
          offsetBufferView.byteLength),
      CesiumGltf::convertStringToPropertyType(values.offsetType),
      0,
      featureTable.count);

  REQUIRE(propertyView.size() == static_cast<size_t>(featureTable.count));
  for (size_t i = 0; i < expected.size(); ++i) {
    std::string_view val = propertyView[i];
    REQUIRE(val == expected[i]);
  }
}

TEST_CASE("Converts simple batch table to EXT_feature_metadata") {
  std::filesystem::path testFilePath = Cesium3DTiles_TEST_DATA_DIR;
  testFilePath = testFilePath / "BatchTables" / "batchedWithJson.b3dm";
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

  CesiumGltf::Class& defaultClass = firstClassIt->second;
  REQUIRE(defaultClass.properties.size() == 4);

  auto idIt = defaultClass.properties.find("id");
  REQUIRE(idIt != defaultClass.properties.end());
  auto longitudeIt = defaultClass.properties.find("Longitude");
  REQUIRE(longitudeIt != defaultClass.properties.end());
  auto latitudeIt = defaultClass.properties.find("Latitude");
  REQUIRE(latitudeIt != defaultClass.properties.end());
  auto heightIt = defaultClass.properties.find("Height");
  REQUIRE(heightIt != defaultClass.properties.end());

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

  // Check metadata values
  {
    std::vector<int8_t> expected = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    checkScalarProperty<int8_t>(
        *pResult->model,
        featureTable,
        defaultClass,
        "id",
        "INT8",
        expected);
  }

  {
    std::vector<double> expected = {
        11.762595914304256,
        13.992324123159051,
        7.490081690251827,
        13.484312580898404,
        11.481756005436182,
        7.836617760360241,
        9.338438434526324,
        13.513022359460592,
        13.74609257467091,
        10.145220385864377};
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
        -1.3196595204101946,
        -1.3196739888070643,
        -1.3196641114334025,
        -1.3196579305297966,
        -1.3196585149509301,
        -1.319678877969692,
        -1.3196612732428445,
        -1.3196718857616954,
        -1.3196471198757775,
        -1.319644104024109};
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
        0.6988582109,
        0.6988498770649103,
        0.6988533339856887,
        0.6988691467754378,
        0.698848878034009,
        0.6988592976292447,
        0.6988600642191055,
        0.6988670019309562,
        0.6988523191715889,
        0.6988697375823105};
    checkScalarProperty<double>(
        *pResult->model,
        featureTable,
        defaultClass,
        "Latitude",
        "FLOAT64",
        expected);
  }
}

TEST_CASE("Convert binary batch table to EXT_feature_metadata") {
  std::filesystem::path testFilePath = Cesium3DTiles_TEST_DATA_DIR;
  testFilePath =
      testFilePath / "BatchTables" / "batchedWithBatchTableBinary.b3dm";
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
    std::vector<int8_t> expected = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    checkScalarProperty<int8_t>(
        *pResult->model,
        featureTable,
        defaultClass,
        "id",
        "INT8",
        expected);
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

TEST_CASE("Upgrade json string and nested json metadata to string") {
  std::filesystem::path testFilePath = Cesium3DTiles_TEST_DATA_DIR;
  testFilePath =
      testFilePath / "BatchTables" / "batchedWithStringAndNestedJson.b3dm";
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

  std::vector<std::string> expected;
  for (int64_t i = 0; i < featureTable.count; ++i) {
    std::string v = std::string("{\"name\":\"building") + std::to_string(i) +
                    "\",\"year\":" + std::to_string(i) + "}";
    expected.push_back(v);
  }
  checkStringProperty(
      *pResult->model,
      featureTable,
      defaultClass,
      "info",
      expected);
}
