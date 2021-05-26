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
  bufferView.byteLength =
      (instanceCount - 1) * *bufferView.byteStride + sizeof(T);

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
