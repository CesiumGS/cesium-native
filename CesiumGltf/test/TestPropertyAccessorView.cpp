#include "CesiumGltf/Model.h"
#include "CesiumGltf/ModelEXT_feature_metadata.h"
#include "CesiumGltf/PropertyAccessorView.h"
#include "CesiumGltf/PropertyType.h"
#include "catch2/catch.hpp"

template <typename T>
static void checkScalarProperty(
    const std::vector<T>& data,
    CesiumGltf::PropertyType propertyType,
    int64_t instanceOffset,
    int64_t instanceCount) {
  CesiumGltf::Model model;
  CesiumGltf::ModelEXT_feature_metadata& metadata =
      model.addExtension<CesiumGltf::ModelEXT_feature_metadata>();
  metadata.schema = CesiumGltf::Schema();
  metadata.schema->name = "TestSchema";

  // copy data to buffer
  CesiumGltf::Buffer& buffer = model.buffers.emplace_back();
  buffer.cesium.data.resize(data.size() * sizeof(T));
  std::memcpy(buffer.cesium.data.data(), data.data(), data.size() * sizeof(T));

  CesiumGltf::BufferView& bufferView = model.bufferViews.emplace_back();
  bufferView.buffer = static_cast<uint32_t>(model.buffers.size() - 1);
  bufferView.byteOffset = instanceOffset * sizeof(T);
  bufferView.byteLength = instanceCount * sizeof(T);

  // create schema
  CesiumGltf::Class& metaClass = metadata.schema->classes["Test"];
  CesiumGltf::ClassProperty& metaProperty =
      metaClass.properties["TestProperty"];
  metaProperty.type = CesiumGltf::convertProperttTypeToString(propertyType);

  // create feature table
  CesiumGltf::FeatureTable& featureTable = metadata.featureTables["Tests"];
  featureTable.count = instanceCount;

  // point feature table class to data
  featureTable.classProperty = "Test";
  CesiumGltf::FeatureTableProperty& featureTableProperty =
      featureTable.properties["TestProperty"];
  featureTableProperty.bufferView =
      static_cast<uint32_t>(model.bufferViews.size() - 1);

  // check values
  auto propertyView = CesiumGltf::PropertyAccessorView::create(
      model,
      metaProperty,
      featureTableProperty,
      featureTable.count);
  REQUIRE(propertyView != std::nullopt);
  REQUIRE(propertyView->getType() == static_cast<uint32_t>(propertyType));
  REQUIRE(propertyView->numOfInstances() == static_cast<size_t>(instanceCount));
  for (size_t i = 0; i < propertyView->numOfInstances(); ++i) {
    REQUIRE(propertyView->get<T>(i) == data[instanceOffset + i]);
  }
}

template <typename T>
static void checkFixedArray(
    const std::vector<T>& data,
    int64_t componentCount,
    CesiumGltf::PropertyType componentType,
    int64_t instanceCount) {
  CesiumGltf::Model model;
  CesiumGltf::ModelEXT_feature_metadata& metadata =
      model.addExtension<CesiumGltf::ModelEXT_feature_metadata>();
  metadata.schema = CesiumGltf::Schema();
  metadata.schema->name = "TestSchema";

  // create schema
  CesiumGltf::Class& metaClass = metadata.schema->classes["Test"];
  CesiumGltf::ClassProperty& metaProperty =
      metaClass.properties["TestProperty"];
  metaProperty.type =
      CesiumGltf::convertProperttTypeToString(CesiumGltf::PropertyType::Array);
  metaProperty.componentCount = componentCount;
  metaProperty.componentType =
      CesiumGltf::convertProperttTypeToString(componentType);

  // copy data to buffer
  CesiumGltf::Buffer& buffer = model.buffers.emplace_back();
  buffer.cesium.data.resize(data.size() * sizeof(T));
  std::memcpy(buffer.cesium.data.data(), data.data(), data.size() * sizeof(T));

  CesiumGltf::BufferView& bufferView = model.bufferViews.emplace_back();
  bufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
  bufferView.byteOffset = 0;
  bufferView.byteLength = buffer.cesium.data.size();

  // create feature table
  CesiumGltf::FeatureTable& featureTable = metadata.featureTables["Tests"];
  featureTable.count = instanceCount;

  // point feature table class to data
  featureTable.classProperty = "Test";
  CesiumGltf::FeatureTableProperty& featureTableProperty =
      featureTable.properties["TestProperty"];
  featureTableProperty.bufferView =
      static_cast<uint32_t>(model.bufferViews.size() - 1);

  // check values
  auto propertyView = CesiumGltf::PropertyAccessorView::create(
      model,
      metaProperty,
      featureTableProperty,
      featureTable.count);
  REQUIRE(propertyView != std::nullopt);
  REQUIRE(
      propertyView->getType() ==
      (static_cast<uint32_t>(CesiumGltf::PropertyType::Array) |
       static_cast<uint32_t>(componentType)));

  for (size_t i = 0; i < propertyView->numOfInstances(); ++i) {
    gsl::span<const T> val = propertyView->get<gsl::span<const T>>(i);
    REQUIRE(val.size() == static_cast<size_t>(componentCount));
    for (size_t j = 0; j < val.size(); ++j) {
      REQUIRE(val[j] == data[i * componentCount + j]);
    }
  }
}

template <typename T, typename E>
static void checkDynamicArray(
    const std::vector<T>& data,
    const std::vector<E>& offset,
    CesiumGltf::PropertyType componentType,
    int64_t instanceCount) {
  CesiumGltf::Model model;
  CesiumGltf::ModelEXT_feature_metadata& metadata =
      model.addExtension<CesiumGltf::ModelEXT_feature_metadata>();
  metadata.schema = CesiumGltf::Schema();
  metadata.schema->name = "TestSchema";

  // create schema
  CesiumGltf::Class& metaClass = metadata.schema->classes["Test"];
  CesiumGltf::ClassProperty& metaProperty =
      metaClass.properties["TestProperty"];
  metaProperty.type =
      CesiumGltf::convertProperttTypeToString(CesiumGltf::PropertyType::Array);
  metaProperty.componentType =
      CesiumGltf::convertProperttTypeToString(componentType);

  // copy data to buffer
  CesiumGltf::Buffer& buffer = model.buffers.emplace_back();
  buffer.cesium.data.resize(data.size() * sizeof(T));
  std::memcpy(buffer.cesium.data.data(), data.data(), data.size() * sizeof(T));

  CesiumGltf::BufferView& bufferView = model.bufferViews.emplace_back();
  bufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
  bufferView.byteOffset = 0;
  bufferView.byteLength = buffer.cesium.data.size();
  uint32_t bufferViewIdx = static_cast<uint32_t>(model.bufferViews.size() - 1);

  // copy offset to buffer
  CesiumGltf::Buffer& offsetBuffer = model.buffers.emplace_back();
  offsetBuffer.cesium.data.resize(offset.size() * sizeof(E));
  std::memcpy(offsetBuffer.cesium.data.data(), offset.data(), offset.size() * sizeof(E));

  CesiumGltf::BufferView& offsetBufferView = model.bufferViews.emplace_back();
  offsetBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
  offsetBufferView.byteOffset = 0;
  offsetBufferView.byteLength = offsetBuffer.cesium.data.size();
  uint32_t offsetBufferViewIdx = static_cast<uint32_t>(model.bufferViews.size() - 1);

  // create feature table
  CesiumGltf::FeatureTable& featureTable = metadata.featureTables["Tests"];
  featureTable.count = instanceCount;

  // point feature table class to data
  featureTable.classProperty = "Test";
  CesiumGltf::FeatureTableProperty& featureTableProperty =
      featureTable.properties["TestProperty"];
  featureTableProperty.bufferView = bufferViewIdx;
  featureTableProperty.arrayOffsetBufferView = offsetBufferViewIdx;

  // check values
  auto propertyView = CesiumGltf::PropertyAccessorView::create(
      model,
      metaProperty,
      featureTableProperty,
      featureTable.count);
  REQUIRE(propertyView != std::nullopt);
  REQUIRE(
      propertyView->getType() ==
      (static_cast<uint32_t>(CesiumGltf::PropertyType::Array) |
       static_cast<uint32_t>(componentType)));
  size_t expectedIdx = 0;
  for (size_t i = 0; i < propertyView->numOfInstances(); ++i) {
    gsl::span<const T> val = propertyView->get<gsl::span<const T>>(i);
    REQUIRE(val.size() == (offset[i + 1] - offset[i]) / sizeof(T));
    for (size_t j = 0; j < val.size(); ++j) {
      REQUIRE(val[j] == data[expectedIdx]);
      ++expectedIdx;
    }
  }
  REQUIRE(expectedIdx == data.size());
}

TEST_CASE("Access continuous scalar primitive type") {
  SECTION("uint8_t") {
    std::vector<uint8_t> data{21, 255, 3, 4, 122, 30, 11, 20};
    checkScalarProperty(data, CesiumGltf::PropertyType::Uint8, 1, 4);
  }

  SECTION("int8_t") {
    std::vector<int8_t> data{21, -122, -3, 12, -11};
    checkScalarProperty(data, CesiumGltf::PropertyType::Int8, 0, data.size());
  }

  SECTION("uint16_t") {
    std::vector<uint16_t> data{21, 266, 3, 4, 122};
    checkScalarProperty(
        data,
        CesiumGltf::PropertyType::Uint16,
        2,
        data.size() - 2);
  }

  SECTION("int16_t") {
    std::vector<int16_t> data{21, 26600, -3, 4222, -11122};
    checkScalarProperty(data, CesiumGltf::PropertyType::Int16, 0, data.size());
  }

  SECTION("uint32_t") {
    std::vector<uint32_t> data{2100, 266000, 3, 4, 122};
    checkScalarProperty(
        data,
        CesiumGltf::PropertyType::Uint32,
        2,
        data.size() - 2);
  }

  SECTION("int32_t") {
    std::vector<int32_t> data{210000, 26600, -3, 4222, -11122, 1, 5, 7, 9};
    checkScalarProperty(data, CesiumGltf::PropertyType::Int32, 2, 3);
  }

  SECTION("uint64_t") {
    std::vector<uint64_t> data{2100, 266000, 3, 4, 122};
    checkScalarProperty(data, CesiumGltf::PropertyType::Uint64, 0, data.size());
  }

  SECTION("int64_t") {
    std::vector<int64_t> data{210000, 26600, -3, 4222, -11122};
    checkScalarProperty(data, CesiumGltf::PropertyType::Int64, 0, data.size());
  }

  SECTION("Float32") {
    std::vector<float> data{21.5f, 26.622f, 3.14f, 4.4f, 122.3f};
    checkScalarProperty(
        data,
        CesiumGltf::PropertyType::Float32,
        0,
        data.size());
  }

  SECTION("Float64") {
    std::vector<double> data{221.5, 326, 622, 39.14, 43.4, 122.3};
    checkScalarProperty(
        data,
        CesiumGltf::PropertyType::Float64,
        0,
        data.size());
  }
}

TEST_CASE("Access fixed array") {
  SECTION("Fixed array of 4 uint8_ts") {
    // clang-format off
    std::vector<uint8_t> data{
        210, 211, 3, 42, 
        122, 22, 1, 45};
    // clang-format on
    checkFixedArray(data, 4, CesiumGltf::PropertyType::Uint8, data.size() / 4);
  }

  SECTION("Fixed array of 3 int8_ts") {
    // clang-format off
    std::vector<int8_t> data{
        122, -12, 3, 
        44, 11, -2, 
        5, 6, -22, 
        5, 6, 1};
    // clang-format on
    checkFixedArray(data, 3, CesiumGltf::PropertyType::Int8, data.size() / 3);
  }

  SECTION("Fixed array of 4 uint16_ts") {
    // clang-format off
    std::vector<uint16_t> data{
        122, 12, 3, 44, 
        11, 2, 5, 6000, 
        119, 30, 51, 200, 
        22000, 50000, 6000, 1};
    // clang-format on
    checkFixedArray(data, 4, CesiumGltf::PropertyType::Uint16, data.size() / 4);
  }

  SECTION("Fixed array of 4 int16_ts") {
    // clang-format off
    std::vector<int16_t> data{
        -122, 12, 3, 44, 
        11, 2, 5, -6000, 
        119, 30, 51, 200, 
        22000, -500, 6000, 1};
    // clang-format on
    checkFixedArray(data, 4, CesiumGltf::PropertyType::Int16, data.size() / 4);
  }

  SECTION("Fixed array of 6 uint32_ts") {
    // clang-format off
    std::vector<uint32_t> data{
        122, 12, 3, 44, 34444, 2222,
        11, 2, 5, 6000, 1111, 2222,
        119, 30, 51, 200, 12534, 11,
        22000, 500, 6000, 1, 3, 7};
    // clang-format on
    checkFixedArray(data, 6, CesiumGltf::PropertyType::Uint32, data.size() / 6);
  }

  SECTION("Fixed array of 2 int32_ts") {
    // clang-format off
    std::vector<uint32_t> data{
        122, 12, 
        3, 44};
    // clang-format on
    checkFixedArray(data, 2, CesiumGltf::PropertyType::Uint32, data.size() / 2);
  }

  SECTION("Fixed array of 4 uint64_ts") {
    // clang-format off
    std::vector<uint64_t> data{
        10022, 120000, 2422, 1111, 
        3, 440000, 333, 1455};
    // clang-format on
    checkFixedArray(data, 4, CesiumGltf::PropertyType::Uint64, data.size() / 4);
  }

  SECTION("Fixed array of 4 int64_ts") {
    // clang-format off
    std::vector<int64_t> data{
        10022, -120000, 2422, 1111, 
        3, 440000, -333, 1455};
    // clang-format on
    checkFixedArray(data, 4, CesiumGltf::PropertyType::Int64, data.size() / 4);
  }

  SECTION("Fixed array of 4 floats") {
    // clang-format off
    std::vector<float> data{
        10.022f, -12.43f, 242.2f, 1.111f, 
        3.333f, 440000.1f, -33.3f, 14.55f};
    // clang-format on
    checkFixedArray(
        data,
        4,
        CesiumGltf::PropertyType::Float32,
        data.size() / 4);
  }

  SECTION("Fixed array of 4 double") {
    // clang-format off
    std::vector<double> data{
        10.022, -12.43, 242.2, 1.111, 
        3.333, 440000.1, -33.3, 14.55};
    // clang-format on
    checkFixedArray(
        data,
        4,
        CesiumGltf::PropertyType::Float64,
        data.size() / 4);
  }
}

TEST_CASE("Access dynamic array") {
  SECTION("array of uint8_t") { 
    // clang-format off
    std::vector<uint8_t> data{
        3, 2,
        0, 45, 2, 1, 4,
        1, 3, 2,
        1, 3, 4, 1
    };
    std::vector<uint32_t> offset{
        0, 2, 7, 10, 14
    };
    // clang-format on

    checkDynamicArray(data, offset, CesiumGltf::PropertyType::Uint8, 4);
  }

  SECTION("array of int32_t") { 
    // clang-format off
    std::vector<int32_t> data{
        3, 200,
        0, 450, 200, 1, 4,
        1, 3, 2,
        1, 3, 4, 1
    };
    std::vector<uint32_t> offset{
        0, 2 * sizeof(int32_t), 7 * sizeof(int32_t), 10 * sizeof(int32_t), 14 * sizeof(int32_t)
    };
    // clang-format on

    checkDynamicArray(data, offset, CesiumGltf::PropertyType::Int32, 4);
  }

  SECTION("array of double") { 
    // clang-format off
    std::vector<double> data{
        3.333, 200.2,
        0.1122, 4.50, 2.30, 1.22, 4.444,
        1.4, 3.3, 2.2,
        1.11, 3.2, 4.111, 1.44
    };
    std::vector<uint32_t> offset{
        0, 2 * sizeof(double), 7 * sizeof(double), 10 * sizeof(double), 14 * sizeof(double)
    };
    // clang-format on

    checkDynamicArray(data, offset, CesiumGltf::PropertyType::Float64, 4);
  }
}

TEST_CASE("Access string") {
  CesiumGltf::Model model;
  CesiumGltf::ModelEXT_feature_metadata& metadata =
      model.addExtension<CesiumGltf::ModelEXT_feature_metadata>();
  metadata.schema = CesiumGltf::Schema();
  metadata.schema->name = "TestSchema";

  // create schema
  CesiumGltf::Class& metaClass = metadata.schema->classes["Test"];
  CesiumGltf::ClassProperty& metaProperty =
      metaClass.properties["TestProperty"];
  metaProperty.type =
      CesiumGltf::convertProperttTypeToString(CesiumGltf::PropertyType::String);

  // copy data to buffer
  CesiumGltf::Buffer& buffer = model.buffers.emplace_back();
  std::vector<std::string> strings {"This is a fine test", "What's going on", "Good morning"};
  size_t totalSize = 0;
  for (const auto& s : strings) {
    totalSize += s.size();
  }

  uint32_t currentOffset = 0;
  buffer.cesium.data.resize(totalSize);
  for (size_t i = 0; i < strings.size(); ++i) {
    std::memcpy(buffer.cesium.data.data() + currentOffset, strings[i].data(), strings[i].size());
    currentOffset += static_cast<uint32_t>(strings[i].size());
  }

  CesiumGltf::BufferView& bufferView = model.bufferViews.emplace_back();
  bufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
  bufferView.byteOffset = 0;
  bufferView.byteLength = buffer.cesium.data.size();
  uint32_t bufferViewIdx = static_cast<uint32_t>(model.bufferViews.size() - 1);

  // copy offset to buffer
  CesiumGltf::Buffer& offsetBuffer = model.buffers.emplace_back();
  offsetBuffer.cesium.data.resize((strings.size() + 1) * sizeof(uint32_t));
  currentOffset = 0;
  for (size_t i = 0; i < strings.size(); ++i) {
    std::memcpy(offsetBuffer.cesium.data.data() + i * sizeof(uint32_t), &currentOffset, sizeof(uint32_t));
    currentOffset += static_cast<uint32_t>(strings[i].size());
  }
  std::memcpy(offsetBuffer.cesium.data.data() + strings.size() * sizeof(uint32_t), &currentOffset, sizeof(uint32_t));

  CesiumGltf::BufferView& offsetBufferView = model.bufferViews.emplace_back();
  offsetBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
  offsetBufferView.byteOffset = 0;
  offsetBufferView.byteLength = buffer.cesium.data.size();
  uint32_t offsetBufferViewIdx = static_cast<uint32_t>(model.bufferViews.size() - 1);

  // create feature table
  CesiumGltf::FeatureTable& featureTable = metadata.featureTables["Tests"];
  featureTable.count = strings.size();

  // point feature table class to data
  featureTable.classProperty = "Test";
  CesiumGltf::FeatureTableProperty& featureTableProperty =
      featureTable.properties["TestProperty"];
  featureTableProperty.bufferView = bufferViewIdx;
  featureTableProperty.stringOffsetBufferView = offsetBufferViewIdx;

  // check values
  auto propertyView = CesiumGltf::PropertyAccessorView::create(
      model,
      metaProperty,
      featureTableProperty,
      featureTable.count);
  REQUIRE(propertyView != std::nullopt);
  for (size_t i = 0; i < propertyView->numOfInstances(); ++i) {
    REQUIRE(propertyView->get<std::string_view>(i) == strings[i]);
  }
}

TEST_CASE("Wrong format property") {
  CesiumGltf::Model model;
  CesiumGltf::ModelEXT_feature_metadata& metadata =
      model.addExtension<CesiumGltf::ModelEXT_feature_metadata>();
  metadata.schema = CesiumGltf::Schema();
  metadata.schema->name = "TestSchema";

  // create schema
  CesiumGltf::Class& metaClass = metadata.schema->classes["Test"];
  CesiumGltf::ClassProperty& metaProperty =
      metaClass.properties["TestProperty"];
  metaProperty.type =
      CesiumGltf::convertProperttTypeToString(CesiumGltf::PropertyType::Int64);

  SECTION("Buffer view points to wrong buffer index") {
    // copy data to buffer
    std::vector<int64_t> data{210000, 26600, -3, 4222, -11122};
    CesiumGltf::Buffer& buffer = model.buffers.emplace_back();
    buffer.cesium.data.resize(data.size() * sizeof(int64_t));
    std::memcpy(
        buffer.cesium.data.data(),
        data.data(),
        data.size() * sizeof(int64_t));

    // bufferView doesn't point to correct one
    CesiumGltf::BufferView& bufferView = model.bufferViews.emplace_back();
    bufferView.byteOffset = 0;
    bufferView.byteLength = buffer.cesium.data.size();

    // create feature table
    CesiumGltf::FeatureTable& featureTable = metadata.featureTables["Tests"];
    featureTable.count = data.size();

    // point feature table class to data
    featureTable.classProperty = "Test";
    CesiumGltf::FeatureTableProperty& featureTableProperty =
        featureTable.properties["TestProperty"];
    featureTableProperty.bufferView =
        static_cast<uint32_t>(model.bufferViews.size() - 1);

    // check values
    auto propertyView = CesiumGltf::PropertyAccessorView::create(
        model,
        metaProperty,
        featureTableProperty,
        featureTable.count);
    REQUIRE(propertyView == std::nullopt);
  }

  SECTION("Buffer view byte length outside of the real buffer length") {
    // copy data to buffer
    std::vector<int64_t> data{210000, 26600, -3, 4222, -11122};
    CesiumGltf::Buffer& buffer = model.buffers.emplace_back();
    buffer.cesium.data.resize(data.size() * sizeof(int64_t));
    std::memcpy(
        buffer.cesium.data.data(),
        data.data(),
        data.size() * sizeof(int64_t));

    // bufferView doesn't point to correct one
    CesiumGltf::BufferView& bufferView = model.bufferViews.emplace_back();
    bufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
    bufferView.byteOffset = sizeof(int64_t);
    bufferView.byteLength = buffer.cesium.data.size();

    // create feature table
    CesiumGltf::FeatureTable& featureTable = metadata.featureTables["Tests"];
    featureTable.count = data.size();

    // point feature table class to data
    featureTable.classProperty = "Test";
    CesiumGltf::FeatureTableProperty& featureTableProperty =
        featureTable.properties["TestProperty"];
    featureTableProperty.bufferView =
        static_cast<uint32_t>(model.bufferViews.size() - 1);

    // check values
    auto propertyView = CesiumGltf::PropertyAccessorView::create(
        model,
        metaProperty,
        featureTableProperty,
        featureTable.count);
    REQUIRE(propertyView == std::nullopt);
  }
}
