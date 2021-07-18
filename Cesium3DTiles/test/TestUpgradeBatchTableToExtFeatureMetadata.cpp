#include "Batched3DModelContent.h"
#include "CesiumGltf/MeshPrimitiveEXT_feature_metadata.h"
#include "CesiumGltf/MetadataFeatureTableView.h"
#include "CesiumGltf/MetadataPropertyView.h"
#include "CesiumGltf/ModelEXT_feature_metadata.h"
#include "readFile.h"
#include "upgradeBatchTableToFeatureMetadata.h"
#include <catch2/catch.hpp>
#include <filesystem>
#include <rapidjson/document.h>
#include <set>
#include <spdlog/spdlog.h>

using namespace CesiumGltf;
using namespace Cesium3DTiles;

template <typename ExpectedType, typename PropertyViewType = ExpectedType>
static void checkScalarProperty(
    const Model& model,
    const FeatureTable& featureTable,
    const Class& metaClass,
    const std::string& propertyName,
    const std::string& expectedPropertyType,
    const std::vector<ExpectedType>& expected) {
  const ClassProperty& property = metaClass.properties.at(propertyName);
  REQUIRE(property.type == expectedPropertyType);
  REQUIRE(property.componentType.isNull());
  REQUIRE(property.componentCount == std::nullopt);

  MetadataFeatureTableView view(&model, &featureTable);
  std::optional<MetadataPropertyView<PropertyViewType>> propertyView =
      view.getPropertyView<PropertyViewType>(propertyName);
  REQUIRE(propertyView != std::nullopt);
  REQUIRE(propertyView->size() == featureTable.count);
  for (int64_t i = 0; i < propertyView->size(); ++i) {
    if constexpr (
        std::is_same_v<PropertyViewType, float> ||
        std::is_same_v<PropertyViewType, double>) {
      REQUIRE(propertyView->get(i) == Approx(expected[static_cast<size_t>(i)]));
    } else {
      REQUIRE(
          static_cast<ExpectedType>(propertyView->get(i)) ==
          expected[static_cast<size_t>(i)]);
    }
  }
}

template <typename ExpectedType, typename PropertyViewType = ExpectedType>
static void checkArrayProperty(
    const Model& model,
    const FeatureTable& featureTable,
    const Class& metaClass,
    const std::string& propertyName,
    int64_t expectedComponentCount,
    const std::string& expectedComponentType,
    const std::vector<std::vector<ExpectedType>>& expected) {
  const ClassProperty& property = metaClass.properties.at(propertyName);
  REQUIRE(property.type == "ARRAY");
  REQUIRE(property.componentType.getString() == expectedComponentType);
  if (expectedComponentCount > 0) {
    REQUIRE(
        property.componentCount.value() ==
        static_cast<int64_t>(expectedComponentCount));
  }

  MetadataFeatureTableView view(&model, &featureTable);
  std::optional<MetadataPropertyView<MetadataArrayView<PropertyViewType>>>
      propertyView = view.getPropertyView<MetadataArrayView<PropertyViewType>>(
          propertyName);
  REQUIRE(propertyView->size() == featureTable.count);
  for (size_t i = 0; i < expected.size(); ++i) {
    MetadataArrayView<PropertyViewType> val =
        propertyView->get(static_cast<int64_t>(i));
    if (expectedComponentCount > 0) {
      REQUIRE(val.size() == expectedComponentCount);
    }

    for (size_t j = 0; j < expected[i].size(); ++j) {
      if constexpr (
          std::is_same_v<ExpectedType, float> ||
          std::is_same_v<ExpectedType, double>) {
        REQUIRE(val[static_cast<int64_t>(j)] == Approx(expected[i][j]));
      } else {
        REQUIRE(val[static_cast<int64_t>(j)] == expected[i][j]);
      }
    }
  }
}

template <typename ExpectedType, typename PropertyViewType = ExpectedType>
static void createTestForScalarJson(
    const std::vector<ExpectedType>& expected,
    const std::string& expectedPropertyType) {
  Model model;

  rapidjson::Document featureTableJson;
  featureTableJson.SetObject();
  rapidjson::Value batchLength(rapidjson::kNumberType);
  batchLength.SetUint64(expected.size());
  featureTableJson.AddMember(
      "BATCH_LENGTH",
      batchLength,
      featureTableJson.GetAllocator());

  rapidjson::Document batchTableJson;
  batchTableJson.SetObject();
  rapidjson::Value scalarProperty(rapidjson::kArrayType);
  for (size_t i = 0; i < expected.size(); ++i) {
    if constexpr (std::is_same_v<ExpectedType, std::string>) {
      rapidjson::Value value(rapidjson::kStringType);
      value.SetString(
          expected[i].c_str(),
          static_cast<rapidjson::SizeType>(expected[i].size()),
          batchTableJson.GetAllocator());
      scalarProperty.PushBack(value, batchTableJson.GetAllocator());
    } else {
      scalarProperty.PushBack(expected[i], batchTableJson.GetAllocator());
    }
  }

  batchTableJson.AddMember(
      "scalarProp",
      scalarProperty,
      batchTableJson.GetAllocator());

  upgradeBatchTableToFeatureMetadata(
      spdlog::default_logger(),
      model,
      featureTableJson,
      batchTableJson,
      gsl::span<const std::byte>());

  ModelEXT_feature_metadata* metadata =
      model.getExtension<ModelEXT_feature_metadata>();
  REQUIRE(metadata != nullptr);

  std::optional<Schema> schema = metadata->schema;
  REQUIRE(schema != std::nullopt);

  const std::unordered_map<std::string, Class>& classes = schema->classes;
  REQUIRE(classes.size() == 1);

  const Class& defaultClass = classes.at("default");
  const std::unordered_map<std::string, ClassProperty>& properties =
      defaultClass.properties;
  REQUIRE(properties.size() == 1);

  const FeatureTable& featureTable = metadata->featureTables["default"];
  checkScalarProperty<ExpectedType, PropertyViewType>(
      model,
      featureTable,
      defaultClass,
      "scalarProp",
      expectedPropertyType,
      expected);
}

template <typename ExpectedType, typename PropertyViewType = ExpectedType>
static void createTestForArrayJson(
    const std::vector<std::vector<ExpectedType>>& expected,
    const std::string& expectedComponentType,
    int64_t componentCount) {
  Model model;

  rapidjson::Document featureTableJson;
  featureTableJson.SetObject();
  rapidjson::Value batchLength(rapidjson::kNumberType);
  batchLength.SetUint64(expected.size());
  featureTableJson.AddMember(
      "BATCH_LENGTH",
      batchLength,
      featureTableJson.GetAllocator());

  rapidjson::Document batchTableJson;
  batchTableJson.SetObject();
  rapidjson::Value fixedArrayProperties(rapidjson::kArrayType);
  for (size_t i = 0; i < expected.size(); ++i) {
    rapidjson::Value innerArray(rapidjson::kArrayType);
    for (size_t j = 0; j < expected[i].size(); ++j) {
      if constexpr (std::is_same_v<ExpectedType, std::string>) {
        rapidjson::Value value(rapidjson::kStringType);
        value.SetString(
            expected[i][j].c_str(),
            static_cast<rapidjson::SizeType>(expected[i][j].size()),
            batchTableJson.GetAllocator());
        innerArray.PushBack(value, batchTableJson.GetAllocator());
      } else {
        innerArray.PushBack(expected[i][j], batchTableJson.GetAllocator());
      }
    }
    fixedArrayProperties.PushBack(innerArray, batchTableJson.GetAllocator());
  }

  batchTableJson.AddMember(
      "fixedArrayProp",
      fixedArrayProperties,
      batchTableJson.GetAllocator());

  upgradeBatchTableToFeatureMetadata(
      spdlog::default_logger(),
      model,
      featureTableJson,
      batchTableJson,
      gsl::span<const std::byte>());

  ModelEXT_feature_metadata* metadata =
      model.getExtension<ModelEXT_feature_metadata>();
  REQUIRE(metadata != nullptr);

  std::optional<Schema> schema = metadata->schema;
  REQUIRE(schema != std::nullopt);

  const std::unordered_map<std::string, Class>& classes = schema->classes;
  REQUIRE(classes.size() == 1);

  const Class& defaultClass = classes.at("default");
  const std::unordered_map<std::string, ClassProperty>& properties =
      defaultClass.properties;
  REQUIRE(properties.size() == 1);

  const FeatureTable& featureTable = metadata->featureTables["default"];
  checkArrayProperty<ExpectedType, PropertyViewType>(
      model,
      featureTable,
      defaultClass,
      "fixedArrayProp",
      componentCount,
      expectedComponentType,
      expected);
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
  CHECK(
      idIt2->second.bufferView < static_cast<int32_t>(gltf.bufferViews.size()));
  CHECK(longitudeIt2->second.bufferView >= 0);
  CHECK(
      longitudeIt2->second.bufferView <
      static_cast<int32_t>(gltf.bufferViews.size()));
  CHECK(latitudeIt2->second.bufferView >= 0);
  CHECK(
      latitudeIt2->second.bufferView <
      static_cast<int32_t>(gltf.bufferViews.size()));
  CHECK(heightIt2->second.bufferView >= 0);
  CHECK(
      heightIt2->second.bufferView <
      static_cast<int32_t>(gltf.bufferViews.size()));

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
    std::vector<std::vector<double>> expected{
        {-1.31968, 0.698874, 6.155801922082901},
        {-1.3196832683949145, 0.6988615321420496, 13.410263679921627},
        {-1.3196637662080655, 0.6988736012180136, 6.1022464875131845},
        {-1.3196656317210846, 0.6988863062831799, 6.742499912157655},
        {-1.319679266890895,  0.6988864387845588, 6.869888566434383},
        {-1.319693717777418, 0.6988814788613282, 10.701326800510287},
        {-1.3196607462778132, 0.6988618972526105, 6.163868889212608},
        {-1.3196940116311096, 0.6988590050687061, 12.224825594574213},
        {-1.319683648959897, 0.6988690935212543, 12.546202838420868},
        {-1.3196959060375169, 0.6988854945986224, 7.632075032219291}
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

TEST_CASE("Upgrade json nested json metadata to string") {
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
  REQUIRE(featureTable.count == 10);

  {
    std::vector<std::string> expected;
    for (int64_t i = 0; i < featureTable.count; ++i) {
      std::string v = std::string("{\"name\":\"building") + std::to_string(i) +
                      "\",\"year\":" + std::to_string(i) + "}";
      expected.push_back(v);
    }
    checkScalarProperty<std::string, std::string_view>(
        *pResult->model,
        featureTable,
        defaultClass,
        "info",
        "STRING",
        expected);
  }

  {
    std::vector<std::vector<std::string>> expected;
    for (int64_t i = 0; i < featureTable.count; ++i) {
      std::vector<std::string> expectedVal;
      expectedVal.emplace_back("room" + std::to_string(i) + "_a");
      expectedVal.emplace_back("room" + std::to_string(i) + "_b");
      expectedVal.emplace_back("room" + std::to_string(i) + "_c");
      expected.emplace_back(std::move(expectedVal));
    }
    checkArrayProperty<std::string, std::string_view>(
        *pResult->model,
        featureTable,
        defaultClass,
        "rooms",
        3,
        "STRING",
        expected);
  }
}

TEST_CASE("Upgrade bool json to boolean binary") {
  Model model;

  rapidjson::Document featureTableJson;
  featureTableJson.SetObject();
  rapidjson::Value batchLength(rapidjson::kNumberType);
  batchLength.SetInt64(10);
  featureTableJson.AddMember(
      "BATCH_LENGTH",
      batchLength,
      featureTableJson.GetAllocator());

  std::vector<bool> expected =
      {true, false, true, true, false, true, false, true, false, true};
  rapidjson::Document batchTableJson;
  batchTableJson.SetObject();
  rapidjson::Value boolProperties(rapidjson::kArrayType);
  for (size_t i = 0; i < expected.size(); ++i) {
    boolProperties.PushBack(
        rapidjson::Value(static_cast<bool>(expected[i])),
        batchTableJson.GetAllocator());
  }
  batchTableJson.AddMember(
      "boolProp",
      boolProperties,
      batchTableJson.GetAllocator());

  upgradeBatchTableToFeatureMetadata(
      spdlog::default_logger(),
      model,
      featureTableJson,
      batchTableJson,
      gsl::span<const std::byte>());

  ModelEXT_feature_metadata* metadata =
      model.getExtension<ModelEXT_feature_metadata>();
  REQUIRE(metadata != nullptr);

  std::optional<Schema> schema = metadata->schema;
  REQUIRE(schema != std::nullopt);

  const std::unordered_map<std::string, Class>& classes = schema->classes;
  REQUIRE(classes.size() == 1);

  const Class& defaultClass = classes.at("default");
  const std::unordered_map<std::string, ClassProperty>& properties =
      defaultClass.properties;
  REQUIRE(properties.size() == 1);

  const ClassProperty& propertyClass = properties.at("boolProp");
  REQUIRE(propertyClass.type == "BOOLEAN");

  const FeatureTable& featureTable = metadata->featureTables["default"];
  checkScalarProperty(
      model,
      featureTable,
      defaultClass,
      "boolProp",
      "BOOLEAN",
      expected);
}

TEST_CASE("Upgrade fixed json number array") {
  SECTION("int8_t") {
    // clang-format off
    std::vector<std::vector<int8_t>> expected {
      {0, 1, 4, 1},
      {12, 50, -12, -1},
      {123, 10, 122, 3},
      {13, 45, 122, 94},
      {11, 22, 3, 5}};
    // clang-format on

    std::string expectedComponentType = "INT8";
    createTestForArrayJson(expected, expectedComponentType, 4);
  }

  SECTION("uint8_t") {
    // clang-format off
    std::vector<std::vector<uint8_t>> expected {
      {0, 1, 4, 1, 223},
      {12, 50, 242, 212, 11},
      {223, 10, 122, 3, 44},
      {13, 45, 122, 94, 244},
      {119, 112, 156, 5, 35}};
    // clang-format on

    std::string expectedComponentType = "UINT8";
    createTestForArrayJson(expected, expectedComponentType, 5);
  }

  SECTION("int16_t") {
    // clang-format off
    std::vector<std::vector<int16_t>> expected {
      {0, 1, 4, 4445},
      {12, 50, -12, -1},
      {123, 10, 3333, 3},
      {13, 450, 122, 94},
      {11, 22, 3, 50}};
    // clang-format on

    std::string expectedComponentType = "INT16";
    createTestForArrayJson(expected, expectedComponentType, 4);
  }

  SECTION("uint16_t") {
    // clang-format off
    std::vector<std::vector<uint16_t>> expected {
      {0, 1, 4, 65000},
      {12, 50, 12, 1},
      {123, 10, 33330, 3},
      {13, 450, 1220, 94},
      {11, 22, 3, 50000}};
    // clang-format on

    std::string expectedComponentType = "UINT16";
    createTestForArrayJson(expected, expectedComponentType, 4);
  }

  SECTION("int32_t") {
    // clang-format off
    std::vector<std::vector<int32_t>> expected {
      {0, 1, 4, 1},
      {1244, -500000, 1222, 544662},
      {123, -10, 122, 334},
      {13, 45, 122, 94},
      {11, 22, 3, 2147483647}};
    // clang-format on

    std::string expectedComponentType = "INT32";
    createTestForArrayJson(expected, expectedComponentType, 4);
  }

  SECTION("uint32_t") {
    // clang-format off
    std::vector<std::vector<uint32_t>> expected {
      {0, 1, 4, 1},
      {1244, 12200000, 1222, 544662},
      {123, 10, 122, 334},
      {13, 45, 122, 94},
      {11, 22, 3, 4294967295}};
    // clang-format on

    std::string expectedComponentType = "UINT32";
    createTestForArrayJson(expected, expectedComponentType, 4);
  }

  SECTION("int64_t") {
    // clang-format off
    std::vector<std::vector<int64_t>> expected {
      {0, 1, 4, 1},
      {1244, -9223372036854775807, 1222, 544662},
      {123, 10, 122, 334},
      {13, 45, 122, 94},
      {11, 22, 3, 9223372036854775807}};
    // clang-format on

    std::string expectedComponentType = "INT64";
    createTestForArrayJson(expected, expectedComponentType, 4);
  }

  SECTION("uint64_t") {
    // clang-format off
    std::vector<std::vector<uint64_t>> expected {
      {0, 1, 4, 1},
      {1244, 13223302036854775807u, 1222, 544662},
      {123, 10, 122, 334},
      {13, 45, 122, 94},
      {11, 22, 3, 13223302036854775807u}};
    // clang-format on

    std::string expectedComponentType = "UINT64";
    createTestForArrayJson(expected, expectedComponentType, 4);
  }

  SECTION("double") {
    // clang-format off
    std::vector<std::vector<double>> expected {
      {0.122, 1.1233, 4.113, 1.11},
      {1.244, 122.3, 1.222, 544.66},
      {12.003, 1.21, 2.123, 33.12},
      {1.333, 4.232, 1.422, 9.4},
      {1.1221, 2.2, 3.0, 122.31}};
    // clang-format on

    std::string expectedComponentType = "FLOAT64";
    createTestForArrayJson(expected, expectedComponentType, 4);
  }

  SECTION("string") {
    // clang-format off
    std::vector<std::vector<std::string>> expected{
      {"Test0", "Test1", "Test2", "Test4"},
      {"Test5", "Test6", "Test7", "Test8"},
      {"Test9", "Test10", "Test11", "Test12"},
      {"Test13", "Test14", "Test15", "Test16"},
    };
    // clang-format on

    createTestForArrayJson<std::string, std::string_view>(
        expected,
        "STRING",
        4);
  }

  SECTION("Boolean") {
    // clang-format off
    std::vector<std::vector<bool>> expected{
      {true, true, false, true, false, true},
      {true, false, true, false, true, true},
      {false, true, true, false, false, true},
      {false, true, true, true, true, true},
    };
    // clang-format on

    createTestForArrayJson(expected, "BOOLEAN", 6);
  }
}

TEST_CASE("Upgrade dynamic json number array") {
  SECTION("int8_t") {
    // clang-format off
    std::vector<std::vector<int8_t>> expected {
      {0, 1, 4},
      {12, 50, -12},
      {123, 10, 122, 3, 23},
      {13, 45},
      {11, 22, 3, 5, 33, 12, -122}};
    // clang-format on

    std::string expectedComponentType = "INT8";
    createTestForArrayJson(expected, expectedComponentType, 0);
  }

  SECTION("uint8_t") {
    // clang-format off
    std::vector<std::vector<uint8_t>> expected {
      {0, 223},
      {12, 50, 242, 212, 11},
      {223},
      {13, 45},
      {119, 112, 156, 5, 35, 244, 122}};
    // clang-format on

    std::string expectedComponentType = "UINT8";
    createTestForArrayJson(expected, expectedComponentType, 0);
  }

  SECTION("int16_t") {
    // clang-format off
    std::vector<std::vector<int16_t>> expected {
      {0, 1, 4, 4445, 12333},
      {12, 50, -12, -1},
      {123, 10},
      {13, 450, 122, 94, 334},
      {11, 22, 3, 50, 455, 122, 3333, 5555, 12233}};
    // clang-format on

    std::string expectedComponentType = "INT16";
    createTestForArrayJson(expected, expectedComponentType, 0);
  }

  SECTION("uint16_t") {
    // clang-format off
    std::vector<std::vector<uint16_t>> expected {
      {0, 1},
      {12, 50, 12, 1, 333, 5666},
      {123, 10, 33330, 3, 1},
      {13, 1220},
      {11, 22, 3, 50000, 333}};
    // clang-format on

    std::string expectedComponentType = "UINT16";
    createTestForArrayJson(expected, expectedComponentType, 0);
  }

  SECTION("int32_t") {
    // clang-format off
    std::vector<std::vector<int32_t>> expected {
      {0, 1},
      {1244, -500000, 1222, 544662},
      {123, -10},
      {13},
      {11, 22, 3, 2147483647, 12233}};
    // clang-format on

    std::string expectedComponentType = "INT32";
    createTestForArrayJson(expected, expectedComponentType, 0);
  }

  SECTION("uint32_t") {
    // clang-format off
    std::vector<std::vector<uint32_t>> expected {
      {0, 1},
      {1244, 12200000, 1222, 544662},
      {123, 10},
      {13, 45, 122, 94, 333, 212, 534, 1122},
      {11, 22, 3, 4294967295}};
    // clang-format on

    std::string expectedComponentType = "UINT32";
    createTestForArrayJson(expected, expectedComponentType, 0);
  }

  SECTION("int64_t") {
    // clang-format off
    std::vector<std::vector<int64_t>> expected {
      {0, 1, 4, 1},
      {1244, -9223372036854775807, 1222, 544662, 12233},
      {123},
      {13, 45},
      {11, 22, 3, 9223372036854775807, 12333}};
    // clang-format on

    std::string expectedComponentType = "INT64";
    createTestForArrayJson(expected, expectedComponentType, 0);
  }

  SECTION("uint64_t") {
    // clang-format off
    std::vector<std::vector<uint64_t>> expected {
      {1},
      {1244, 13223302036854775807u, 1222, 544662},
      {123, 10, 2},
      {13, 94},
      {11, 22, 3, 13223302036854775807u, 32323}};
    // clang-format on

    std::string expectedComponentType = "UINT64";
    createTestForArrayJson(expected, expectedComponentType, 0);
  }

  SECTION("double") {
    // clang-format off
    std::vector<std::vector<double>> expected {
      {0.122, 1.1233},
      {1.244, 122.3, 1.222, 544.66, 323.122},
      {12.003, 1.21, 2.123, 33.12, 122.2},
      {1.333},
      {1.1221, 2.2}};
    // clang-format on

    std::string expectedComponentType = "FLOAT64";
    createTestForArrayJson(expected, expectedComponentType, 0);
  }

  SECTION("string") {
    // clang-format off
    std::vector<std::vector<std::string>> expected{
      {"This is Test", "Another Test"},
      {"Good morning", "How you doing?", "The book in the freezer", "Batman beats superman", ""},
      {"Test9", "Test10", "", "Test12", ""},
      {"Test13", ""},
    };
    // clang-format on

    createTestForArrayJson<std::string, std::string_view>(
        expected,
        "STRING",
        0);
  }

  SECTION("Boolean") {
    // clang-format off
    std::vector<std::vector<bool>> expected{
      {true, true, false, true, false, false, true},
      {true, false},
      {false, true, true, false},
      {false, true, true},
      {true, true, true, true, false, false}
    };
    // clang-format on

    createTestForArrayJson(expected, "BOOLEAN", 0);
  }
}

TEST_CASE("Upgrade scalar json") {
  SECTION("Uint32") {
    std::vector<uint32_t> expected{32, 45, 21, 65, 78};
    createTestForScalarJson<uint32_t, int8_t>(expected, "INT8");
  }

  SECTION("Boolean") {
    std::vector<bool> expected{true, false, true, false, true, true, false};
    createTestForScalarJson(expected, "BOOLEAN");
  }

  SECTION("String") {
    std::vector<std::string> expected{"Test 0", "Test 1", "Test 2", "Test 3"};
    createTestForScalarJson<std::string, std::string_view>(expected, "STRING");
  }
}
