#include "CesiumGltf/FeatureTableView.h"
#include "catch2/catch.hpp"
#include <bitset>
#include <cstring>

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

  SECTION("Wrong buffer view index") {
    featureTableProperty.bufferView = -1;
    std::optional<PropertyView<uint32_t>> uint32Property =
        view.getPropertyValues<uint32_t>("TestClassProperty");
    REQUIRE(uint32Property == std::nullopt);
  }

  SECTION("Buffer view points outside of the real buffer length") {
    valueBuffer.cesium.data.resize(12);
    std::optional<PropertyView<uint32_t>> uint32Property =
        view.getPropertyValues<uint32_t>("TestClassProperty");
    REQUIRE(uint32Property == std::nullopt);
  }

  SECTION("Buffer view offset is not a multiple of 8") {
    valueBufferView.byteOffset = 1;
    std::optional<PropertyView<uint32_t>> uint32Property =
        view.getPropertyValues<uint32_t>("TestClassProperty");
    REQUIRE(uint32Property == std::nullopt);
  }

  SECTION("Buffer view length isn't multiple of sizeof(T)") {
    valueBufferView.byteLength = 13;
    std::optional<PropertyView<uint32_t>> uint32Property =
        view.getPropertyValues<uint32_t>("TestClassProperty");
    REQUIRE(uint32Property == std::nullopt);
  }

  SECTION("Buffer view length doesn't match with featureTableCount") {
    valueBufferView.byteLength = 12;
    std::optional<PropertyView<uint32_t>> uint32Property =
        view.getPropertyValues<uint32_t>("TestClassProperty");
    REQUIRE(uint32Property == std::nullopt);
  }
}

TEST_CASE("Test boolean properties") {
  Model model;

  // store property value
  uint64_t instanceCount = 21;
  std::vector<bool> expected;
  std::vector<uint8_t> values;
  values.resize(3);
  for (size_t i = 0; i < instanceCount; ++i) {
    if (i % 2 == 0) {
      expected.emplace_back(true);
    } else {
      expected.emplace_back(false);
    }

    uint8_t expectedValue = expected.back();
    size_t byteIndex = i / 8;
    size_t bitIndex = i % 8;
    values[byteIndex] =
        static_cast<uint8_t>((expectedValue << bitIndex) | values[byteIndex]);
  }

  Buffer& valueBuffer = model.buffers.emplace_back();
  valueBuffer.cesium.data.resize(values.size());
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
  testClassProperty.type = "BOOLEAN";

  // setup feature table
  FeatureTable& featureTable = metadata.featureTables["TestFeatureTable"];
  featureTable.classProperty = "TestClass";
  featureTable.count = static_cast<int64_t>(instanceCount);

  // setup feature table property
  FeatureTableProperty& featureTableProperty =
      featureTable.properties["TestClassProperty"];
  featureTableProperty.bufferView =
      static_cast<int32_t>(model.bufferViews.size() - 1);

  // test feature table view
  FeatureTableView view(&model, &featureTable);
  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty->type == "BOOLEAN");
  REQUIRE(classProperty->componentCount == std::nullopt);
  REQUIRE(classProperty->componentType.isNull());

  SECTION("Access correct type") {
    std::optional<PropertyView<bool>> boolProperty =
        view.getPropertyValues<bool>("TestClassProperty");
    REQUIRE(boolProperty != std::nullopt);
    REQUIRE(boolProperty->size() == instanceCount);
    for (size_t i = 0; i < boolProperty->size(); ++i) {
      bool expectedValue = expected[i];
      REQUIRE((*boolProperty)[i] == expectedValue);
    }
  }

  SECTION("Buffer size doesn't match with feature table count") {
    featureTable.count = 66;
    std::optional<PropertyView<bool>> boolProperty =
        view.getPropertyValues<bool>("TestClassProperty");
    REQUIRE(boolProperty == std::nullopt);
  }
}

TEST_CASE("Test string property") {
  Model model;

  std::vector<std::string> expected{"What's up", "Test_0", "Test_1", "", ""};
  size_t totalBytes = 0;
  for (const std::string& expectedValue : expected) {
    totalBytes += expectedValue.size();
  }

  std::vector<std::byte> offsets((expected.size() + 1) * sizeof(uint32_t));
  std::vector<std::byte> values(totalBytes);
  uint32_t* offsetValue = reinterpret_cast<uint32_t*>(offsets.data());
  for (size_t i = 0; i < expected.size(); ++i) {
    const std::string& expectedValue = expected[i];
    std::memcpy(
        values.data() + offsetValue[i],
        expectedValue.c_str(),
        expectedValue.size());
    offsetValue[i + 1] =
        offsetValue[i] + static_cast<uint32_t>(expectedValue.size());
  }

  // store property value
  Buffer& valueBuffer = model.buffers.emplace_back();
  valueBuffer.byteLength = static_cast<int64_t>(values.size());
  valueBuffer.cesium.data = std::move(values);

  BufferView& valueBufferView = model.bufferViews.emplace_back();
  valueBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
  valueBufferView.byteOffset = 0;
  valueBufferView.byteLength = valueBuffer.byteLength;
  int32_t valueBufferViewIdx =
      static_cast<int32_t>(model.bufferViews.size() - 1);

  // store string offset buffer
  Buffer& offsetBuffer = model.buffers.emplace_back();
  offsetBuffer.byteLength = static_cast<int64_t>(offsets.size());
  offsetBuffer.cesium.data = std::move(offsets);

  BufferView& offsetBufferView = model.bufferViews.emplace_back();
  offsetBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
  offsetBufferView.byteOffset = 0;
  offsetBufferView.byteLength = offsetBuffer.byteLength;
  int32_t offsetBufferViewIdx =
      static_cast<int32_t>(model.bufferViews.size() - 1);

  // setup metadata
  ModelEXT_feature_metadata& metadata =
      model.addExtension<ModelEXT_feature_metadata>();

  // setup schema
  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = "STRING";

  // setup feature table
  FeatureTable& featureTable = metadata.featureTables["TestFeatureTable"];
  featureTable.classProperty = "TestClass";
  featureTable.count = static_cast<int64_t>(expected.size());

  // setup feature table property
  FeatureTableProperty& featureTableProperty =
      featureTable.properties["TestClassProperty"];
  featureTableProperty.offsetType = "UINT32";
  featureTableProperty.bufferView = valueBufferViewIdx;
  featureTableProperty.stringOffsetBufferView = offsetBufferViewIdx;

  // test feature table view
  FeatureTableView view(&model, &featureTable);
  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty->type == "STRING");
  REQUIRE(classProperty->componentCount == std::nullopt);
  REQUIRE(classProperty->componentType.isNull());

  SECTION("Access correct type") {
    std::optional<PropertyView<std::string_view>> stringProperty =
        view.getPropertyValues<std::string_view>("TestClassProperty");
    REQUIRE(stringProperty != std::nullopt);
    for (size_t i = 0; i < expected.size(); ++i) {
      REQUIRE((*stringProperty)[i] == expected[i]);
    }
  }

  SECTION("Wrong offset type") {
    featureTableProperty.offsetType = "UINT8";
    std::optional<PropertyView<std::string_view>> stringProperty =
        view.getPropertyValues<std::string_view>("TestClassProperty");
    REQUIRE(stringProperty == std::nullopt);

    featureTableProperty.offsetType = "UINT64";
    stringProperty =
        view.getPropertyValues<std::string_view>("TestClassProperty");
    REQUIRE(stringProperty == std::nullopt);

    featureTableProperty.offsetType = "NONSENSE";
    stringProperty =
        view.getPropertyValues<std::string_view>("TestClassProperty");
    REQUIRE(stringProperty == std::nullopt);
  }

  SECTION("Offset values are not sorted ascending") {
    uint32_t* offset =
        reinterpret_cast<uint32_t*>(offsetBuffer.cesium.data.data());
    offset[2] = static_cast<uint32_t>(valueBuffer.byteLength + 4);
    std::optional<PropertyView<std::string_view>> stringProperty =
        view.getPropertyValues<std::string_view>("TestClassProperty");
    REQUIRE(stringProperty == std::nullopt);
  }

  SECTION("Offset value points outside of value buffer") {
    uint32_t* offset =
        reinterpret_cast<uint32_t*>(offsetBuffer.cesium.data.data());
    offset[featureTable.count] =
        static_cast<uint32_t>(valueBuffer.byteLength + 4);
    std::optional<PropertyView<std::string_view>> stringProperty =
        view.getPropertyValues<std::string_view>("TestClassProperty");
    REQUIRE(stringProperty == std::nullopt);
  }
}

TEST_CASE("Test fixed numeric array") {
  Model model;

  // store property value
  std::vector<uint32_t> values =
      {12, 34, 30, 11, 34, 34, 11, 33, 122, 33, 223, 11};
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
  testClassProperty.type = "ARRAY";
  testClassProperty.componentType = "UINT32";
  testClassProperty.componentCount = 3;

  // setup feature table
  FeatureTable& featureTable = metadata.featureTables["TestFeatureTable"];
  featureTable.classProperty = "TestClass";
  featureTable.count = static_cast<int64_t>(
      values.size() / testClassProperty.componentCount.value());

  // setup feature table property
  FeatureTableProperty& featureTableProperty =
      featureTable.properties["TestClassProperty"];
  featureTableProperty.bufferView =
      static_cast<int32_t>(model.bufferViews.size() - 1);

  // test feature table view
  FeatureTableView view(&model, &featureTable);
  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty->type == "ARRAY");
  REQUIRE(classProperty->componentCount == 3);
  REQUIRE(classProperty->componentType.getString() == "UINT32");

  SECTION("Access the right type") {
    std::optional<PropertyView<ArrayView<uint32_t>>> arrayProperty =
        view.getPropertyValues<ArrayView<uint32_t>>("TestClassProperty");
    REQUIRE(arrayProperty != std::nullopt);

    for (size_t i = 0; i < arrayProperty->size(); ++i) {
      ArrayView<uint32_t> member = (*arrayProperty)[i];
      for (size_t j = 0; j < member.size(); ++j) {
        REQUIRE(member[j] == values[i * 3 + j]);
      }
    }
  }

  SECTION("Wrong component type") {
    testClassProperty.componentType = "UINT8";
    std::optional<PropertyView<ArrayView<uint32_t>>> arrayProperty =
        view.getPropertyValues<ArrayView<uint32_t>>("TestClassProperty");
    REQUIRE(arrayProperty == std::nullopt);
  }

  SECTION("Buffer size is a multiple of type size") {
    valueBufferView.byteLength = 13;
    std::optional<PropertyView<ArrayView<uint32_t>>> arrayProperty =
        view.getPropertyValues<ArrayView<uint32_t>>("TestClassProperty");
    REQUIRE(arrayProperty == std::nullopt);
  }

  SECTION("Negative component count") {
    testClassProperty.componentCount = -1;
    std::optional<PropertyView<ArrayView<uint32_t>>> arrayProperty =
        view.getPropertyValues<ArrayView<uint32_t>>("TestClassProperty");
    REQUIRE(arrayProperty == std::nullopt);
  }

  SECTION("Value buffer doesn't fit into feature table count") {
    testClassProperty.componentCount = 55;
    std::optional<PropertyView<ArrayView<uint32_t>>> arrayProperty =
        view.getPropertyValues<ArrayView<uint32_t>>("TestClassProperty");
    REQUIRE(arrayProperty == std::nullopt);
  }
}
