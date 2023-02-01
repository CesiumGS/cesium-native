#include "BatchTableToGltfFeatureMetadata.h"
#include "ConvertTileToGltf.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/HttpHeaders.h>
#include <CesiumGltf/ExtensionMeshPrimitiveExtFeatureMetadata.h>
#include <CesiumGltf/ExtensionModelExtFeatureMetadata.h>
#include <CesiumGltf/MetadataFeatureTableView.h>
#include <CesiumGltf/MetadataPropertyView.h>

#include <catch2/catch.hpp>
#include <rapidjson/document.h>
#include <spdlog/sinks/ringbuffer_sink.h>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <set>

using namespace CesiumGltf;
using namespace Cesium3DTilesSelection;

template <typename ExpectedType, typename PropertyViewType = ExpectedType>
static void checkScalarProperty(
    const Model& model,
    const FeatureTable& featureTable,
    const Class& metaClass,
    const std::string& propertyName,
    const std::string& expectedPropertyType,
    const std::vector<ExpectedType>& expected,
    size_t expectedTotalInstances) {
  const ClassProperty& property = metaClass.properties.at(propertyName);
  REQUIRE(property.type == expectedPropertyType);
  REQUIRE(property.componentType == std::nullopt);
  REQUIRE(property.componentCount == std::nullopt);

  MetadataFeatureTableView view(&model, &featureTable);
  std::optional<MetadataPropertyView<PropertyViewType>> propertyView =
      view.getPropertyView<PropertyViewType>(propertyName);
  REQUIRE(propertyView != std::nullopt);
  REQUIRE(propertyView->size() == featureTable.count);
  REQUIRE(propertyView->size() == static_cast<int64_t>(expectedTotalInstances));
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
    const std::vector<std::vector<ExpectedType>>& expected,
    size_t expectedTotalInstances) {
  const ClassProperty& property = metaClass.properties.at(propertyName);
  REQUIRE(property.type == "ARRAY");
  REQUIRE(property.componentType.has_value());
  REQUIRE(*property.componentType == expectedComponentType);
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
  REQUIRE(propertyView->size() == static_cast<int64_t>(expectedTotalInstances));
  for (size_t i = 0; i < expectedTotalInstances; ++i) {
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
    const std::string& expectedPropertyType,
    size_t totalInstances) {
  Model model;

  rapidjson::Document featureTableJson;
  featureTableJson.SetObject();
  rapidjson::Value batchLength(rapidjson::kNumberType);
  batchLength.SetUint64(totalInstances);
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
      scalarProperty.PushBack(
          ExpectedType(expected[i]),
          batchTableJson.GetAllocator());
    }
  }

  batchTableJson.AddMember(
      "scalarProp",
      scalarProperty,
      batchTableJson.GetAllocator());

  auto errors = BatchTableToGltfFeatureMetadata::convertFromB3dm(
      featureTableJson,
      batchTableJson,
      gsl::span<const std::byte>(),
      model);

  ExtensionModelExtFeatureMetadata* metadata =
      model.getExtension<ExtensionModelExtFeatureMetadata>();
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
      expected,
      totalInstances);
}

template <typename ExpectedType, typename PropertyViewType = ExpectedType>
static void createTestForArrayJson(
    const std::vector<std::vector<ExpectedType>>& expected,
    const std::string& expectedComponentType,
    int64_t componentCount,
    size_t totalInstances) {
  Model model;

  rapidjson::Document featureTableJson;
  featureTableJson.SetObject();
  rapidjson::Value batchLength(rapidjson::kNumberType);
  batchLength.SetUint64(totalInstances);
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
        innerArray.PushBack(
            ExpectedType(expected[i][j]),
            batchTableJson.GetAllocator());
      }
    }
    fixedArrayProperties.PushBack(innerArray, batchTableJson.GetAllocator());
  }

  batchTableJson.AddMember(
      "fixedArrayProp",
      fixedArrayProperties,
      batchTableJson.GetAllocator());

  auto errors = BatchTableToGltfFeatureMetadata::convertFromB3dm(
      featureTableJson,
      batchTableJson,
      gsl::span<const std::byte>(),
      model);

  ExtensionModelExtFeatureMetadata* metadata =
      model.getExtension<ExtensionModelExtFeatureMetadata>();
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
      expected,
      totalInstances);
}

std::set<int32_t> getUniqueBufferViewIds(
    const std::vector<Accessor>& accessors,
    FeatureTable& featureTable) {
  std::set<int32_t> result;
  for (auto it = accessors.begin(); it != accessors.end(); it++) {
    result.insert(it->bufferView);
  }

  auto& properties = featureTable.properties;
  for (auto it = properties.begin(); it != properties.end(); it++) {
    auto& property = it->second;
    result.insert(property.bufferView);
    if (property.arrayOffsetBufferView >= 0) {
      result.insert(property.arrayOffsetBufferView);
    }
    if (property.stringOffsetBufferView >= 0) {
      result.insert(property.stringOffsetBufferView);
    }
  }

  return result;
}

TEST_CASE("Converts JSON B3DM batch table to EXT_feature_metadata") {
  std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testFilePath = testFilePath / "BatchTables" / "batchedWithJson.b3dm";

  GltfConverterResult result = ConvertTileToGltf::fromB3dm(testFilePath);

  REQUIRE(result.model);

  Model& gltf = *result.model;

  ExtensionModelExtFeatureMetadata* pExtension =
      gltf.getExtension<ExtensionModelExtFeatureMetadata>();
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

      ExtensionMeshPrimitiveExtFeatureMetadata* pPrimitiveExtension =
          primitive.getExtension<ExtensionMeshPrimitiveExtFeatureMetadata>();
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
        *result.model,
        featureTable,
        defaultClass,
        "id",
        "INT8",
        expected,
        expected.size());
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
        *result.model,
        featureTable,
        defaultClass,
        "Height",
        "FLOAT64",
        expected,
        expected.size());
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
        *result.model,
        featureTable,
        defaultClass,
        "Longitude",
        "FLOAT64",
        expected,
        expected.size());
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
        *result.model,
        featureTable,
        defaultClass,
        "Latitude",
        "FLOAT64",
        expected,
        expected.size());
  }
}

TEST_CASE("Convert binary B3DM batch table to EXT_feature_metadata") {
  std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testFilePath =
      testFilePath / "BatchTables" / "batchedWithBatchTableBinary.b3dm";

  GltfConverterResult result = ConvertTileToGltf::fromB3dm(testFilePath);

  REQUIRE(!result.errors);
  REQUIRE(result.model != std::nullopt);

  ExtensionModelExtFeatureMetadata* metadata =
      result.model->getExtension<ExtensionModelExtFeatureMetadata>();
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
        *result.model,
        featureTable,
        defaultClass,
        "id",
        "INT8",
        expected,
        expected.size());
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
        *result.model,
        featureTable,
        defaultClass,
        "Height",
        "FLOAT64",
        expected,
        expected.size());
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
        *result.model,
        featureTable,
        defaultClass,
        "Longitude",
        "FLOAT64",
        expected,
        expected.size());
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
        *result.model,
        featureTable,
        defaultClass,
        "Latitude",
        "FLOAT64",
        expected,
        expected.size());
  }

  {
    std::vector<uint8_t> expected(10, 255);
    checkScalarProperty<uint8_t>(
        *result.model,
        featureTable,
        defaultClass,
        "code",
        "UINT8",
        expected,
        expected.size());
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
        *result.model,
        featureTable,
        defaultClass,
        "cartographic",
        3,
        "FLOAT64",
        expected,
        expected.size());
  }
}

TEST_CASE("Converts batched PNTS batch table to EXT_feature_metadata") {
  std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testFilePath = testFilePath / "PointCloud" / "pointCloudBatched.pnts";

  GltfConverterResult result = ConvertTileToGltf::fromPnts(testFilePath);

  REQUIRE(result.model);
  Model& gltf = *result.model;

  ExtensionModelExtFeatureMetadata* pExtension =
      gltf.getExtension<ExtensionModelExtFeatureMetadata>();
  REQUIRE(pExtension);

  // Check the schema
  REQUIRE(pExtension->schema);
  REQUIRE(pExtension->schema->classes.size() == 1);

  auto firstClassIt = pExtension->schema->classes.begin();
  CHECK(firstClassIt->first == "default");

  CesiumGltf::Class& defaultClass = firstClassIt->second;
  REQUIRE(defaultClass.properties.size() == 3);

  {
    auto nameIt = defaultClass.properties.find("name");
    REQUIRE(nameIt != defaultClass.properties.end());
    auto dimensionsIt = defaultClass.properties.find("dimensions");
    REQUIRE(dimensionsIt != defaultClass.properties.end());
    auto idIt = defaultClass.properties.find("id");
    REQUIRE(idIt != defaultClass.properties.end());

    CHECK(nameIt->second.type == "STRING");
    CHECK(dimensionsIt->second.type == "ARRAY");
    REQUIRE(dimensionsIt->second.componentType);
    CHECK(dimensionsIt->second.componentType.value() == "FLOAT32");
    CHECK(idIt->second.type == "UINT32");
  }

  // Check the feature table
  auto firstFeatureTableIt = pExtension->featureTables.begin();
  REQUIRE(firstFeatureTableIt != pExtension->featureTables.end());

  FeatureTable& featureTable = firstFeatureTableIt->second;
  CHECK(featureTable.classProperty == "default");
  REQUIRE(featureTable.properties.size() == 3);

  {
    auto nameIt = featureTable.properties.find("name");
    REQUIRE(nameIt != featureTable.properties.end());
    auto dimensionsIt = featureTable.properties.find("dimensions");
    REQUIRE(dimensionsIt != featureTable.properties.end());
    auto idIt = featureTable.properties.find("id");
    REQUIRE(idIt != featureTable.properties.end());

    CHECK(nameIt->second.bufferView >= 0);
    CHECK(
        nameIt->second.bufferView <
        static_cast<int32_t>(gltf.bufferViews.size()));
    CHECK(dimensionsIt->second.bufferView >= 0);
    CHECK(
        dimensionsIt->second.bufferView <
        static_cast<int32_t>(gltf.bufferViews.size()));
    CHECK(idIt->second.bufferView >= 0);
    CHECK(
        idIt->second.bufferView <
        static_cast<int32_t>(gltf.bufferViews.size()));
  }

  std::set<int32_t> bufferViewSet =
      getUniqueBufferViewIds(gltf.accessors, featureTable);
  CHECK(bufferViewSet.size() == gltf.bufferViews.size());

  // Check the mesh primitive
  REQUIRE(gltf.meshes.size() == 1);
  Mesh& mesh = gltf.meshes[0];
  REQUIRE(mesh.primitives.size() == 1);

  MeshPrimitive& primitive = mesh.primitives[0];
  CHECK(
      primitive.attributes.find("_FEATURE_ID_0") != primitive.attributes.end());

  ExtensionMeshPrimitiveExtFeatureMetadata* pPrimitiveExtension =
      primitive.getExtension<ExtensionMeshPrimitiveExtFeatureMetadata>();
  REQUIRE(pPrimitiveExtension);
  REQUIRE(pPrimitiveExtension->featureIdAttributes.size() == 1);

  FeatureIDAttribute& attribute = pPrimitiveExtension->featureIdAttributes[0];
  CHECK(attribute.featureTable == "default");
  CHECK(attribute.featureIds.attribute == "_FEATURE_ID_0");

  // Check metadata values
  {
    std::vector<std::string> expected = {
        "section0",
        "section1",
        "section2",
        "section3",
        "section4",
        "section5",
        "section6",
        "section7"};
    checkScalarProperty<std::string, std::string_view>(
        *result.model,
        featureTable,
        defaultClass,
        "name",
        "STRING",
        expected,
        expected.size());
  }

  {
    std::vector<std::vector<float>> expected = {
        {0.1182744f, 0.7206326f, 0.6399210f},
        {0.5820198f, 0.1433532f, 0.5373732f},
        {0.9446688f, 0.7586156f, 0.5218483f},
        {0.1059076f, 0.4146619f, 0.4736004f},
        {0.2645556f, 0.1863323f, 0.7742336f},
        {0.7369181f, 0.4561503f, 0.2165503f},
        {0.5684339f, 0.1352181f, 0.0187897f},
        {0.3241409f, 0.6176354f, 0.1496748f}};

    checkArrayProperty<float>(
        *result.model,
        featureTable,
        defaultClass,
        "dimensions",
        3,
        "FLOAT32",
        expected,
        expected.size());
  }

  {
    std::vector<uint32_t> expected = {0, 1, 2, 3, 4, 5, 6, 7};
    checkScalarProperty<uint32_t>(
        *result.model,
        featureTable,
        defaultClass,
        "id",
        "UINT32",
        expected,
        expected.size());
  }
}

TEST_CASE("Converts per-point PNTS batch table to EXT_feature_metadata") {
  std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testFilePath =
      testFilePath / "PointCloud" / "pointCloudWithPerPointProperties.pnts";

  GltfConverterResult result = ConvertTileToGltf::fromPnts(testFilePath);

  REQUIRE(result.model);
  Model& gltf = *result.model;

  ExtensionModelExtFeatureMetadata* pExtension =
      gltf.getExtension<ExtensionModelExtFeatureMetadata>();
  REQUIRE(pExtension);

  // Check the schema
  REQUIRE(pExtension->schema);
  REQUIRE(pExtension->schema->classes.size() == 1);

  auto firstClassIt = pExtension->schema->classes.begin();
  CHECK(firstClassIt->first == "default");

  CesiumGltf::Class& defaultClass = firstClassIt->second;
  REQUIRE(defaultClass.properties.size() == 3);

  {
    auto temperatureIt = defaultClass.properties.find("temperature");
    REQUIRE(temperatureIt != defaultClass.properties.end());
    auto secondaryColorIt = defaultClass.properties.find("secondaryColor");
    REQUIRE(secondaryColorIt != defaultClass.properties.end());
    auto idIt = defaultClass.properties.find("id");
    REQUIRE(idIt != defaultClass.properties.end());

    CHECK(temperatureIt->second.type == "FLOAT32");
    CHECK(secondaryColorIt->second.type == "ARRAY");
    REQUIRE(secondaryColorIt->second.componentType);
    CHECK(secondaryColorIt->second.componentType.value() == "FLOAT32");
    CHECK(idIt->second.type == "UINT16");
  }

  // Check the feature table
  auto firstFeatureTableIt = pExtension->featureTables.begin();
  REQUIRE(firstFeatureTableIt != pExtension->featureTables.end());

  FeatureTable& featureTable = firstFeatureTableIt->second;
  CHECK(featureTable.classProperty == "default");
  REQUIRE(featureTable.properties.size() == 3);

  {
    auto temperatureIt = featureTable.properties.find("temperature");
    REQUIRE(temperatureIt != featureTable.properties.end());
    auto secondaryColorIt = featureTable.properties.find("secondaryColor");
    REQUIRE(secondaryColorIt != featureTable.properties.end());
    auto idIt = featureTable.properties.find("id");
    REQUIRE(idIt != featureTable.properties.end());

    CHECK(temperatureIt->second.bufferView >= 0);
    CHECK(
        temperatureIt->second.bufferView <
        static_cast<int32_t>(gltf.bufferViews.size()));
    CHECK(secondaryColorIt->second.bufferView >= 0);
    CHECK(
        secondaryColorIt->second.bufferView <
        static_cast<int32_t>(gltf.bufferViews.size()));
    CHECK(idIt->second.bufferView >= 0);
    CHECK(
        idIt->second.bufferView <
        static_cast<int32_t>(gltf.bufferViews.size()));
  }

  std::set<int32_t> bufferViewSet =
      getUniqueBufferViewIds(gltf.accessors, featureTable);
  CHECK(bufferViewSet.size() == gltf.bufferViews.size());

  // Check the mesh primitive
  REQUIRE(gltf.meshes.size() == 1);
  Mesh& mesh = gltf.meshes[0];
  REQUIRE(mesh.primitives.size() == 1);

  MeshPrimitive& primitive = mesh.primitives[0];
  CHECK(
      primitive.attributes.find("_FEATURE_ID_0") == primitive.attributes.end());

  ExtensionMeshPrimitiveExtFeatureMetadata* pPrimitiveExtension =
      primitive.getExtension<ExtensionMeshPrimitiveExtFeatureMetadata>();
  REQUIRE(pPrimitiveExtension);
  REQUIRE(pPrimitiveExtension->featureIdAttributes.size() == 1);

  FeatureIDAttribute& attribute = pPrimitiveExtension->featureIdAttributes[0];
  CHECK(attribute.featureTable == "default");
  // Check for implicit feature IDs
  CHECK(!attribute.featureIds.attribute);
  CHECK(attribute.featureIds.constant == 0);
  CHECK(attribute.featureIds.divisor == 1);

  // Check metadata values
  {
    std::vector<float> expected = {
        0.2883332f,
        0.4338732f,
        0.1750928f,
        0.1430827f,
        0.1156976f,
        0.3274261f,
        0.1337213f,
        0.0207673f};
    checkScalarProperty<float>(
        *result.model,
        featureTable,
        defaultClass,
        "temperature",
        "FLOAT32",
        expected,
        expected.size());
  }

  {
    std::vector<std::vector<float>> expected = {
        {0.0202183f, 0, 0},
        {0.3682415f, 0, 0},
        {0.8326198f, 0, 0},
        {0.9571551f, 0, 0},
        {0.7781567f, 0, 0},
        {0.1403507f, 0, 0},
        {0.8700121f, 0, 0},
        {0.8700872f, 0, 0}};

    checkArrayProperty<float>(
        *result.model,
        featureTable,
        defaultClass,
        "secondaryColor",
        3,
        "FLOAT32",
        expected,
        expected.size());
  }

  {
    std::vector<uint16_t> expected = {0, 1, 2, 3, 4, 5, 6, 7};
    checkScalarProperty<uint16_t>(
        *result.model,
        featureTable,
        defaultClass,
        "id",
        "UINT16",
        expected,
        expected.size());
  }
}

TEST_CASE("Converts Draco per-point PNTS batch table to "
          "EXT_feature_metadata") {
  std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testFilePath = testFilePath / "PointCloud" / "pointCloudDraco.pnts";

  GltfConverterResult result = ConvertTileToGltf::fromPnts(testFilePath);

  REQUIRE(result.model);
  Model& gltf = *result.model;

  ExtensionModelExtFeatureMetadata* pExtension =
      gltf.getExtension<ExtensionModelExtFeatureMetadata>();
  REQUIRE(pExtension);

  // Check the schema
  REQUIRE(pExtension->schema);
  REQUIRE(pExtension->schema->classes.size() == 1);

  auto firstClassIt = pExtension->schema->classes.begin();
  CHECK(firstClassIt->first == "default");

  CesiumGltf::Class& defaultClass = firstClassIt->second;
  REQUIRE(defaultClass.properties.size() == 3);

  {
    auto temperatureIt = defaultClass.properties.find("temperature");
    REQUIRE(temperatureIt != defaultClass.properties.end());
    auto secondaryColorIt = defaultClass.properties.find("secondaryColor");
    REQUIRE(secondaryColorIt != defaultClass.properties.end());
    auto idIt = defaultClass.properties.find("id");
    REQUIRE(idIt != defaultClass.properties.end());

    CHECK(temperatureIt->second.type == "FLOAT32");
    CHECK(secondaryColorIt->second.type == "ARRAY");
    REQUIRE(secondaryColorIt->second.componentType);
    CHECK(secondaryColorIt->second.componentType.value() == "FLOAT32");
    CHECK(idIt->second.type == "UINT16");
  }

  // Check the feature table
  auto firstFeatureTableIt = pExtension->featureTables.begin();
  REQUIRE(firstFeatureTableIt != pExtension->featureTables.end());

  FeatureTable& featureTable = firstFeatureTableIt->second;
  CHECK(featureTable.classProperty == "default");
  REQUIRE(featureTable.properties.size() == 3);

  {
    auto temperatureIt = featureTable.properties.find("temperature");
    REQUIRE(temperatureIt != featureTable.properties.end());
    auto secondaryColorIt = featureTable.properties.find("secondaryColor");
    REQUIRE(secondaryColorIt != featureTable.properties.end());
    auto idIt = featureTable.properties.find("id");
    REQUIRE(idIt != featureTable.properties.end());

    CHECK(temperatureIt->second.bufferView >= 0);
    CHECK(
        temperatureIt->second.bufferView <
        static_cast<int32_t>(gltf.bufferViews.size()));
    CHECK(secondaryColorIt->second.bufferView >= 0);
    CHECK(
        secondaryColorIt->second.bufferView <
        static_cast<int32_t>(gltf.bufferViews.size()));
    CHECK(idIt->second.bufferView >= 0);
    CHECK(
        idIt->second.bufferView <
        static_cast<int32_t>(gltf.bufferViews.size()));
  }

  std::set<int32_t> bufferViewSet =
      getUniqueBufferViewIds(gltf.accessors, featureTable);
  CHECK(bufferViewSet.size() == gltf.bufferViews.size());

  // Check the mesh primitive
  REQUIRE(gltf.meshes.size() == 1);
  Mesh& mesh = gltf.meshes[0];
  REQUIRE(mesh.primitives.size() == 1);

  MeshPrimitive& primitive = mesh.primitives[0];
  CHECK(
      primitive.attributes.find("_FEATURE_ID_0") == primitive.attributes.end());

  ExtensionMeshPrimitiveExtFeatureMetadata* pPrimitiveExtension =
      primitive.getExtension<ExtensionMeshPrimitiveExtFeatureMetadata>();
  REQUIRE(pPrimitiveExtension);
  REQUIRE(pPrimitiveExtension->featureIdAttributes.size() == 1);

  FeatureIDAttribute& attribute = pPrimitiveExtension->featureIdAttributes[0];
  CHECK(attribute.featureTable == "default");
  // Check for implicit feature IDs
  CHECK(!attribute.featureIds.attribute);
  CHECK(attribute.featureIds.constant == 0);
  CHECK(attribute.featureIds.divisor == 1);

  // Check metadata values
  {
    std::vector<float> expected = {
        0.2883025f,
        0.4338731f,
        0.1751145f,
        0.1430345f,
        0.1156959f,
        0.3274441f,
        0.1337535f,
        0.0207673f};
    checkScalarProperty<float>(
        *result.model,
        featureTable,
        defaultClass,
        "temperature",
        "FLOAT32",
        expected,
        expected.size());
  }

  {
    std::vector<std::vector<float>> expected = {
        {0.1182744f, 0, 0},
        {0.7206645f, 0, 0},
        {0.6399421f, 0, 0},
        {0.5820239f, 0, 0},
        {0.1432983f, 0, 0},
        {0.5374249f, 0, 0},
        {0.9446688f, 0, 0},
        {0.7586040f, 0, 0}};

    checkArrayProperty<float>(
        *result.model,
        featureTable,
        defaultClass,
        "secondaryColor",
        3,
        "FLOAT32",
        expected,
        expected.size());
  }

  {
    std::vector<uint16_t> expected = {0, 1, 2, 3, 4, 5, 6, 7};
    checkScalarProperty<uint16_t>(
        *result.model,
        featureTable,
        defaultClass,
        "id",
        "UINT16",
        expected,
        expected.size());
  }
}

TEST_CASE("Upgrade json nested json metadata to string") {
  std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testFilePath =
      testFilePath / "BatchTables" / "batchedWithStringAndNestedJson.b3dm";

  GltfConverterResult result = ConvertTileToGltf::fromB3dm(testFilePath);

  REQUIRE(!result.errors);
  REQUIRE(result.model != std::nullopt);

  ExtensionModelExtFeatureMetadata* metadata =
      result.model->getExtension<ExtensionModelExtFeatureMetadata>();
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
        *result.model,
        featureTable,
        defaultClass,
        "info",
        "STRING",
        expected,
        expected.size());
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
        *result.model,
        featureTable,
        defaultClass,
        "rooms",
        3,
        "STRING",
        expected,
        expected.size());
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

  auto errors = BatchTableToGltfFeatureMetadata::convertFromB3dm(
      featureTableJson,
      batchTableJson,
      gsl::span<const std::byte>(),
      model);

  ExtensionModelExtFeatureMetadata* metadata =
      model.getExtension<ExtensionModelExtFeatureMetadata>();
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
      expected,
      expected.size());
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
    createTestForArrayJson(expected, expectedComponentType, 4, expected.size());
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
    createTestForArrayJson(expected, expectedComponentType, 5, expected.size());
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
    createTestForArrayJson(expected, expectedComponentType, 4, expected.size());
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
    createTestForArrayJson(expected, expectedComponentType, 4, expected.size());
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
    createTestForArrayJson(expected, expectedComponentType, 4, expected.size());
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
    createTestForArrayJson(expected, expectedComponentType, 4, expected.size());
  }

  SECTION("int64_t") {
    // though the max positive number is only uint32_t. However, due to negative
    // number, it is upgraded to int64_t
    // clang-format off
    std::vector<std::vector<int64_t>> expected {
      {0, 1, 4, 1},
      {1244, -922, 1222, 54},
      {123, 10, 122, 334},
      {13, 45, 122, 94},
      {11, 22, 3, 3147483647}};
    // clang-format on

    std::string expectedComponentType = "INT64";
    createTestForArrayJson(expected, expectedComponentType, 4, expected.size());
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
    createTestForArrayJson(expected, expectedComponentType, 4, expected.size());
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
    createTestForArrayJson(expected, expectedComponentType, 4, expected.size());
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
        4,
        expected.size());
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

    createTestForArrayJson(expected, "BOOLEAN", 6, expected.size());
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
    createTestForArrayJson(expected, expectedComponentType, 0, expected.size());
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
    createTestForArrayJson(expected, expectedComponentType, 0, expected.size());
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
    createTestForArrayJson(expected, expectedComponentType, 0, expected.size());
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
    createTestForArrayJson(expected, expectedComponentType, 0, expected.size());
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
    createTestForArrayJson(expected, expectedComponentType, 0, expected.size());
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
    createTestForArrayJson(expected, expectedComponentType, 0, expected.size());
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
    createTestForArrayJson(expected, expectedComponentType, 0, expected.size());
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
    createTestForArrayJson(expected, expectedComponentType, 0, expected.size());
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
    createTestForArrayJson(expected, expectedComponentType, 0, expected.size());
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
        0,
        expected.size());
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

    createTestForArrayJson(expected, "BOOLEAN", 0, expected.size());
  }
}

TEST_CASE("Upgrade scalar json") {
  SECTION("Uint32") {
    std::vector<uint32_t> expected{32, 45, 21, 65, 78};
    createTestForScalarJson<uint32_t, int8_t>(
        expected,
        "INT8",
        expected.size());
  }

  SECTION("Boolean") {
    std::vector<bool> expected{true, false, true, false, true, true, false};
    createTestForScalarJson(expected, "BOOLEAN", expected.size());
  }

  SECTION("String") {
    std::vector<std::string> expected{"Test 0", "Test 1", "Test 2", "Test 3"};
    createTestForScalarJson<std::string, std::string_view>(
        expected,
        "STRING",
        expected.size());
  }
}

TEST_CASE("Cannot write pass batch length table") {
  SECTION("Numeric") {
    std::vector<uint32_t> expected{32, 45, 21, 65, 78, 20, 33, 12};
    createTestForScalarJson<uint32_t, int8_t>(expected, "INT8", 4);
  }

  SECTION("Boolean") {
    std::vector<bool> expected{true, false, true, false, true, true, false};
    createTestForScalarJson(expected, "BOOLEAN", 4);
  }

  SECTION("String") {
    std::vector<std::string>
        expected{"Test 0", "Test 1", "Test 2", "Test 3", "Test 4"};
    createTestForScalarJson<std::string, std::string_view>(
        expected,
        "STRING",
        3);
  }

  SECTION("Fixed number array") {
    // clang-format off
    std::vector<std::vector<uint64_t>> expected {
      {0, 1, 4, 1},
      {1244, 13223302036854775807u, 1222, 544662},
      {123, 10, 122, 334},
      {13, 45, 122, 94},
      {11, 22, 3, 13223302036854775807u}};
    // clang-format on

    std::string expectedComponentType = "UINT64";
    createTestForArrayJson(expected, expectedComponentType, 4, 2);
  }

  SECTION("Fixed boolean array") {
    // clang-format off
    std::vector<std::vector<bool>> expected{
      {true, true, false},
      {true, false, true},
      {false, true, true},
      {false, true, true},
    };
    // clang-format on

    createTestForArrayJson(expected, "BOOLEAN", 3, 2);
  }

  SECTION("Fixed string array") {
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
        4,
        2);
  }

  SECTION("Dynamic number array") {
    // clang-format off
    std::vector<std::vector<int32_t>> expected {
      {0, 1},
      {1244, -500000, 1222, 544662},
      {123, -10},
      {13},
      {11, 22, 3, 2147483647, 12233}};
    // clang-format on

    std::string expectedComponentType = "INT32";
    createTestForArrayJson(expected, expectedComponentType, 0, 3);
  }

  SECTION("Dynamic boolean array") {
    // clang-format off
    std::vector<std::vector<bool>> expected{
      {true, true, false, true, false, false, true},
      {true, false},
      {false, true, true, false},
      {false, true, true},
      {true, true, false, false}
    };
    // clang-format on

    createTestForArrayJson(expected, "BOOLEAN", 0, 2);
  }

  SECTION("Dynamic string array") {
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
        0,
        2);
  }
}

TEST_CASE("Converts Feature Classes 3DTILES_batch_table_hierarchy example to "
          "EXT_feature_metadata") {
  Model gltf;

  std::string featureTableJson = R"(
    {
      "BATCH_LENGTH": 8
    }
  )";

  // "Feature classes" example from the spec:
  // https://github.com/CesiumGS/3d-tiles/tree/main/extensions/3DTILES_batch_table_hierarchy#feature-classes
  std::string batchTableJson = R"(
    {
      "extensions" : {
        "3DTILES_batch_table_hierarchy" : {
          "classes" : [
            {
              "name" : "Lamp",
              "length" : 3,
              "instances" : {
                "lampStrength" : [10, 5, 7],
                "lampColor" : ["yellow", "white", "white"]
              }
            },
            {
              "name" : "Car",
              "length" : 3,
              "instances" : {
                "carType" : ["truck", "bus", "sedan"],
                "carColor" : ["green", "blue", "red"]
              }
            },
            {
              "name" : "Tree",
              "length" : 2,
              "instances" : {
                "treeHeight" : [10, 15],
                "treeAge" : [5, 8]
              }
            }
          ],
          "instancesLength" : 8,
          "classIds" : [0, 0, 0, 1, 1, 1, 2, 2]
        }
      }
    }
  )";

  rapidjson::Document featureTableParsed;
  featureTableParsed.Parse(featureTableJson.data(), featureTableJson.size());

  rapidjson::Document batchTableParsed;
  batchTableParsed.Parse(batchTableJson.data(), batchTableJson.size());

  auto errors = BatchTableToGltfFeatureMetadata::convertFromB3dm(
      featureTableParsed,
      batchTableParsed,
      gsl::span<const std::byte>(),
      gltf);

  ExtensionModelExtFeatureMetadata* pExtension =
      gltf.getExtension<ExtensionModelExtFeatureMetadata>();
  REQUIRE(pExtension);

  // Check the schema
  REQUIRE(pExtension->schema);
  REQUIRE(pExtension->schema->classes.size() == 1);

  auto firstClassIt = pExtension->schema->classes.begin();
  CHECK(firstClassIt->first == "default");

  CesiumGltf::Class& defaultClass = firstClassIt->second;
  REQUIRE(defaultClass.properties.size() == 6);

  // Check the feature table
  auto firstFeatureTableIt = pExtension->featureTables.begin();
  REQUIRE(firstFeatureTableIt != pExtension->featureTables.end());

  FeatureTable& featureTable = firstFeatureTableIt->second;
  CHECK(featureTable.classProperty == "default");
  REQUIRE(featureTable.properties.size() == 6);

  // Even though some of these properties are numeric, they become STRING
  // because not every feature has every property, and only STRING can
  // represent null.
  struct Expected {
    std::string name;
    std::string type;
    std::vector<std::string> values;
  };

  std::vector<Expected> expectedProperties{
      {"lampStrength",
       "STRING",
       {"10", "5", "7", "null", "null", "null", "null", "null"}},
      {"lampColor",
       "STRING",
       {"yellow", "white", "white", "null", "null", "null", "null", "null"}},
      {"carType",
       "STRING",
       {"null", "null", "null", "truck", "bus", "sedan", "null", "null"}},
      {"carColor",
       "STRING",
       {"null", "null", "null", "green", "blue", "red", "null", "null"}},
      {"treeHeight",
       "STRING",
       {"null", "null", "null", "null", "null", "null", "10", "15"}},
      {"treeAge",
       "STRING",
       {"null", "null", "null", "null", "null", "null", "5", "8"}}};

  for (const auto& expected : expectedProperties) {
    auto it = defaultClass.properties.find(expected.name);
    REQUIRE(it != defaultClass.properties.end());
    CHECK(it->second.type == expected.type);

    checkScalarProperty<std::string, std::string_view>(
        gltf,
        featureTable,
        defaultClass,
        expected.name,
        expected.type,
        expected.values,
        expected.values.size());
  }
}

TEST_CASE("Converts Feature Hierarchy 3DTILES_batch_table_hierarchy example to "
          "EXT_feature_metadata") {
  Model gltf;

  std::string featureTableJson = R"(
    {
      "BATCH_LENGTH": 6
    }
  )";

  // "Feature hierarchy" example from the spec:
  // https://github.com/CesiumGS/3d-tiles/tree/main/extensions/3DTILES_batch_table_hierarchy#feature-hierarchy
  std::string batchTableJson = R"(
    {
      "extensions" : {
        "3DTILES_batch_table_hierarchy" : {
          "classes" : [
            {
              "name" : "Wall",
              "length" : 6,
              "instances" : {
                "wall_color" : ["blue", "pink", "green", "lime", "black", "brown"],
                "wall_windows" : [2, 4, 4, 2, 0, 3]
              }
            },
            {
              "name" : "Building",
              "length" : 3,
              "instances" : {
                "building_name" : ["building_0", "building_1", "building_2"],
                "building_id" : [0, 1, 2],
                "building_address" : ["10 Main St", "12 Main St", "14 Main St"]
              }
            },
            {
              "name" : "Block",
              "length" : 1,
              "instances" : {
                "block_lat_long" : [[0.12, 0.543]],
                "block_district" : ["central"]
              }
            }
          ],
          "instancesLength" : 10,
          "classIds" : [0, 0, 0, 0, 0, 0, 1, 1, 1, 2],
          "parentIds" : [6, 6, 7, 7, 8, 8, 9, 9, 9, 9]
        }
      }
    }
  )";

  rapidjson::Document featureTableParsed;
  featureTableParsed.Parse(featureTableJson.data(), featureTableJson.size());

  rapidjson::Document batchTableParsed;
  batchTableParsed.Parse(batchTableJson.data(), batchTableJson.size());

  BatchTableToGltfFeatureMetadata::convertFromB3dm(
      featureTableParsed,
      batchTableParsed,
      gsl::span<const std::byte>(),
      gltf);

  ExtensionModelExtFeatureMetadata* pExtension =
      gltf.getExtension<ExtensionModelExtFeatureMetadata>();
  REQUIRE(pExtension);

  // Check the schema
  REQUIRE(pExtension->schema);
  REQUIRE(pExtension->schema->classes.size() == 1);

  auto firstClassIt = pExtension->schema->classes.begin();
  CHECK(firstClassIt->first == "default");

  CesiumGltf::Class& defaultClass = firstClassIt->second;
  REQUIRE(defaultClass.properties.size() == 7);

  // Check the feature table
  auto firstFeatureTableIt = pExtension->featureTables.begin();
  REQUIRE(firstFeatureTableIt != pExtension->featureTables.end());

  FeatureTable& featureTable = firstFeatureTableIt->second;
  CHECK(featureTable.classProperty == "default");
  REQUIRE(featureTable.properties.size() == 7);

  struct ExpectedString {
    std::string name;
    std::string type;
    std::vector<std::string> values;
  };

  std::vector<ExpectedString> expectedStringProperties{
      {"wall_color",
       "STRING",
       {"blue", "pink", "green", "lime", "black", "brown"}},
      {"building_name",
       "STRING",
       {"building_0",
        "building_0",
        "building_1",
        "building_1",
        "building_2",
        "building_2"}},
      {"building_address",
       "STRING",
       {"10 Main St",
        "10 Main St",
        "12 Main St",
        "12 Main St",
        "14 Main St",
        "14 Main St"}},
      {"block_district",
       "STRING",
       {"central", "central", "central", "central", "central", "central"}}};

  for (const auto& expected : expectedStringProperties) {
    auto it = defaultClass.properties.find(expected.name);
    REQUIRE(it != defaultClass.properties.end());
    CHECK(it->second.type == expected.type);

    checkScalarProperty<std::string, std::string_view>(
        gltf,
        featureTable,
        defaultClass,
        expected.name,
        expected.type,
        expected.values,
        expected.values.size());
  }

  struct ExpectedInt8Properties {
    std::string name;
    std::string type;
    std::vector<int8_t> values;
  };

  std::vector<ExpectedInt8Properties> expectedInt8Properties{
      {"wall_windows", "INT8", {2, 4, 4, 2, 0, 3}},
      {"building_id", "INT8", {0, 0, 1, 1, 2, 2}},
  };

  for (const auto& expected : expectedInt8Properties) {
    auto it = defaultClass.properties.find(expected.name);
    REQUIRE(it != defaultClass.properties.end());
    CHECK(it->second.type == expected.type);

    checkScalarProperty<int8_t>(
        gltf,
        featureTable,
        defaultClass,
        expected.name,
        expected.type,
        expected.values,
        expected.values.size());
  }

  struct ExpectedDoubleArrayProperties {
    std::string name;
    std::string type;
    std::string componentType;
    int64_t componentCount;
    std::vector<std::vector<double>> values;
  };

  std::vector<ExpectedDoubleArrayProperties> expectedDoubleArrayProperties{
      {"block_lat_long",
       "ARRAY",
       "FLOAT64",
       2,
       {{0.12, 0.543},
        {0.12, 0.543},
        {0.12, 0.543},
        {0.12, 0.543},
        {0.12, 0.543},
        {0.12, 0.543}}}};

  for (const auto& expected : expectedDoubleArrayProperties) {
    auto it = defaultClass.properties.find(expected.name);
    REQUIRE(it != defaultClass.properties.end());
    CHECK(it->second.type == expected.type);
    CHECK(it->second.componentType == expected.componentType);

    checkArrayProperty<double>(
        gltf,
        featureTable,
        defaultClass,
        expected.name,
        expected.componentCount,
        expected.componentType,
        expected.values,
        expected.values.size());
  }
}

TEST_CASE(
    "3DTILES_batch_table_hierarchy with parentCounts is ok if all are 1") {
  Model gltf;

  std::string featureTableJson = R"(
    {
      "BATCH_LENGTH": 3
    }
  )";

  // "Feature hierarchy" example from the spec:
  // https://github.com/CesiumGS/3d-tiles/tree/main/extensions/3DTILES_batch_table_hierarchy#feature-hierarchy
  std::string batchTableJson = R"(
    {
      "extensions" : {
        "3DTILES_batch_table_hierarchy" : {
          "classes" : [
            {
              "name" : "Parent1",
              "length" : 3,
              "instances" : {
                "some_property" : ["a", "b", "c"]
              }
            },
            {
              "name" : "Parent2",
              "length" : 3,
              "instances" : {
                "another_property" : ["d", "e", "f"]
              }
            },
            {
              "name" : "Main",
              "length" : 3,
              "instances" : {
                "third" : [1, 2, 3]
              }
            }
          ],
          "instancesLength" : 5,
          "classIds" : [2, 2, 2, 0, 1],
          "parentCounts": [1, 1, 1, 1, 1],
          "parentIds" : [3, 3, 3, 4, 4]
        }
      }
    }
  )";

  rapidjson::Document featureTableParsed;
  featureTableParsed.Parse(featureTableJson.data(), featureTableJson.size());

  rapidjson::Document batchTableParsed;
  batchTableParsed.Parse(batchTableJson.data(), batchTableJson.size());

  auto pLog = std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(3);
  spdlog::default_logger()->sinks().emplace_back(pLog);

  BatchTableToGltfFeatureMetadata::convertFromB3dm(
      featureTableParsed,
      batchTableParsed,
      gsl::span<const std::byte>(),
      gltf);

  // There should not be any log messages about parentCounts, since they're
  // all 1.
  std::vector<std::string> logMessages = pLog->last_formatted();
  REQUIRE(logMessages.size() == 0);

  // There should actually be metadata properties as normal.
  ExtensionModelExtFeatureMetadata* pExtension =
      gltf.getExtension<ExtensionModelExtFeatureMetadata>();
  REQUIRE(pExtension);

  // Check the schema
  REQUIRE(pExtension->schema);
  REQUIRE(pExtension->schema->classes.size() == 1);

  auto firstClassIt = pExtension->schema->classes.begin();
  CHECK(firstClassIt->first == "default");

  CesiumGltf::Class& defaultClass = firstClassIt->second;
  REQUIRE(defaultClass.properties.size() == 3);

  // Check the feature table
  auto firstFeatureTableIt = pExtension->featureTables.begin();
  REQUIRE(firstFeatureTableIt != pExtension->featureTables.end());

  FeatureTable& featureTable = firstFeatureTableIt->second;
  CHECK(featureTable.classProperty == "default");
  REQUIRE(featureTable.properties.size() == 3);
}

TEST_CASE(
    "3DTILES_batch_table_hierarchy with parentCounts != 1 is not supported") {
  Model gltf;

  std::string featureTableJson = R"(
    {
      "BATCH_LENGTH": 3
    }
  )";

  // "Feature hierarchy" example from the spec:
  // https://github.com/CesiumGS/3d-tiles/tree/main/extensions/3DTILES_batch_table_hierarchy#feature-hierarchy
  std::string batchTableJson = R"(
    {
      "extensions" : {
        "3DTILES_batch_table_hierarchy" : {
          "classes" : [
            {
              "name" : "Parent1",
              "length" : 3,
              "instances" : {
                "some_property" : ["a", "b", "c"]
              }
            },
            {
              "name" : "Parent2",
              "length" : 3,
              "instances" : {
                "another_property" : ["d", "e", "f"]
              }
            },
            {
              "name" : "Main",
              "length" : 3,
              "instances" : {
                "third" : [1, 2, 3]
              }
            }
          ],
          "instancesLength" : 5,
          "classIds" : [2, 2, 2, 0, 1],
          "parentCounts": [2, 2, 2, 1, 1],
          "parentIds" : [3, 4, 3, 4, 3, 4, 3, 4]
        }
      }
    }
  )";

  rapidjson::Document featureTableParsed;
  featureTableParsed.Parse(featureTableJson.data(), featureTableJson.size());

  rapidjson::Document batchTableParsed;
  batchTableParsed.Parse(batchTableJson.data(), batchTableJson.size());

  auto pLog = std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(3);
  spdlog::default_logger()->sinks().emplace_back(pLog);

  auto errors = BatchTableToGltfFeatureMetadata::convertFromB3dm(
      featureTableParsed,
      batchTableParsed,
      gsl::span<const std::byte>(),
      gltf);

  // There should be a log message about parentCounts, and no properties.
  const std::vector<std::string>& logMessages = errors.warnings;
  REQUIRE(logMessages.size() == 1);
  CHECK(logMessages[0].find("parentCounts") != std::string::npos);

  ExtensionModelExtFeatureMetadata* pExtension =
      gltf.getExtension<ExtensionModelExtFeatureMetadata>();
  REQUIRE(pExtension);

  // Check the schema
  REQUIRE(pExtension->schema);
  REQUIRE(pExtension->schema->classes.size() == 1);

  auto firstClassIt = pExtension->schema->classes.begin();
  CHECK(firstClassIt->first == "default");

  CesiumGltf::Class& defaultClass = firstClassIt->second;
  REQUIRE(defaultClass.properties.size() == 0);

  // Check the feature table
  auto firstFeatureTableIt = pExtension->featureTables.begin();
  REQUIRE(firstFeatureTableIt != pExtension->featureTables.end());

  FeatureTable& featureTable = firstFeatureTableIt->second;
  CHECK(featureTable.classProperty == "default");
  REQUIRE(featureTable.properties.size() == 0);
}
