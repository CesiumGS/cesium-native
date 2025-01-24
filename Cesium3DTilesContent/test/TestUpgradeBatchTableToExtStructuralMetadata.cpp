#include "BatchTableToGltfStructuralMetadata.h"
#include "ConvertTileToGltf.h"

#include <Cesium3DTilesContent/GltfConverterResult.h>
#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/Class.h>
#include <CesiumGltf/ClassProperty.h>
#include <CesiumGltf/ExtensionExtMeshFeatures.h>
#include <CesiumGltf/ExtensionKhrDracoMeshCompression.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltf/FeatureId.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/PropertyArrayView.h>
#include <CesiumGltf/PropertyTable.h>
#include <CesiumGltf/PropertyTablePropertyView.h>
#include <CesiumGltf/PropertyTableView.h>
#include <CesiumGltf/Schema.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_float3.hpp>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <spdlog/sinks/ringbuffer_sink.h>
#include <spdlog/spdlog.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace doctest;
using namespace CesiumGltf;
using namespace Cesium3DTilesContent;
using namespace CesiumUtility;

template <typename ExpectedType, typename PropertyViewType = ExpectedType>
static void checkNonArrayProperty(
    const Model& model,
    const PropertyTable& propertyTable,
    const Class& metaClass,
    const std::string& propertyName,
    const std::string& expectedType,
    const std::optional<std::string>& expectedComponentType,
    const std::vector<ExpectedType>& expected,
    size_t expectedTotalInstances,
    const std::optional<PropertyViewType>& noDataValue = std::nullopt) {
  const ClassProperty& property = metaClass.properties.at(propertyName);
  REQUIRE(property.type == expectedType);
  REQUIRE(property.componentType == expectedComponentType);
  REQUIRE(!property.array);
  REQUIRE(!property.count);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  PropertyTablePropertyView<PropertyViewType> propertyView =
      view.getPropertyView<PropertyViewType>(propertyName);
  REQUIRE(propertyView.status() == PropertyTablePropertyViewStatus::Valid);
  REQUIRE(propertyView.size() == propertyTable.count);
  REQUIRE(propertyView.size() == static_cast<int64_t>(expectedTotalInstances));
  for (int64_t i = 0; i < propertyView.size(); ++i) {
    if constexpr (std::is_same_v<PropertyViewType, glm::vec3>) {
      REQUIRE(Math::equalsEpsilon(
          static_cast<glm::dvec3>(propertyView.getRaw(i)),
          static_cast<glm::dvec3>(expected[static_cast<size_t>(i)]),
          Math::Epsilon6));
    } else if constexpr (
        std::is_same_v<PropertyViewType, float> ||
        std::is_same_v<PropertyViewType, double>) {
      REQUIRE(
          propertyView.getRaw(i) == Approx(expected[static_cast<size_t>(i)]));
    } else {
      REQUIRE(
          static_cast<ExpectedType>(propertyView.getRaw(i)) ==
          expected[static_cast<size_t>(i)]);
    }

    if (noDataValue && propertyView.getRaw(i) == noDataValue) {
      REQUIRE(!propertyView.get(i));
    } else {
      REQUIRE(propertyView.get(i) == propertyView.getRaw(i));
    }
  }
}

template <typename ExpectedType, typename PropertyViewType = ExpectedType>
static void checkArrayProperty(
    const Model& model,
    const PropertyTable& propertyTable,
    const Class& metaClass,
    const std::string& propertyName,
    int64_t expectedCount,
    const std::string& expectedType,
    const std::optional<std::string>& expectedComponentType,
    const std::vector<std::vector<ExpectedType>>& expected,
    size_t expectedTotalInstances) {
  const ClassProperty& property = metaClass.properties.at(propertyName);
  REQUIRE(property.type == expectedType);
  REQUIRE(property.componentType == expectedComponentType);
  REQUIRE(property.array);
  REQUIRE(property.count.value_or(0) == expectedCount);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  PropertyTablePropertyView<PropertyArrayView<PropertyViewType>> propertyView =
      view.getPropertyView<PropertyArrayView<PropertyViewType>>(propertyName);
  REQUIRE(propertyView.status() == PropertyTablePropertyViewStatus::Valid);
  REQUIRE(propertyView.size() == propertyTable.count);
  REQUIRE(propertyView.size() == static_cast<int64_t>(expectedTotalInstances));
  for (size_t i = 0; i < expectedTotalInstances; ++i) {
    PropertyArrayView<PropertyViewType> value =
        propertyView.getRaw(static_cast<int64_t>(i));
    if (expectedCount > 0) {
      REQUIRE(value.size() == expectedCount);
    }

    for (size_t j = 0; j < expected[i].size(); ++j) {
      if constexpr (
          std::is_same_v<ExpectedType, float> ||
          std::is_same_v<ExpectedType, double>) {
        REQUIRE(value[static_cast<int64_t>(j)] == Approx(expected[i][j]));
      } else {
        REQUIRE(value[static_cast<int64_t>(j)] == expected[i][j]);
      }
    }
  }
}

template <typename ExpectedType, typename PropertyViewType = ExpectedType>
static void createTestForNonArrayJson(
    const std::vector<ExpectedType>& expected,
    const std::string& expectedType,
    const std::optional<std::string>& expectedComponentType,
    size_t totalInstances,
    const std::optional<PropertyViewType> expectedNoData) {
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
    if (static_cast<PropertyViewType>(expected[i]) == expectedNoData) {
      rapidjson::Value nullValue;
      nullValue.SetNull();
      scalarProperty.PushBack(nullValue, batchTableJson.GetAllocator());
      continue;
    }

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
      "scalarProperty",
      scalarProperty,
      batchTableJson.GetAllocator());

  auto errors = BatchTableToGltfStructuralMetadata::convertFromB3dm(
      featureTableJson,
      batchTableJson,
      std::span<const std::byte>(),
      model);

  const ExtensionModelExtStructuralMetadata* pMetadata =
      model.getExtension<ExtensionModelExtStructuralMetadata>();
  REQUIRE(pMetadata);

  const CesiumUtility::IntrusivePointer<Schema> schema = pMetadata->schema;
  REQUIRE(schema);

  const std::unordered_map<std::string, Class>& classes = schema->classes;
  REQUIRE(classes.size() == 1);

  const Class& defaultClass = classes.at("default");
  const std::unordered_map<std::string, ClassProperty>& properties =
      defaultClass.properties;
  REQUIRE(properties.size() == 1);

  REQUIRE(pMetadata->propertyTables.size() == 1);

  const PropertyTable& propertyTable = pMetadata->propertyTables[0];
  checkNonArrayProperty<ExpectedType, PropertyViewType>(
      model,
      propertyTable,
      defaultClass,
      "scalarProperty",
      expectedType,
      expectedComponentType,
      expected,
      totalInstances,
      expectedNoData);
}

template <typename ExpectedType, typename PropertyViewType = ExpectedType>
static void createTestForNonArrayJson(
    const std::vector<ExpectedType>& expected,
    const std::string& expectedType,
    const std::optional<std::string>& expectedComponentType,
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
      "scalarProperty",
      scalarProperty,
      batchTableJson.GetAllocator());

  auto errors = BatchTableToGltfStructuralMetadata::convertFromB3dm(
      featureTableJson,
      batchTableJson,
      std::span<const std::byte>(),
      model);

  const ExtensionModelExtStructuralMetadata* pMetadata =
      model.getExtension<ExtensionModelExtStructuralMetadata>();
  REQUIRE(pMetadata);

  const CesiumUtility::IntrusivePointer<Schema> schema = pMetadata->schema;
  REQUIRE(schema);

  const std::unordered_map<std::string, Class>& classes = schema->classes;
  REQUIRE(classes.size() == 1);

  const Class& defaultClass = classes.at("default");
  const std::unordered_map<std::string, ClassProperty>& properties =
      defaultClass.properties;
  REQUIRE(properties.size() == 1);

  REQUIRE(pMetadata->propertyTables.size() == 1);

  const PropertyTable& propertyTable = pMetadata->propertyTables[0];
  checkNonArrayProperty<ExpectedType, PropertyViewType>(
      model,
      propertyTable,
      defaultClass,
      "scalarProperty",
      expectedType,
      expectedComponentType,
      expected,
      totalInstances);
}

template <typename ExpectedType, typename PropertyViewType = ExpectedType>
static void createTestForArrayJson(
    const std::vector<std::vector<ExpectedType>>& expected,
    const std::string& expectedType,
    const std::optional<std::string>& expectedComponentType,
    int64_t arrayCount,
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
      "fixedLengthArrayProperty",
      fixedArrayProperties,
      batchTableJson.GetAllocator());

  auto errors = BatchTableToGltfStructuralMetadata::convertFromB3dm(
      featureTableJson,
      batchTableJson,
      std::span<const std::byte>(),
      model);

  const ExtensionModelExtStructuralMetadata* pMetadata =
      model.getExtension<ExtensionModelExtStructuralMetadata>();
  REQUIRE(pMetadata);

  const CesiumUtility::IntrusivePointer<Schema>& schema = pMetadata->schema;
  REQUIRE(schema);
  REQUIRE(schema->classes.find("default") != schema->classes.end());

  const Class& defaultClass = schema->classes.at("default");
  REQUIRE(defaultClass.properties.size() == 1);

  REQUIRE(pMetadata->propertyTables.size() == 1);
  const PropertyTable& propertyTable = pMetadata->propertyTables[0];

  checkArrayProperty<ExpectedType, PropertyViewType>(
      model,
      propertyTable,
      defaultClass,
      "fixedLengthArrayProperty",
      arrayCount,
      expectedType,
      expectedComponentType,
      expected,
      totalInstances);
}

std::set<int32_t> getUniqueBufferViewIds(
    const std::vector<Accessor>& accessors,
    const PropertyTable& propertyTable) {
  std::set<int32_t> result;
  for (auto it = accessors.begin(); it != accessors.end(); it++) {
    result.insert(it->bufferView);
  }

  auto& properties = propertyTable.properties;
  for (auto it = properties.begin(); it != properties.end(); it++) {
    auto& property = it->second;
    result.insert(property.values);
    if (property.arrayOffsets >= 0) {
      result.insert(property.arrayOffsets);
    }
    if (property.stringOffsets >= 0) {
      result.insert(property.stringOffsets);
    }
  }

  return result;
}

TEST_CASE("Converts JSON B3DM batch table to EXT_structural_metadata") {
  std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testFilePath = testFilePath / "BatchTables" / "batchedWithJson.b3dm";

  GltfConverterResult result = ConvertTileToGltf::fromB3dm(testFilePath);

  REQUIRE(result.model);

  Model& gltf = *result.model;

  ExtensionModelExtStructuralMetadata* pExtension =
      gltf.getExtension<ExtensionModelExtStructuralMetadata>();
  REQUIRE(pExtension);
  CHECK(
      gltf.isExtensionUsed(ExtensionModelExtStructuralMetadata::ExtensionName));

  // Check the schema
  REQUIRE(pExtension->schema);
  REQUIRE(pExtension->schema->classes.size() == 1);

  auto firstClassIt = pExtension->schema->classes.begin();
  CHECK(firstClassIt->first == "default");

  Class& defaultClass = firstClassIt->second;
  REQUIRE(defaultClass.properties.size() == 4);

  auto idIt = defaultClass.properties.find("id");
  REQUIRE(idIt != defaultClass.properties.end());
  auto longitudeIt = defaultClass.properties.find("Longitude");
  REQUIRE(longitudeIt != defaultClass.properties.end());
  auto latitudeIt = defaultClass.properties.find("Latitude");
  REQUIRE(latitudeIt != defaultClass.properties.end());
  auto heightIt = defaultClass.properties.find("Height");
  REQUIRE(heightIt != defaultClass.properties.end());

  CHECK(idIt->second.type == ClassProperty::Type::SCALAR);
  CHECK(longitudeIt->second.type == ClassProperty::Type::SCALAR);
  CHECK(latitudeIt->second.type == ClassProperty::Type::SCALAR);
  CHECK(heightIt->second.type == ClassProperty::Type::SCALAR);

  CHECK(idIt->second.componentType == ClassProperty::ComponentType::INT8);
  CHECK(
      longitudeIt->second.componentType ==
      ClassProperty::ComponentType::FLOAT64);
  CHECK(
      latitudeIt->second.componentType ==
      ClassProperty::ComponentType::FLOAT64);
  CHECK(
      heightIt->second.componentType == ClassProperty::ComponentType::FLOAT64);

  // Check the property table
  REQUIRE(pExtension->propertyTables.size() == 1);
  PropertyTable& propertyTable = pExtension->propertyTables[0];
  CHECK(propertyTable.classProperty == "default");
  REQUIRE(propertyTable.properties.size() == 4);

  auto idIt2 = propertyTable.properties.find("id");
  REQUIRE(idIt2 != propertyTable.properties.end());
  auto longitudeIt2 = propertyTable.properties.find("Longitude");
  REQUIRE(longitudeIt2 != propertyTable.properties.end());
  auto latitudeIt2 = propertyTable.properties.find("Latitude");
  REQUIRE(latitudeIt2 != propertyTable.properties.end());
  auto heightIt2 = propertyTable.properties.find("Height");
  REQUIRE(heightIt2 != propertyTable.properties.end());

  REQUIRE(idIt2->second.values >= 0);
  REQUIRE(idIt2->second.values < static_cast<int32_t>(gltf.bufferViews.size()));
  REQUIRE(longitudeIt2->second.values >= 0);
  REQUIRE(
      longitudeIt2->second.values <
      static_cast<int32_t>(gltf.bufferViews.size()));
  REQUIRE(latitudeIt2->second.values >= 0);
  REQUIRE(
      latitudeIt2->second.values <
      static_cast<int32_t>(gltf.bufferViews.size()));
  REQUIRE(heightIt2->second.values >= 0);
  REQUIRE(
      heightIt2->second.values < static_cast<int32_t>(gltf.bufferViews.size()));

  // Make sure all property bufferViews are unique
  std::set<int32_t> bufferViews{
      idIt2->second.values,
      longitudeIt2->second.values,
      latitudeIt2->second.values,
      heightIt2->second.values};
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
      CHECK(
          primitive.attributes.find("_BATCH_ID") == primitive.attributes.end());

      ExtensionExtMeshFeatures* pPrimitiveExtension =
          primitive.getExtension<ExtensionExtMeshFeatures>();
      REQUIRE(pPrimitiveExtension);
      CHECK(gltf.isExtensionUsed(ExtensionExtMeshFeatures::ExtensionName));
      REQUIRE(pPrimitiveExtension->featureIds.size() == 1);
      const FeatureId& featureId = pPrimitiveExtension->featureIds[0];
      CHECK(featureId.featureCount == 10);
      CHECK(featureId.attribute == 0);
      CHECK(featureId.propertyTable == 0);
    }
  }

  // Check metadata values
  {
    std::vector<int8_t> expected = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    checkNonArrayProperty<int8_t>(
        gltf,
        propertyTable,
        defaultClass,
        "id",
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::INT8,
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
    checkNonArrayProperty<double>(
        *result.model,
        propertyTable,
        defaultClass,
        "Height",
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::FLOAT64,
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
    checkNonArrayProperty<double>(
        gltf,
        propertyTable,
        defaultClass,
        "Longitude",
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::FLOAT64,
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
    checkNonArrayProperty<double>(
        gltf,
        propertyTable,
        defaultClass,
        "Latitude",
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::FLOAT64,
        expected,
        expected.size());
  }
}

TEST_CASE("Convert binary B3DM batch table to EXT_structural_metadata") {
  std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testFilePath =
      testFilePath / "BatchTables" / "batchedWithBatchTableBinary.b3dm";

  GltfConverterResult result = ConvertTileToGltf::fromB3dm(testFilePath);

  REQUIRE(!result.errors);
  REQUIRE(result.model);

  const Model& model = *result.model;

  const ExtensionModelExtStructuralMetadata* metadata =
      model.getExtension<ExtensionModelExtStructuralMetadata>();
  REQUIRE(metadata);

  CesiumUtility::IntrusivePointer<Schema> schema = metadata->schema;
  REQUIRE(schema);

  const std::unordered_map<std::string, Class>& classes = schema->classes;
  REQUIRE(classes.size() == 1);

  const Class& defaultClass = classes.at("default");
  const std::unordered_map<std::string, ClassProperty>& properties =
      defaultClass.properties;
  REQUIRE(properties.size() == 6);

  REQUIRE(metadata->propertyTables.size() == 1);
  const PropertyTable& propertyTable = metadata->propertyTables[0];

  // Check that batch IDs were converted to EXT_mesh_features
  CHECK(!model.meshes.empty());

  for (const Mesh& mesh : model.meshes) {
    CHECK(!mesh.primitives.empty());
    for (const MeshPrimitive& primitive : mesh.primitives) {
      CHECK(
          primitive.attributes.find("_FEATURE_ID_0") !=
          primitive.attributes.end());
      CHECK(
          primitive.attributes.find("_FEATURE_ID_1") ==
          primitive.attributes.end());
      CHECK(
          primitive.attributes.find("_BATCH_ID") == primitive.attributes.end());

      const ExtensionExtMeshFeatures* pPrimitiveExtension =
          primitive.getExtension<ExtensionExtMeshFeatures>();
      REQUIRE(pPrimitiveExtension);
      CHECK(model.isExtensionUsed(ExtensionExtMeshFeatures::ExtensionName));
      REQUIRE(pPrimitiveExtension->featureIds.size() == 1);
      const FeatureId& featureId = pPrimitiveExtension->featureIds[0];
      CHECK(featureId.featureCount == 10);
      CHECK(featureId.attribute == 0);
      CHECK(featureId.propertyTable == 0);
    }
  }

  {
    std::vector<int8_t> expected = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    checkNonArrayProperty<int8_t>(
        model,
        propertyTable,
        defaultClass,
        "id",
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::INT8,
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
    checkNonArrayProperty<double>(
        model,
        propertyTable,
        defaultClass,
        "Height",
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::FLOAT64,
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
    checkNonArrayProperty<double>(
        model,
        propertyTable,
        defaultClass,
        "Longitude",
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::FLOAT64,
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
    checkNonArrayProperty<double>(
        model,
        propertyTable,
        defaultClass,
        "Latitude",
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::FLOAT64,
        expected,
        expected.size());
  }

  {
    std::vector<uint8_t> expected(10, 255);
    checkNonArrayProperty<uint8_t>(
        model,
        propertyTable,
        defaultClass,
        "code",
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::UINT8,
        expected,
        expected.size());
  }

  {
    // clang-format off
    std::vector<glm::dvec3> expected{
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
    checkNonArrayProperty<glm::dvec3>(
        model,
        propertyTable,
        defaultClass,
        "cartographic",
        ClassProperty::Type::VEC3,
        ClassProperty::ComponentType::FLOAT64,
        expected,
        expected.size());
  }
}

TEST_CASE("Converts batched PNTS batch table to EXT_structural_metadata") {
  std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testFilePath = testFilePath / "PointCloud" / "pointCloudBatched.pnts";

  GltfConverterResult result = ConvertTileToGltf::fromPnts(testFilePath);

  REQUIRE(result.model);
  const Model& gltf = *result.model;

  const ExtensionModelExtStructuralMetadata* pExtension =
      gltf.getExtension<ExtensionModelExtStructuralMetadata>();
  REQUIRE(pExtension);
  CHECK(
      gltf.isExtensionUsed(ExtensionModelExtStructuralMetadata::ExtensionName));

  // Check the schema
  REQUIRE(pExtension->schema);
  REQUIRE(pExtension->schema->classes.size() == 1);

  auto firstClassIt = pExtension->schema->classes.begin();
  CHECK(firstClassIt->first == "default");

  const Class& defaultClass = firstClassIt->second;
  REQUIRE(defaultClass.properties.size() == 3);

  {
    auto nameIt = defaultClass.properties.find("name");
    REQUIRE(nameIt != defaultClass.properties.end());
    auto dimensionsIt = defaultClass.properties.find("dimensions");
    REQUIRE(dimensionsIt != defaultClass.properties.end());
    auto idIt = defaultClass.properties.find("id");
    REQUIRE(idIt != defaultClass.properties.end());

    CHECK(nameIt->second.type == ClassProperty::Type::STRING);
    CHECK(dimensionsIt->second.type == ClassProperty::Type::VEC3);
    CHECK(
        dimensionsIt->second.componentType ==
        ClassProperty::ComponentType::FLOAT32);
    CHECK(idIt->second.type == ClassProperty::Type::SCALAR);
    CHECK(idIt->second.componentType == ClassProperty::ComponentType::UINT32);
  }

  // Check the property table
  REQUIRE(pExtension->propertyTables.size() == 1);
  const PropertyTable& propertyTable = pExtension->propertyTables[0];
  CHECK(propertyTable.classProperty == "default");
  REQUIRE(propertyTable.properties.size() == 3);

  {
    auto nameIt = propertyTable.properties.find("name");
    REQUIRE(nameIt != propertyTable.properties.end());
    auto dimensionsIt = propertyTable.properties.find("dimensions");
    REQUIRE(dimensionsIt != propertyTable.properties.end());
    auto idIt = propertyTable.properties.find("id");
    REQUIRE(idIt != propertyTable.properties.end());

    REQUIRE(nameIt->second.values >= 0);
    REQUIRE(
        nameIt->second.values < static_cast<int32_t>(gltf.bufferViews.size()));
    REQUIRE(dimensionsIt->second.values >= 0);
    REQUIRE(
        dimensionsIt->second.values <
        static_cast<int32_t>(gltf.bufferViews.size()));
    REQUIRE(idIt->second.values >= 0);
    REQUIRE(
        idIt->second.values < static_cast<int32_t>(gltf.bufferViews.size()));
  }

  std::set<int32_t> bufferViewSet =
      getUniqueBufferViewIds(gltf.accessors, propertyTable);
  CHECK(bufferViewSet.size() == gltf.bufferViews.size());

  // Check the mesh primitive
  REQUIRE(gltf.meshes.size() == 1);
  const Mesh& mesh = gltf.meshes[0];
  REQUIRE(mesh.primitives.size() == 1);

  const MeshPrimitive& primitive = mesh.primitives[0];
  CHECK(
      primitive.attributes.find("_FEATURE_ID_0") != primitive.attributes.end());

  const ExtensionExtMeshFeatures* pPrimitiveExtension =
      primitive.getExtension<ExtensionExtMeshFeatures>();
  REQUIRE(pPrimitiveExtension);
  CHECK(gltf.isExtensionUsed(ExtensionExtMeshFeatures::ExtensionName));
  REQUIRE(pPrimitiveExtension->featureIds.size() == 1);
  const FeatureId& featureId = pPrimitiveExtension->featureIds[0];
  CHECK(featureId.featureCount == 8);
  CHECK(featureId.attribute == 0);
  CHECK(featureId.propertyTable == 0);

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
    checkNonArrayProperty<std::string, std::string_view>(
        gltf,
        propertyTable,
        defaultClass,
        "name",
        ClassProperty::Type::STRING,
        std::nullopt,
        expected,
        expected.size());
  }

  {
    std::vector<glm::vec3> expected = {
        {0.1182744f, 0.7206326f, 0.6399210f},
        {0.5820198f, 0.1433532f, 0.5373732f},
        {0.9446688f, 0.7586156f, 0.5218483f},
        {0.1059076f, 0.4146619f, 0.4736004f},
        {0.2645556f, 0.1863323f, 0.7742336f},
        {0.7369181f, 0.4561503f, 0.2165503f},
        {0.5684339f, 0.1352181f, 0.0187897f},
        {0.3241409f, 0.6176354f, 0.1496748f}};
    checkNonArrayProperty<glm::vec3>(
        gltf,
        propertyTable,
        defaultClass,
        "dimensions",
        ClassProperty::Type::VEC3,
        ClassProperty::ComponentType::FLOAT32,
        expected,
        expected.size());
  }

  {
    std::vector<uint32_t> expected = {0, 1, 2, 3, 4, 5, 6, 7};
    checkNonArrayProperty<uint32_t>(
        gltf,
        propertyTable,
        defaultClass,
        "id",
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::UINT32,
        expected,
        expected.size());
  }
}

TEST_CASE("Converts per-point PNTS batch table to EXT_structural_metadata") {
  std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testFilePath =
      testFilePath / "PointCloud" / "pointCloudWithPerPointProperties.pnts";

  GltfConverterResult result = ConvertTileToGltf::fromPnts(testFilePath);

  REQUIRE(result.model);
  const Model& gltf = *result.model;

  const ExtensionModelExtStructuralMetadata* pExtension =
      gltf.getExtension<ExtensionModelExtStructuralMetadata>();
  REQUIRE(pExtension);
  CHECK(
      gltf.isExtensionUsed(ExtensionModelExtStructuralMetadata::ExtensionName));

  // Check the schema
  REQUIRE(pExtension->schema);
  REQUIRE(pExtension->schema->classes.size() == 1);

  auto firstClassIt = pExtension->schema->classes.begin();
  CHECK(firstClassIt->first == "default");

  const Class& defaultClass = firstClassIt->second;
  REQUIRE(defaultClass.properties.size() == 3);

  {
    auto temperatureIt = defaultClass.properties.find("temperature");
    REQUIRE(temperatureIt != defaultClass.properties.end());
    auto secondaryColorIt = defaultClass.properties.find("secondaryColor");
    REQUIRE(secondaryColorIt != defaultClass.properties.end());
    auto idIt = defaultClass.properties.find("id");
    REQUIRE(idIt != defaultClass.properties.end());

    CHECK(temperatureIt->second.type == ClassProperty::Type::SCALAR);
    CHECK(
        temperatureIt->second.componentType ==
        ClassProperty::ComponentType::FLOAT32);
    CHECK(secondaryColorIt->second.type == ClassProperty::Type::VEC3);
    REQUIRE(secondaryColorIt->second.componentType);
    CHECK(
        secondaryColorIt->second.componentType ==
        ClassProperty::ComponentType::FLOAT32);
    CHECK(idIt->second.type == ClassProperty::Type::SCALAR);
    CHECK(idIt->second.componentType == ClassProperty::ComponentType::UINT16);
  }

  // Check the property table
  REQUIRE(pExtension->propertyTables.size() == 1);
  const PropertyTable& propertyTable = pExtension->propertyTables[0];
  CHECK(propertyTable.classProperty == "default");
  REQUIRE(propertyTable.properties.size() == 3);

  {
    auto temperatureIt = propertyTable.properties.find("temperature");
    REQUIRE(temperatureIt != propertyTable.properties.end());
    auto secondaryColorIt = propertyTable.properties.find("secondaryColor");
    REQUIRE(secondaryColorIt != propertyTable.properties.end());
    auto idIt = propertyTable.properties.find("id");
    REQUIRE(idIt != propertyTable.properties.end());

    REQUIRE(temperatureIt->second.values >= 0);
    REQUIRE(
        temperatureIt->second.values <
        static_cast<int32_t>(gltf.bufferViews.size()));
    REQUIRE(secondaryColorIt->second.values >= 0);
    REQUIRE(
        secondaryColorIt->second.values <
        static_cast<int32_t>(gltf.bufferViews.size()));
    REQUIRE(idIt->second.values >= 0);
    REQUIRE(
        idIt->second.values < static_cast<int32_t>(gltf.bufferViews.size()));
  }

  std::set<int32_t> bufferViewSet =
      getUniqueBufferViewIds(gltf.accessors, propertyTable);
  CHECK(bufferViewSet.size() == gltf.bufferViews.size());

  // Check the mesh primitive
  REQUIRE(gltf.meshes.size() == 1);
  const Mesh& mesh = gltf.meshes[0];
  REQUIRE(mesh.primitives.size() == 1);

  const MeshPrimitive& primitive = mesh.primitives[0];
  CHECK(
      primitive.attributes.find("_FEATURE_ID_0") == primitive.attributes.end());

  const ExtensionExtMeshFeatures* pPrimitiveExtension =
      primitive.getExtension<ExtensionExtMeshFeatures>();
  REQUIRE(pPrimitiveExtension);
  CHECK(gltf.isExtensionUsed(ExtensionExtMeshFeatures::ExtensionName));
  REQUIRE(pPrimitiveExtension->featureIds.size() == 1);
  const FeatureId& featureId = pPrimitiveExtension->featureIds[0];
  CHECK(featureId.featureCount == 8);
  CHECK(!featureId.attribute);
  CHECK(featureId.propertyTable == 0);

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
    checkNonArrayProperty<float>(
        gltf,
        propertyTable,
        defaultClass,
        "temperature",
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::FLOAT32,
        expected,
        expected.size());
  }

  {
    std::vector<glm::vec3> expected = {
        {0.0202183f, 0, 0},
        {0.3682415f, 0, 0},
        {0.8326198f, 0, 0},
        {0.9571551f, 0, 0},
        {0.7781567f, 0, 0},
        {0.1403507f, 0, 0},
        {0.8700121f, 0, 0},
        {0.8700872f, 0, 0}};
    checkNonArrayProperty<glm::vec3>(
        gltf,
        propertyTable,
        defaultClass,
        "secondaryColor",
        ClassProperty::Type::VEC3,
        ClassProperty::ComponentType::FLOAT32,
        expected,
        expected.size());
  }

  {
    std::vector<uint16_t> expected = {0, 1, 2, 3, 4, 5, 6, 7};
    checkNonArrayProperty<uint16_t>(
        gltf,
        propertyTable,
        defaultClass,
        "id",
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::UINT16,
        expected,
        expected.size());
  }
}

TEST_CASE("Draco-compressed b3dm uses _FEATURE_ID_0 attribute name in glTF") {
  std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testFilePath =
      testFilePath / "BatchTables" / "batchedWithBatchTable-draco.b3dm";

  CesiumGltfReader::GltfReaderOptions options;
  options.decodeDraco = false;

  GltfConverterResult result =
      ConvertTileToGltf::fromB3dm(testFilePath, options);
  CHECK(result.errors.errors.empty());
  CHECK(result.errors.warnings.empty());
  REQUIRE(result.model);

  const Model& gltf = *result.model;

  CHECK(!gltf.meshes.empty());
  for (const Mesh& mesh : gltf.meshes) {
    CHECK(!mesh.primitives.empty());
    for (const MeshPrimitive& primitive : mesh.primitives) {
      CHECK(
          primitive.attributes.find("_FEATURE_ID_0") !=
          primitive.attributes.end());

      const ExtensionKhrDracoMeshCompression* pDraco =
          primitive.getExtension<ExtensionKhrDracoMeshCompression>();
      REQUIRE(pDraco);
      CHECK(
          pDraco->attributes.find("_FEATURE_ID_0") != pDraco->attributes.end());
    }
  }
}

TEST_CASE("Converts Draco per-point PNTS batch table to "
          "EXT_structural_metadata") {
  std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testFilePath = testFilePath / "PointCloud" / "pointCloudDraco.pnts";

  GltfConverterResult result = ConvertTileToGltf::fromPnts(testFilePath);

  REQUIRE(result.model);
  const Model& gltf = *result.model;

  const ExtensionModelExtStructuralMetadata* pExtension =
      gltf.getExtension<ExtensionModelExtStructuralMetadata>();
  REQUIRE(pExtension);
  CHECK(
      gltf.isExtensionUsed(ExtensionModelExtStructuralMetadata::ExtensionName));

  // Check the schema
  REQUIRE(pExtension->schema);
  REQUIRE(pExtension->schema->classes.size() == 1);

  auto firstClassIt = pExtension->schema->classes.begin();
  CHECK(firstClassIt->first == "default");

  const Class& defaultClass = firstClassIt->second;
  REQUIRE(defaultClass.properties.size() == 3);

  {
    auto temperatureIt = defaultClass.properties.find("temperature");
    REQUIRE(temperatureIt != defaultClass.properties.end());
    auto secondaryColorIt = defaultClass.properties.find("secondaryColor");
    REQUIRE(secondaryColorIt != defaultClass.properties.end());
    auto idIt = defaultClass.properties.find("id");
    REQUIRE(idIt != defaultClass.properties.end());

    CHECK(temperatureIt->second.type == ClassProperty::Type::SCALAR);
    CHECK(
        temperatureIt->second.componentType ==
        ClassProperty::ComponentType::FLOAT32);
    CHECK(secondaryColorIt->second.type == ClassProperty::Type::VEC3);
    REQUIRE(secondaryColorIt->second.componentType);
    CHECK(
        secondaryColorIt->second.componentType ==
        ClassProperty::ComponentType::FLOAT32);
    CHECK(idIt->second.type == ClassProperty::Type::SCALAR);
    CHECK(idIt->second.componentType == ClassProperty::ComponentType::UINT16);
  }

  // Check the property table
  REQUIRE(pExtension->propertyTables.size() == 1);
  const PropertyTable& propertyTable = pExtension->propertyTables[0];
  CHECK(propertyTable.classProperty == "default");
  REQUIRE(propertyTable.properties.size() == 3);

  {
    auto temperatureIt = propertyTable.properties.find("temperature");
    REQUIRE(temperatureIt != propertyTable.properties.end());
    auto secondaryColorIt = propertyTable.properties.find("secondaryColor");
    REQUIRE(secondaryColorIt != propertyTable.properties.end());
    auto idIt = propertyTable.properties.find("id");
    REQUIRE(idIt != propertyTable.properties.end());

    REQUIRE(temperatureIt->second.values >= 0);
    REQUIRE(
        temperatureIt->second.values <
        static_cast<int32_t>(gltf.bufferViews.size()));
    REQUIRE(secondaryColorIt->second.values >= 0);
    REQUIRE(
        secondaryColorIt->second.values <
        static_cast<int32_t>(gltf.bufferViews.size()));
    REQUIRE(idIt->second.values >= 0);
    REQUIRE(
        idIt->second.values < static_cast<int32_t>(gltf.bufferViews.size()));
  }

  std::set<int32_t> bufferViewSet =
      getUniqueBufferViewIds(gltf.accessors, propertyTable);
  CHECK(bufferViewSet.size() == gltf.bufferViews.size());

  // Check the mesh primitive
  REQUIRE(gltf.meshes.size() == 1);
  const Mesh& mesh = gltf.meshes[0];
  REQUIRE(mesh.primitives.size() == 1);

  const MeshPrimitive& primitive = mesh.primitives[0];
  CHECK(
      primitive.attributes.find("_FEATURE_ID_0") == primitive.attributes.end());

  const ExtensionExtMeshFeatures* pPrimitiveExtension =
      primitive.getExtension<ExtensionExtMeshFeatures>();
  REQUIRE(pPrimitiveExtension);
  CHECK(gltf.isExtensionUsed(ExtensionExtMeshFeatures::ExtensionName));
  REQUIRE(pPrimitiveExtension->featureIds.size() == 1);
  const FeatureId& featureId = pPrimitiveExtension->featureIds[0];
  CHECK(featureId.featureCount == 8);
  CHECK(!featureId.attribute);
  CHECK(featureId.propertyTable == 0);

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
    checkNonArrayProperty<float>(
        gltf,
        propertyTable,
        defaultClass,
        "temperature",
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::FLOAT32,
        expected,
        expected.size());
  }

  {
    std::vector<glm::vec3> expected = {
        {0.1182744f, 0, 0},
        {0.7206645f, 0, 0},
        {0.6399421f, 0, 0},
        {0.5820239f, 0, 0},
        {0.1432983f, 0, 0},
        {0.5374249f, 0, 0},
        {0.9446688f, 0, 0},
        {0.7586040f, 0, 0}};
    checkNonArrayProperty<glm::vec3>(
        gltf,
        propertyTable,
        defaultClass,
        "secondaryColor",
        ClassProperty::Type::VEC3,
        ClassProperty::ComponentType::FLOAT32,
        expected,
        expected.size());
  }

  {
    std::vector<uint16_t> expected = {0, 1, 2, 3, 4, 5, 6, 7};
    checkNonArrayProperty<uint16_t>(
        gltf,
        propertyTable,
        defaultClass,
        "id",
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::UINT16,
        expected,
        expected.size());
  }
}

TEST_CASE("Upgrade nested JSON metadata to string") {
  std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testFilePath =
      testFilePath / "BatchTables" / "batchedWithStringAndNestedJson.b3dm";

  GltfConverterResult result = ConvertTileToGltf::fromB3dm(testFilePath);

  REQUIRE(!result.errors);
  REQUIRE(result.model);

  const Model& model = *result.model;
  const ExtensionModelExtStructuralMetadata* pMetadata =
      result.model->getExtension<ExtensionModelExtStructuralMetadata>();
  REQUIRE(pMetadata);

  const CesiumUtility::IntrusivePointer<Schema>& schema = pMetadata->schema;
  REQUIRE(schema);

  const std::unordered_map<std::string, Class>& classes = schema->classes;
  REQUIRE(classes.size() == 1);

  const Class& defaultClass = classes.at("default");
  const std::unordered_map<std::string, ClassProperty>& properties =
      defaultClass.properties;
  REQUIRE(properties.size() == 6);

  REQUIRE(pMetadata->propertyTables.size() == 1);
  const PropertyTable& propertyTable = pMetadata->propertyTables[0];
  REQUIRE(propertyTable.count == 10);

  {
    std::vector<std::string> expected;
    for (int64_t i = 0; i < propertyTable.count; ++i) {
      std::string v = std::string("{\"name\":\"building") + std::to_string(i) +
                      "\",\"year\":" + std::to_string(i) + "}";
      expected.push_back(v);
    }
    checkNonArrayProperty<std::string, std::string_view>(
        model,
        propertyTable,
        defaultClass,
        "info",
        ClassProperty::Type::STRING,
        std::nullopt,
        expected,
        expected.size());
  }

  {
    std::vector<std::vector<std::string>> expected;
    for (int64_t i = 0; i < propertyTable.count; ++i) {
      std::vector<std::string> expectedVal;
      expectedVal.emplace_back("room" + std::to_string(i) + "_a");
      expectedVal.emplace_back("room" + std::to_string(i) + "_b");
      expectedVal.emplace_back("room" + std::to_string(i) + "_c");
      expected.emplace_back(std::move(expectedVal));
    }
    checkArrayProperty<std::string, std::string_view>(
        model,
        propertyTable,
        defaultClass,
        "rooms",
        3,
        ClassProperty::Type::STRING,
        std::nullopt,
        expected,
        expected.size());
  }
}

TEST_CASE("Upgrade JSON booleans to binary") {
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

  auto errors = BatchTableToGltfStructuralMetadata::convertFromB3dm(
      featureTableJson,
      batchTableJson,
      std::span<const std::byte>(),
      model);

  const ExtensionModelExtStructuralMetadata* pMetadata =
      model.getExtension<ExtensionModelExtStructuralMetadata>();
  REQUIRE(pMetadata);

  const CesiumUtility::IntrusivePointer<Schema>& schema = pMetadata->schema;
  REQUIRE(schema);

  const std::unordered_map<std::string, Class>& classes = schema->classes;
  REQUIRE(classes.size() == 1);

  const Class& defaultClass = classes.at("default");
  const std::unordered_map<std::string, ClassProperty>& properties =
      defaultClass.properties;
  REQUIRE(properties.size() == 1);

  const ClassProperty& propertyClass = properties.at("boolProp");
  REQUIRE(propertyClass.type == "BOOLEAN");

  REQUIRE(pMetadata->propertyTables.size() == 1);
  const PropertyTable& propertyTable = pMetadata->propertyTables[0];
  checkNonArrayProperty(
      model,
      propertyTable,
      defaultClass,
      "boolProp",
      ClassProperty::Type::BOOLEAN,
      std::nullopt,
      expected,
      expected.size());
}

TEST_CASE("Upgrade fixed-length JSON arrays") {
  SUBCASE("int8_t") {
    // clang-format off
    std::vector<std::vector<int8_t>> expected {
      {0, 1, 4, 1},
      {12, 50, -12, -1},
      {123, 10, 122, 3},
      {13, 45, 122, 94},
      {11, 22, 3, 5}};
    // clang-format on

    createTestForArrayJson(
        expected,
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::INT8,
        4,
        expected.size());
  }

  SUBCASE("uint8_t") {
    // clang-format off
    std::vector<std::vector<uint8_t>> expected {
      {0, 1, 4, 1, 223},
      {12, 50, 242, 212, 11},
      {223, 10, 122, 3, 44},
      {13, 45, 122, 94, 244},
      {119, 112, 156, 5, 35}};
    // clang-format on

    createTestForArrayJson(
        expected,
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::UINT8,
        5,
        expected.size());
  }

  SUBCASE("int16_t") {
    // clang-format off
     std::vector<std::vector<int16_t>> expected {
      {0, 1, 4, 4445},
      {12, 50, -12, -1},
      {123, 10, 3333, 3},
      {13, 450, 122, 94},
      {11, 22, 3, 50}};
    // clang-format on

    createTestForArrayJson(
        expected,
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::INT16,
        4,
        expected.size());
  }

  SUBCASE("uint16_t") {
    // clang-format off
     std::vector<std::vector<uint16_t>> expected {
      {0, 1, 4, 65000},
      {12, 50, 12, 1},
      {123, 10, 33330, 3},
      {13, 450, 1220, 94},
      {11, 22, 3, 50000}};
    // clang-format on

    createTestForArrayJson(
        expected,
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::UINT16,
        4,
        expected.size());
  }

  SUBCASE("int32_t") {
    // clang-format off
     std::vector<std::vector<int32_t>> expected {
      {0, 1, 4, 1},
      {1244, -500000, 1222, 544662},
      {123, -10, 122, 334},
      {13, 45, 122, 94},
      {11, 22, 3, 2147483647}};
    // clang-format on

    createTestForArrayJson(
        expected,
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::INT32,
        4,
        expected.size());
  }

  SUBCASE("uint32_t") {
    // clang-format off
     std::vector<std::vector<uint32_t>> expected {
      {0, 1, 4, 1},
      {1244, 12200000, 1222, 544662},
      {123, 10, 122, 334},
      {13, 45, 122, 94},
      {11, 22, 3, (uint32_t)4294967295}};
    // clang-format on

    createTestForArrayJson(
        expected,
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::UINT32,
        4,
        expected.size());
  }

  SUBCASE("int64_t") {
    // The max positive number only requires uint32_t, but due to
    // the negative number, it is upgraded to int64_t.
    // clang-format off
     std::vector<std::vector<int64_t>> expected {
      {0, 1, 4, 1},
      {1244, -922, 1222, 54},
      {123, 10, 122, 334},
      {13, 45, 122, 94},
      {11, 22, 3, 3147483647}};
    // clang-format on

    createTestForArrayJson(
        expected,
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::INT64,
        4,
        expected.size());
  }

  SUBCASE("uint64_t") {
    // clang-format off
     std::vector<std::vector<uint64_t>> expected {
      {0, 1, 4, 1},
      {1244, 13223302036854775807u, 1222, 544662},
      {123, 10, 122, 334},
      {13, 45, 122, 94},
      {11, 22, 3, 13223302036854775807u}};
    // clang-format on

    createTestForArrayJson(
        expected,
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::UINT64,
        4,
        expected.size());
  }

  SUBCASE("float") {
    // clang-format off
     std::vector<std::vector<float>> expected {
      {0.122f, 1.1233f, 4.113f, 1.11f},
      {1.244f, 122.3f, 1.222f, 544.66f},
      {12.003f, 1.21f, 2.123f, 33.12f},
      {1.333f, 4.232f, 1.422f, 9.4f},
      {1.1221f, 2.2f, 3.0f, 122.31f}};
    // clang-format on

    createTestForArrayJson(
        expected,
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::FLOAT32,
        4,
        expected.size());
  }

  SUBCASE("double") {
    // clang-format off
     std::vector<std::vector<double>> expected {
      {0.122, 1.1233, 4.113, 1.11},
      {1.244, 122.3, 1.222, 544.66},
      {12.003, 1.21, 2.123, 33.12},
      {1.333, 4.232, 1.422, 9.4},
      {1.1221, 2.2, 3.0, 122.31}};
    // clang-format on

    createTestForArrayJson(
        expected,
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::FLOAT64,
        4,
        expected.size());
  }

  SUBCASE("string") {
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
        ClassProperty::Type::STRING,
        std::nullopt,
        4,
        expected.size());
  }

  SUBCASE("Boolean") {
    // clang-format off
     std::vector<std::vector<bool>> expected{
      {true, true, false, true, false, true},
      {true, false, true, false, true, true},
      {false, true, true, false, false, true},
      {false, true, true, true, true, true},
    };
    // clang-format on

    createTestForArrayJson(
        expected,
        ClassProperty::Type::BOOLEAN,
        std::nullopt,
        6,
        expected.size());
  }
}

TEST_CASE("Upgrade variable-length JSON arrays") {
  SUBCASE("int8_t") {
    // clang-format off
    std::vector<std::vector<int8_t>> expected {
      {0, 1, 4},
      {12, 50, -12},
      {123, 10, 122, 3, 23},
      {13, 45},
      {11, 22, 3, 5, 33, 12, -122}};
    // clang-format on

    createTestForArrayJson(
        expected,
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::INT8,
        0,
        expected.size());
  }

  SUBCASE("uint8_t") {
    // clang-format off
    std::vector<std::vector<uint8_t>> expected {
      {0, 223},
      {12, 50, 242, 212, 11},
      {223},
      {13, 45},
      {119, 112, 156, 5, 35, 244, 122}};
    // clang-format on

    createTestForArrayJson(
        expected,
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::UINT8,
        0,
        expected.size());
  }

  SUBCASE("int16_t") {
    // clang-format off
    std::vector<std::vector<int16_t>> expected {
      {0, 1, 4, 4445, 12333},
      {12, 50, -12, -1},
      {123, 10},
      {13, 450, 122, 94, 334},
      {11, 22, 3, 50, 455, 122, 3333, 5555, 12233}};
    // clang-format on

    createTestForArrayJson(
        expected,
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::INT16,
        0,
        expected.size());
  }

  SUBCASE("uint16_t") {
    // clang-format off
    std::vector<std::vector<uint16_t>> expected {
      {0, 1},
      {12, 50, 12, 1, 333, 5666},
      {123, 10, 33330, 3, 1},
      {13, 1220},
      {11, 22, 3, 50000, 333}};
    // clang-format on

    createTestForArrayJson(
        expected,
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::UINT16,
        0,
        expected.size());
  }

  SUBCASE("int32_t") {
    // clang-format off
    std::vector<std::vector<int32_t>> expected {
      {0, 1},
      {1244, -500000, 1222, 544662},
      {123, -10},
      {13},
      {11, 22, 3, 2147483647, 12233}};
    // clang-format on

    createTestForArrayJson(
        expected,
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::INT32,
        0,
        expected.size());
  }

  SUBCASE("uint32_t") {
    // clang-format off
    std::vector<std::vector<uint32_t>> expected {
      {0, 1},
      {1244, 12200000, 1222, 544662},
      {123, 10},
      {13, 45, 122, 94, 333, 212, 534, 1122},
      {11, 22, 3, (uint32_t)4294967295}};
    // clang-format on

    createTestForArrayJson(
        expected,
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::UINT32,
        0,
        expected.size());
  }

  SUBCASE("int64_t") {
    // clang-format off
    std::vector<std::vector<int64_t>> expected {
      {0, 1, 4, 1},
      {1244, -9223372036854775807, 1222, 544662, 12233},
      {123},
      {13, 45},
      {11, 22, 3, 9223372036854775807, 12333}};
    // clang-format on

    createTestForArrayJson(
        expected,
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::INT64,
        0,
        expected.size());
  }

  SUBCASE("uint64_t") {
    // clang-format off
    std::vector<std::vector<uint64_t>> expected {
      {1},
      {1244, 13223302036854775807u, 1222, 544662},
      {123, 10, 2},
      {13, 94},
      {11, 22, 3, 13223302036854775807u, 32323}};
    // clang-format on

    createTestForArrayJson(
        expected,
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::UINT64,
        0,
        expected.size());
  }

  SUBCASE("float") {
    // clang-format off
    std::vector<std::vector<float>> expected {
      {0.122f, 1.1233f},
      {1.244f, 122.3f, 1.222f, 544.66f, 323.122f},
      {12.003f, 1.21f, 2.123f, 33.12f, 122.2f},
      {1.333f},
      {1.1221f, 2.2f}};
    // clang-format on

    createTestForArrayJson(
        expected,
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::FLOAT32,
        0,
        expected.size());
  }

  SUBCASE("double") {
    // clang-format off
    std::vector<std::vector<double>> expected {
      {0.122, 1.1233},
      {1.244, 122.3, 1.222, 544.66, 323.122},
      {12.003, 1.21, 2.123, 33.12, 122.2},
      {1.333},
      {1.1221, 2.2}};
    // clang-format on

    createTestForArrayJson(
        expected,
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::FLOAT64,
        0,
        expected.size());
  }

  SUBCASE("string") {
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
        ClassProperty::Type::STRING,
        std::nullopt,
        0,
        expected.size());
  }

  SUBCASE("Boolean") {
    // clang-format off
    std::vector<std::vector<bool>> expected{
      {true, true, false, true, false, false, true},
      {true, false},
      {false, true, true, false},
      {false, true, true},
      {true, true, true, true, false, false}
    };
    // clang-format on

    createTestForArrayJson(
        expected,
        ClassProperty::Type::BOOLEAN,
        std::nullopt,
        0,
        expected.size());
  }
}

TEST_CASE("Upgrade JSON values") {
  SUBCASE("Uint32") {
    // Even though the values are typed uint32, they are small enough to be
    // stored as int8s. Signed types are preferred over unsigned.
    std::vector<uint32_t> expected{32, 45, 21, 65, 78};
    createTestForNonArrayJson<uint32_t, int8_t>(
        expected,
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::INT8,
        expected.size());
  }

  SUBCASE("Boolean") {
    std::vector<bool> expected{true, false, true, false, true, true, false};
    createTestForNonArrayJson(
        expected,
        ClassProperty::Type::BOOLEAN,
        std::nullopt,
        expected.size());
  }

  SUBCASE("String") {
    std::vector<std::string> expected{"Test 0", "Test 1", "Test 2", "Test 3"};
    createTestForNonArrayJson<std::string, std::string_view>(
        expected,
        ClassProperty::Type::STRING,
        std::nullopt,
        expected.size());
  }
}

TEST_CASE("Uses sentinel values for JSON null values") {
  SUBCASE("Uint32 with sentinel value 0") {
    // Even though the values are typed uint32, they are small enough to be
    // stored as int8s. Signed types are preferred over unsigned.
    std::vector<uint32_t> expected{32, 45, 0, 21, 0, 65, 78};
    createTestForNonArrayJson<uint32_t, int8_t>(
        expected,
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::INT8,
        expected.size(),
        static_cast<int8_t>(0));
  }

  SUBCASE("Int32 with sentinel value 0") {
    // Even though the values are typed int32, they are small enough to be
    // stored as int8s. Signed types are preferred over unsigned.
    std::vector<int32_t> expected{32, 45, -3, 0, 21, 0, -65, 78};
    createTestForNonArrayJson<int32_t, int8_t>(
        expected,
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::INT8,
        expected.size(),
        static_cast<int8_t>(0));
  }

  SUBCASE("Int32 with sentinel value -1") {
    // Even though the values are typed int32, they are small enough to be
    // stored as int8s. Signed types are preferred over unsigned.
    std::vector<int32_t> expected{32, 45, -3, 0, 21, 0, -1, -65, 78};
    createTestForNonArrayJson<int32_t, int8_t>(
        expected,
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::INT8,
        expected.size(),
        static_cast<int8_t>(-1));
  }

  SUBCASE("String with 'null'") {
    std::vector<std::string> expected{
        "Test 0",
        "Test 1",
        "Test 2",
        "null"
        "Test 3"};
    createTestForNonArrayJson<std::string, std::string_view>(
        expected,
        ClassProperty::Type::STRING,
        std::nullopt,
        expected.size(),
        std::string_view("null"));
  }
}

TEST_CASE("Defaults to string if no sentinel values are available") {
  SUBCASE("Uint64") {
    Model model;
    std::vector<std::optional<uint64_t>> expected{
        32,
        45,
        0,
        255,
        std::nullopt,
        0,
        65,
        78,
        std::numeric_limits<uint64_t>::max()};

    rapidjson::Document featureTableJson;
    featureTableJson.SetObject();
    rapidjson::Value batchLength(rapidjson::kNumberType);
    batchLength.SetUint64(static_cast<uint64_t>(expected.size()));
    featureTableJson.AddMember(
        "BATCH_LENGTH",
        batchLength,
        featureTableJson.GetAllocator());

    rapidjson::Document batchTableJson;
    batchTableJson.SetObject();
    rapidjson::Value scalarProperty(rapidjson::kArrayType);
    for (size_t i = 0; i < expected.size(); ++i) {
      if (!expected[i]) {
        rapidjson::Value nullValue;
        nullValue.SetNull();
        scalarProperty.PushBack(nullValue, batchTableJson.GetAllocator());
        continue;
      }

      scalarProperty.PushBack(*expected[i], batchTableJson.GetAllocator());
    }

    batchTableJson.AddMember(
        "scalarProperty",
        scalarProperty,
        batchTableJson.GetAllocator());

    auto errors = BatchTableToGltfStructuralMetadata::convertFromB3dm(
        featureTableJson,
        batchTableJson,
        std::span<const std::byte>(),
        model);

    const ExtensionModelExtStructuralMetadata* pMetadata =
        model.getExtension<ExtensionModelExtStructuralMetadata>();
    REQUIRE(pMetadata);

    const CesiumUtility::IntrusivePointer<Schema> schema = pMetadata->schema;
    REQUIRE(schema);

    const std::unordered_map<std::string, Class>& classes = schema->classes;
    REQUIRE(classes.size() == 1);

    const Class& defaultClass = classes.at("default");
    const std::unordered_map<std::string, ClassProperty>& properties =
        defaultClass.properties;
    REQUIRE(properties.size() == 1);

    REQUIRE(pMetadata->propertyTables.size() == 1);

    const PropertyTable& propertyTable = pMetadata->propertyTables[0];
    const ClassProperty& property =
        defaultClass.properties.at("scalarProperty");
    REQUIRE(property.type == ClassProperty::Type::STRING);
    REQUIRE(!property.componentType);
    REQUIRE(!property.array);
    REQUIRE(!property.count);

    PropertyTableView view(model, propertyTable);
    REQUIRE(view.status() == PropertyTableViewStatus::Valid);
    REQUIRE(view.size() == propertyTable.count);

    PropertyTablePropertyView<std::string_view> propertyView =
        view.getPropertyView<std::string_view>("scalarProperty");
    REQUIRE(propertyView.status() == PropertyTablePropertyViewStatus::Valid);
    REQUIRE(propertyView.size() == propertyTable.count);
    REQUIRE(propertyView.size() == static_cast<int64_t>(expected.size()));
    for (int64_t i = 0; i < propertyView.size(); ++i) {
      auto expectedValue = expected[static_cast<size_t>(i)];
      if (expectedValue) {
        std::string asString = std::to_string(*expectedValue);
        REQUIRE(propertyView.getRaw(i) == asString);
      } else {
        REQUIRE(propertyView.getRaw(i) == "null");
      }

      REQUIRE(propertyView.get(i) == propertyView.getRaw(i));
    }
  }

  SUBCASE("Int32") {
    Model model;
    std::vector<std::optional<int32_t>>
        expected{32, 45, 0, -1, std::nullopt, 0, 65, 78};

    rapidjson::Document featureTableJson;
    featureTableJson.SetObject();
    rapidjson::Value batchLength(rapidjson::kNumberType);
    batchLength.SetUint64(static_cast<uint64_t>(expected.size()));
    featureTableJson.AddMember(
        "BATCH_LENGTH",
        batchLength,
        featureTableJson.GetAllocator());

    rapidjson::Document batchTableJson;
    batchTableJson.SetObject();
    rapidjson::Value scalarProperty(rapidjson::kArrayType);
    for (size_t i = 0; i < expected.size(); ++i) {
      if (!expected[i]) {
        rapidjson::Value nullValue;
        nullValue.SetNull();
        scalarProperty.PushBack(nullValue, batchTableJson.GetAllocator());
        continue;
      }

      scalarProperty.PushBack(*expected[i], batchTableJson.GetAllocator());
    }

    batchTableJson.AddMember(
        "scalarProperty",
        scalarProperty,
        batchTableJson.GetAllocator());

    auto errors = BatchTableToGltfStructuralMetadata::convertFromB3dm(
        featureTableJson,
        batchTableJson,
        std::span<const std::byte>(),
        model);

    const ExtensionModelExtStructuralMetadata* pMetadata =
        model.getExtension<ExtensionModelExtStructuralMetadata>();
    REQUIRE(pMetadata);

    const CesiumUtility::IntrusivePointer<Schema> schema = pMetadata->schema;
    REQUIRE(schema);

    const std::unordered_map<std::string, Class>& classes = schema->classes;
    REQUIRE(classes.size() == 1);

    const Class& defaultClass = classes.at("default");
    const std::unordered_map<std::string, ClassProperty>& properties =
        defaultClass.properties;
    REQUIRE(properties.size() == 1);

    REQUIRE(pMetadata->propertyTables.size() == 1);

    const PropertyTable& propertyTable = pMetadata->propertyTables[0];
    const ClassProperty& property =
        defaultClass.properties.at("scalarProperty");
    REQUIRE(property.type == ClassProperty::Type::STRING);
    REQUIRE(!property.componentType);
    REQUIRE(!property.array);
    REQUIRE(!property.count);

    PropertyTableView view(model, propertyTable);
    REQUIRE(view.status() == PropertyTableViewStatus::Valid);
    REQUIRE(view.size() == propertyTable.count);

    PropertyTablePropertyView<std::string_view> propertyView =
        view.getPropertyView<std::string_view>("scalarProperty");
    REQUIRE(propertyView.status() == PropertyTablePropertyViewStatus::Valid);
    REQUIRE(propertyView.size() == propertyTable.count);
    REQUIRE(propertyView.size() == static_cast<int64_t>(expected.size()));
    for (int64_t i = 0; i < propertyView.size(); ++i) {
      auto expectedValue = expected[static_cast<size_t>(i)];
      if (expectedValue) {
        std::string asString = std::to_string(*expectedValue);
        REQUIRE(propertyView.getRaw(i) == asString);
      } else {
        REQUIRE(propertyView.getRaw(i) == "null");
      }

      REQUIRE(propertyView.get(i) == propertyView.getRaw(i));
    }
  }
}

TEST_CASE("Cannot write past batch table length") {
  SUBCASE("Uint32") {
    std::vector<uint32_t> expected{32, 45, 21, 65, 78, 20, 33, 12};
    createTestForNonArrayJson<uint32_t, int8_t>(
        expected,
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::INT8,
        4);
  }

  SUBCASE("Boolean") {
    std::vector<bool> expected{true, false, true, false, true, true, false};
    createTestForNonArrayJson(
        expected,
        ClassProperty::Type::BOOLEAN,
        std::nullopt,
        4);
  }

  SUBCASE("String") {
    std::vector<std::string>
        expected{"Test 0", "Test 1", "Test 2", "Test 3", "Test 4"};
    createTestForNonArrayJson<std::string, std::string_view>(
        expected,
        ClassProperty::Type::STRING,
        std::nullopt,
        3);
  }

  SUBCASE("Fixed-length scalar array") {
    // clang-format off
    std::vector<std::vector<uint64_t>> expected {
      {0, 1, 4, 1},
      {1244, 13223302036854775807u, 1222, 544662},
      {123, 10, 122, 334},
      {13, 45, 122, 94},
      {11, 22, 3, 13223302036854775807u}};
    // clang-format on

    createTestForArrayJson(
        expected,
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::UINT64,
        4,
        2);
  }

  SUBCASE("Fixed-length boolean array") {
    // clang-format off
    std::vector<std::vector<bool>> expected{
      {true, true, false},
      {true, false, true},
      {false, true, true},
      {false, true, true},
    };
    // clang-format on

    createTestForArrayJson(
        expected,
        ClassProperty::Type::BOOLEAN,
        std::nullopt,
        3,
        2);
  }

  SUBCASE("Fixed-length string array") {
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
        ClassProperty::Type::STRING,
        std::nullopt,
        4,
        2);
  }

  SUBCASE("Variable-length number array") {
    // clang-format off
      std::vector<std::vector<int32_t>> expected {
        {0, 1},
        {1244, -500000, 1222, 544662},
        {123, -10},
        {13},
        {11, 22, 3, 2147483647, 12233}};
    // clang-format on

    createTestForArrayJson(
        expected,
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::INT32,
        0,
        3);
  }

  SUBCASE("Variable-length boolean array") {
    // clang-format off
      std::vector<std::vector<bool>> expected{
        {true, true, false, true, false, false, true},
        {true, false},
        {false, true, true, false},
        {false, true, true},
        {true, true, false, false}
      };
    // clang-format on

    createTestForArrayJson(
        expected,
        ClassProperty::Type::BOOLEAN,
        std::nullopt,
        0,
        2);
  }

  SUBCASE("Variable-length string array") {
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
        ClassProperty::Type::STRING,
        std::nullopt,
        0,
        2);
  }
}

TEST_CASE("Converts \"Feature Classes\" 3DTILES_batch_table_hierarchy example "
          "to EXT_structural_metadata") {
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

  auto errors = BatchTableToGltfStructuralMetadata::convertFromB3dm(
      featureTableParsed,
      batchTableParsed,
      std::span<const std::byte>(),
      gltf);

  ExtensionModelExtStructuralMetadata* pExtension =
      gltf.getExtension<ExtensionModelExtStructuralMetadata>();
  REQUIRE(pExtension);
  CHECK(
      gltf.isExtensionUsed(ExtensionModelExtStructuralMetadata::ExtensionName));

  // Check the schema
  REQUIRE(pExtension->schema);
  REQUIRE(pExtension->schema->classes.size() == 1);

  auto firstClassIt = pExtension->schema->classes.begin();
  CHECK(firstClassIt->first == "default");

  Class& defaultClass = firstClassIt->second;
  REQUIRE(defaultClass.properties.size() == 6);

  // Check the property table
  REQUIRE(pExtension->propertyTables.size() == 1);
  PropertyTable& propertyTable = pExtension->propertyTables[0];
  CHECK(propertyTable.classProperty == "default");
  REQUIRE(propertyTable.properties.size() == 6);

  struct ExpectedString {
    std::string name;
    std::vector<std::string> values;
    std::optional<std::string> noDataValue;
  };

  struct ExpectedScalar {
    std::string name;
    std::vector<int8_t> values;
    std::optional<int8_t> noDataValue;
  };

  std::vector<ExpectedScalar> expectedScalar{
      {"lampStrength", {10, 5, 7, 0, 0, 0, 0, 0}, static_cast<int8_t>(0)},
      {"treeHeight", {0, 0, 0, 0, 0, 0, 10, 15}, static_cast<int8_t>(0)},
      {"treeAge", {0, 0, 0, 0, 0, 0, 5, 8}, static_cast<int8_t>(0)}};

  std::vector<ExpectedString> expectedString{
      {"lampColor",
       {"yellow", "white", "white", "null", "null", "null", "null", "null"},
       "null"},
      {"carType",
       {"null", "null", "null", "truck", "bus", "sedan", "null", "null"},
       "null"},
      {"carColor",
       {"null", "null", "null", "green", "blue", "red", "null", "null"},
       "null"}};

  for (const auto& expected : expectedScalar) {
    auto it = defaultClass.properties.find(expected.name);
    REQUIRE(it != defaultClass.properties.end());
    CHECK(it->second.type == ClassProperty::Type::SCALAR);
    CHECK(it->second.componentType == ClassProperty::ComponentType::INT8);

    checkNonArrayProperty<int8_t>(
        gltf,
        propertyTable,
        defaultClass,
        expected.name,
        ClassProperty::Type::SCALAR,
        ClassProperty::ComponentType::INT8,
        expected.values,
        expected.values.size(),
        expected.noDataValue);
  }

  for (const auto& expected : expectedString) {
    auto it = defaultClass.properties.find(expected.name);
    REQUIRE(it != defaultClass.properties.end());
    CHECK(it->second.type == ClassProperty::Type::STRING);

    checkNonArrayProperty<std::string, std::string_view>(
        gltf,
        propertyTable,
        defaultClass,
        expected.name,
        ClassProperty::Type::STRING,
        std::nullopt,
        expected.values,
        expected.values.size(),
        expected.noDataValue);
  }
}

TEST_CASE("Omits value-less properties when converting "
          "3DTILES_batch_table_hierarchy to EXT_structural_metadata") {
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
                "lampColor" : ["yellow", "white", "white"],
                "missingValues": []
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

  auto errors = BatchTableToGltfStructuralMetadata::convertFromB3dm(
      featureTableParsed,
      batchTableParsed,
      std::span<const std::byte>(),
      gltf);

  ExtensionModelExtStructuralMetadata* pExtension =
      gltf.getExtension<ExtensionModelExtStructuralMetadata>();
  REQUIRE(pExtension);
  CHECK(
      gltf.isExtensionUsed(ExtensionModelExtStructuralMetadata::ExtensionName));

  // Check the schema
  REQUIRE(pExtension->schema);
  REQUIRE(pExtension->schema->classes.size() == 1);

  auto firstClassIt = pExtension->schema->classes.begin();
  CHECK(firstClassIt->first == "default");

  Class& defaultClass = firstClassIt->second;
  REQUIRE(defaultClass.properties.size() == 7);

  // Check the property table
  REQUIRE(pExtension->propertyTables.size() == 1);
  PropertyTable& propertyTable = pExtension->propertyTables[0];
  CHECK(propertyTable.classProperty == "default");

  // Verify that all property table properties refer to a valid bufferView.
  for (const std::pair<const std::string, PropertyTableProperty>& pair :
       propertyTable.properties) {
    CHECK(pair.second.values >= 0);
    CHECK(size_t(pair.second.values) < gltf.bufferViews.size());
  }

  CHECK(
      propertyTable.properties.find("missingValues") ==
      propertyTable.properties.end());
}

TEST_CASE(
    "Converts \"Feature Hierarchy\" 3DTILES_batch_table_hierarchy example to "
    "EXT_structural_metadata") {
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
                "wall_color" : ["blue", "pink", "green", "lime", "black",
                "brown"], "wall_windows" : [2, 4, 4, 2, 0, 3]
              }
            },
            {
              "name" : "Building",
              "length" : 3,
              "instances" : {
                "building_name" : ["building_0", "building_1",
                "building_2"], "building_id" : [0, 1, 2], "building_address"
                : ["10 Main St", "12 Main St", "14 Main St"]
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

  BatchTableToGltfStructuralMetadata::convertFromB3dm(
      featureTableParsed,
      batchTableParsed,
      std::span<const std::byte>(),
      gltf);

  ExtensionModelExtStructuralMetadata* pExtension =
      gltf.getExtension<ExtensionModelExtStructuralMetadata>();
  REQUIRE(pExtension);
  CHECK(
      gltf.isExtensionUsed(ExtensionModelExtStructuralMetadata::ExtensionName));

  // Check the schema
  REQUIRE(pExtension->schema);
  REQUIRE(pExtension->schema->classes.size() == 1);

  auto firstClassIt = pExtension->schema->classes.begin();
  CHECK(firstClassIt->first == "default");

  Class& defaultClass = firstClassIt->second;
  REQUIRE(defaultClass.properties.size() == 7);

  // Check the property table
  REQUIRE(pExtension->propertyTables.size() == 1);
  PropertyTable& propertyTable = pExtension->propertyTables[0];
  CHECK(propertyTable.classProperty == "default");
  REQUIRE(propertyTable.properties.size() == 7);

  struct ExpectedString {
    std::string name;
    std::vector<std::string> values;
    std::string type() const { return ClassProperty::Type::STRING; }
    std::optional<std::string> componentType() const { return std::nullopt; }
  };

  std::vector<ExpectedString> expectedStringProperties{
      {"wall_color", {"blue", "pink", "green", "lime", "black", "brown"}},
      {"building_name",
       {"building_0",
        "building_0",
        "building_1",
        "building_1",
        "building_2",
        "building_2"}},
      {"building_address",
       {"10 Main St",
        "10 Main St",
        "12 Main St",
        "12 Main St",
        "14 Main St",
        "14 Main St"}},
      {"block_district",
       {"central", "central", "central", "central", "central", "central"}}};

  for (const auto& expected : expectedStringProperties) {
    auto it = defaultClass.properties.find(expected.name);
    REQUIRE(it != defaultClass.properties.end());
    CHECK(it->second.type == expected.type());
    CHECK(it->second.componentType == expected.componentType());

    checkNonArrayProperty<std::string, std::string_view>(
        gltf,
        propertyTable,
        defaultClass,
        expected.name,
        expected.type(),
        expected.componentType(),
        expected.values,
        expected.values.size());
  }

  struct ExpectedInt8Properties {
    std::string name;
    std::vector<int8_t> values;
    std::string type() const { return ClassProperty::Type::SCALAR; }
    std::optional<std::string> componentType() const {
      return ClassProperty::ComponentType::INT8;
    }
  };

  std::vector<ExpectedInt8Properties> expectedInt8Properties{
      {"wall_windows", {2, 4, 4, 2, 0, 3}},
      {"building_id", {0, 0, 1, 1, 2, 2}},
  };

  for (const auto& expected : expectedInt8Properties) {
    auto it = defaultClass.properties.find(expected.name);
    REQUIRE(it != defaultClass.properties.end());
    CHECK(it->second.type == expected.type());
    CHECK(it->second.componentType == expected.componentType());

    checkNonArrayProperty<int8_t>(
        gltf,
        propertyTable,
        defaultClass,
        expected.name,
        expected.type(),
        expected.componentType(),
        expected.values,
        expected.values.size());
  }

  struct ExpectedDoubleArrayProperties {
    std::string name;
    int64_t count;
    std::vector<std::vector<double>> values;
    std::string type() const { return ClassProperty::Type::SCALAR; }
    std::optional<std::string> componentType() const {
      return ClassProperty::ComponentType::FLOAT64;
    }
  };

  std::vector<ExpectedDoubleArrayProperties> expectedDoubleArrayProperties{
      {"block_lat_long",
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
    CHECK(it->second.type == expected.type());
    CHECK(it->second.componentType == expected.componentType());
    CHECK(it->second.array);
    CHECK(it->second.count == expected.count);

    checkArrayProperty<double>(
        gltf,
        propertyTable,
        defaultClass,
        expected.name,
        expected.count,
        expected.type(),
        expected.componentType(),
        expected.values,
        expected.values.size());
  }
}

TEST_CASE("3DTILES_batch_table_hierarchy with parentCounts is okay if all "
          "values are 1") {
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

  BatchTableToGltfStructuralMetadata::convertFromB3dm(
      featureTableParsed,
      batchTableParsed,
      std::span<const std::byte>(),
      gltf);

  // There should not be any log messages about parentCounts, since they're
  // all 1.
  std::vector<std::string> logMessages = pLog->last_formatted();
  REQUIRE(logMessages.size() == 0);

  // There should actually be metadata properties as normal.
  const ExtensionModelExtStructuralMetadata* pExtension =
      gltf.getExtension<ExtensionModelExtStructuralMetadata>();
  REQUIRE(pExtension);
  CHECK(
      gltf.isExtensionUsed(ExtensionModelExtStructuralMetadata::ExtensionName));

  // Check the schema
  REQUIRE(pExtension->schema);
  REQUIRE(pExtension->schema->classes.size() == 1);

  auto firstClassIt = pExtension->schema->classes.begin();
  CHECK(firstClassIt->first == "default");

  const Class& defaultClass = firstClassIt->second;
  REQUIRE(defaultClass.properties.size() == 3);

  // Check the property table
  REQUIRE(pExtension->propertyTables.size() == 1);
  const PropertyTable& propertyTable = pExtension->propertyTables[0];
  CHECK(propertyTable.classProperty == "default");
  REQUIRE(propertyTable.properties.size() == 3);
}

TEST_CASE("3DTILES_batch_table_hierarchy with parentCounts values != 1 is "
          "unsupported") {
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

  auto errors = BatchTableToGltfStructuralMetadata::convertFromB3dm(
      featureTableParsed,
      batchTableParsed,
      std::span<const std::byte>(),
      gltf);

  // There should be a log message about parentCounts, and no properties.
  const std::vector<std::string>& logMessages = errors.warnings;
  REQUIRE(logMessages.size() == 1);
  CHECK(logMessages[0].find("parentCounts") != std::string::npos);

  const ExtensionModelExtStructuralMetadata* pExtension =
      gltf.getExtension<ExtensionModelExtStructuralMetadata>();
  REQUIRE(pExtension);
  CHECK(
      gltf.isExtensionUsed(ExtensionModelExtStructuralMetadata::ExtensionName));

  // Check the schema
  REQUIRE(pExtension->schema);
  REQUIRE(pExtension->schema->classes.size() == 1);

  auto firstClassIt = pExtension->schema->classes.begin();
  CHECK(firstClassIt->first == "default");

  const Class& defaultClass = firstClassIt->second;
  REQUIRE(defaultClass.properties.size() == 0);

  // Check the property table
  REQUIRE(pExtension->propertyTables.size() == 1);
  const PropertyTable& propertyTable = pExtension->propertyTables[0];

  CHECK(propertyTable.classProperty == "default");
  REQUIRE(propertyTable.properties.size() == 0);
}
