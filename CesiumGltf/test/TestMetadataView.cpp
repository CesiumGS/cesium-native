#include "CesiumGltf/MetadataView.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltf/ModelEXT_feature_metadata.h"
#include "catch2/catch.hpp"

TEST_CASE("Access primitive type") {
  CesiumGltf::Model model;
  CesiumGltf::ModelEXT_feature_metadata& metadata =
      model.addExtension<CesiumGltf::ModelEXT_feature_metadata>();
  metadata.schema = CesiumGltf::Schema();
  metadata.schema->name = "TestSchema";

  SECTION("int16_t") {
    // copy data to buffer
    std::vector<int16_t> data{21, 266, 3, 4, 122};
    CesiumGltf::Buffer& buffer = model.buffers.emplace_back();
    buffer.cesium.data.resize(data.size() * sizeof(int16_t));
    std::memcpy(
        buffer.cesium.data.data(),
        data.data(),
        data.size() * sizeof(int16_t));

    CesiumGltf::BufferView& bufferView = model.bufferViews.emplace_back();
    bufferView.buffer = static_cast<uint32_t>(model.buffers.size() - 1);
    bufferView.byteOffset = 0;
    bufferView.byteLength = buffer.cesium.data.size();

    // create schema
    CesiumGltf::Class& metaClass = metadata.schema->classes["Test"];
    CesiumGltf::ClassProperty& metaProperty =
        metaClass.properties["TestProperty"];
    metaProperty.type = "UINT16";

    // create feature table
    CesiumGltf::FeatureTable& featureTable = metadata.featureTables["Tests"];
    featureTable.count = data.size();

    // point feature table class to data
    featureTable.classProperty = "Test";
    CesiumGltf::FeatureTableProperty& featureTableProperty =
        featureTable.properties["TestProperty"];
    featureTableProperty.bufferView =
        static_cast<uint32_t>(model.bufferViews.size() - 1);

    auto propertyView = CesiumGltf::PropertyAccessorView::create(
        model,
        *metadata.schema,
        featureTable,
        "TestProperty");
    REQUIRE(propertyView != std::nullopt);
  }
}