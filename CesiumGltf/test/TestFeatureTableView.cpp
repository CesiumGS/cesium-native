#include "CesiumGltf/FeatureTableView.h"
#include "catch2/catch.hpp"

using namespace CesiumGltf;

TEST_CASE("Test numeric properties") {
  Model model;

  // store property value
  std::vector<uint32_t> values = {12, 34, 30, 11, 34, 34, 11, 33, 122, 33};
  Buffer& valueBuffer = model.buffers.emplace_back();
  valueBuffer.cesium.data.resize(values.size() * sizeof(uint32_t));
  valueBuffer.byteLength = static_cast<int64_t>(valueBuffer.cesium.data.size());
  std::memcpy(
      valueBuffer.cesium.data.data(),
      values.data(),
      valueBuffer.cesium.data.size());

  BufferView& valueBufferView = model.bufferViews.emplace_back();
  valueBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
  valueBufferView.byteOffset = 0;
  valueBufferView.byteLength = valueBuffer.byteLength;

  // setup metadata
  ModelEXT_feature_metadata& metadata =
      model.addExtension<ModelEXT_feature_metadata>();

  // setup schema
  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = "UINT32";

  // setup feature table
  FeatureTable& featureTable = metadata.featureTables["TestFeatureTable"];
  featureTable.classProperty = "TestClass";
  featureTable.count = static_cast<int64_t>(values.size());

  // setup feature table property
  FeatureTableProperty& featureTableProperty =
      featureTable.properties["TestClassProperty"];
  featureTableProperty.bufferView =
      static_cast<int32_t>(model.bufferViews.size() - 1);

  // test feature table view
  FeatureTableView view(&model, &featureTable);
  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty->type == "UINT32");
  REQUIRE(classProperty->componentCount == std::nullopt);
  REQUIRE(classProperty->componentType.isNull());

  SECTION("Access correct type") {
    std::optional<PropertyView<uint32_t>> uint32Property =
        view.getPropertyValues<uint32_t>("TestClassProperty");
    REQUIRE(uint32Property != std::nullopt);

    for (size_t i = 0; i < uint32Property->size(); ++i) {
      REQUIRE((*uint32Property)[i] == values[i]);
    }
  }

  SECTION("Access wrong type") {
    std::optional<PropertyView<bool>> boolInvalid =
        view.getPropertyValues<bool>("TestClassProperty");
    REQUIRE(boolInvalid == std::nullopt);

    std::optional<PropertyView<uint8_t>> uint8Invalid =
        view.getPropertyValues<uint8_t>("TestClassProperty");
    REQUIRE(uint8Invalid == std::nullopt);

    std::optional<PropertyView<int32_t>> int32Invalid =
        view.getPropertyValues<int32_t>("TestClassProperty");
    REQUIRE(int32Invalid == std::nullopt);

    std::optional<PropertyView<uint64_t>> unt64Invalid =
        view.getPropertyValues<uint64_t>("TestClassProperty");
    REQUIRE(unt64Invalid == std::nullopt);

    std::optional<PropertyView<std::string_view>> stringInvalid =
        view.getPropertyValues<std::string_view>("TestClassProperty");
    REQUIRE(stringInvalid == std::nullopt);

    std::optional<PropertyView<ArrayView<uint32_t>>> uint32ArrayInvalid =
        view.getPropertyValues<ArrayView<uint32_t>>("TestClassProperty");
    REQUIRE(uint32ArrayInvalid == std::nullopt);

    std::optional<PropertyView<ArrayView<bool>>> boolArrayInvalid =
        view.getPropertyValues<ArrayView<bool>>("TestClassProperty");
    REQUIRE(boolArrayInvalid == std::nullopt);

    std::optional<PropertyView<ArrayView<std::string_view>>>
        stringArrayInvalid =
            view.getPropertyValues<ArrayView<std::string_view>>(
                "TestClassProperty");
    REQUIRE(stringArrayInvalid == std::nullopt);
  }

  SECTION("Wrong buffer index") {
    valueBufferView.buffer = 2;
    std::optional<PropertyView<uint32_t>> uint32Property =
        view.getPropertyValues<uint32_t>("TestClassProperty");
    REQUIRE(uint32Property == std::nullopt);
  }

  SECTION("Wrong buffer view index") {}

  SECTION("Wrong buffer size") {}

  SECTION("Buffer view offset is not a multiple of 8") {}
}
