#include "CesiumGltf/MetadataFeatureTableView.h"

#include <catch2/catch.hpp>

#include <cstring>

using namespace CesiumGltf;

TEST_CASE("Test numeric properties") {
  Model model;

  // store property value
  std::vector<uint32_t> values = {12, 34, 30, 11, 34, 34, 11, 33, 122, 33};

  // Construct buffers in the scope to make sure that tests below doesn't use
  // the temp variables. Use index to access the buffer and buffer view instead
  size_t valueBufferIndex = 0;
  size_t valueBufferViewIndex = 0;
  {
    Buffer& valueBuffer = model.buffers.emplace_back();
    valueBuffer.cesium.data.resize(values.size() * sizeof(uint32_t));
    valueBuffer.byteLength =
        static_cast<int64_t>(valueBuffer.cesium.data.size());
    std::memcpy(
        valueBuffer.cesium.data.data(),
        values.data(),
        valueBuffer.cesium.data.size());
    valueBufferIndex = model.buffers.size() - 1;

    BufferView& valueBufferView = model.bufferViews.emplace_back();
    valueBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
    valueBufferView.byteOffset = 0;
    valueBufferView.byteLength = valueBuffer.byteLength;
    valueBufferViewIndex = model.bufferViews.size() - 1;
  }

  // setup metadata
  ExtensionModelExtFeatureMetadata& metadata =
      model.addExtension<ExtensionModelExtFeatureMetadata>();

  // setup schema
  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::UINT32;

  // setup feature table
  FeatureTable& featureTable = metadata.featureTables["TestFeatureTable"];
  featureTable.classProperty = "TestClass";
  featureTable.count = static_cast<int64_t>(values.size());

  // setup feature table property
  FeatureTableProperty& featureTableProperty =
      featureTable.properties["TestClassProperty"];
  featureTableProperty.bufferView = static_cast<int32_t>(valueBufferViewIndex);

  // test feature table view
  MetadataFeatureTableView view(&model, &featureTable);
  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty->type == ClassProperty::Type::UINT32);
  REQUIRE(classProperty->componentCount == std::nullopt);
  REQUIRE(classProperty->componentType == std::nullopt);

  SECTION("Access correct type") {
    MetadataPropertyView<uint32_t> uint32Property =
        view.getPropertyView<uint32_t>("TestClassProperty");
    REQUIRE(uint32Property.status() == MetadataPropertyViewStatus::Valid);
    REQUIRE(uint32Property.size() > 0);

    for (int64_t i = 0; i < uint32Property.size(); ++i) {
      REQUIRE(uint32Property.get(i) == values[static_cast<size_t>(i)]);
    }
  }

  SECTION("Access wrong type") {
    MetadataPropertyView<bool> boolInvalid =
        view.getPropertyView<bool>("TestClassProperty");
    REQUIRE(
        boolInvalid.status() ==
        MetadataPropertyViewStatus::InvalidTypeMismatch);

    MetadataPropertyView<uint8_t> uint8Invalid =
        view.getPropertyView<uint8_t>("TestClassProperty");
    REQUIRE(
        uint8Invalid.status() ==
        MetadataPropertyViewStatus::InvalidTypeMismatch);

    MetadataPropertyView<int32_t> int32Invalid =
        view.getPropertyView<int32_t>("TestClassProperty");
    REQUIRE(
        int32Invalid.status() ==
        MetadataPropertyViewStatus::InvalidTypeMismatch);

    MetadataPropertyView<uint64_t> unt64Invalid =
        view.getPropertyView<uint64_t>("TestClassProperty");
    REQUIRE(
        unt64Invalid.status() ==
        MetadataPropertyViewStatus::InvalidTypeMismatch);

    MetadataPropertyView<std::string_view> stringInvalid =
        view.getPropertyView<std::string_view>("TestClassProperty");
    REQUIRE(
        stringInvalid.status() ==
        MetadataPropertyViewStatus::InvalidTypeMismatch);

    MetadataPropertyView<MetadataArrayView<uint32_t>> uint32ArrayInvalid =
        view.getPropertyView<MetadataArrayView<uint32_t>>("TestClassProperty");
    REQUIRE(
        uint32ArrayInvalid.status() ==
        MetadataPropertyViewStatus::InvalidTypeMismatch);

    MetadataPropertyView<MetadataArrayView<bool>> boolArrayInvalid =
        view.getPropertyView<MetadataArrayView<bool>>("TestClassProperty");
    REQUIRE(
        boolArrayInvalid.status() ==
        MetadataPropertyViewStatus::InvalidTypeMismatch);

    MetadataPropertyView<MetadataArrayView<std::string_view>>
        stringArrayInvalid =
            view.getPropertyView<MetadataArrayView<std::string_view>>(
                "TestClassProperty");
    REQUIRE(
        stringArrayInvalid.status() ==
        MetadataPropertyViewStatus::InvalidTypeMismatch);
  }

  SECTION("Wrong buffer index") {
    model.bufferViews[valueBufferViewIndex].buffer = 2;
    MetadataPropertyView<uint32_t> uint32Property =
        view.getPropertyView<uint32_t>("TestClassProperty");
    REQUIRE(
        uint32Property.status() ==
        MetadataPropertyViewStatus::InvalidValueBufferIndex);
  }

  SECTION("Wrong buffer view index") {
    featureTableProperty.bufferView = -1;
    MetadataPropertyView<uint32_t> uint32Property =
        view.getPropertyView<uint32_t>("TestClassProperty");
    REQUIRE(
        uint32Property.status() ==
        MetadataPropertyViewStatus::InvalidValueBufferViewIndex);
  }

  SECTION("Buffer view points outside of the real buffer length") {
    model.buffers[valueBufferIndex].cesium.data.resize(12);
    MetadataPropertyView<uint32_t> uint32Property =
        view.getPropertyView<uint32_t>("TestClassProperty");
    REQUIRE(
        uint32Property.status() ==
        MetadataPropertyViewStatus::InvalidBufferViewOutOfBound);
  }

  // Even though the EXT_feature_metadata spec technically compels us to
  // enforce an 8-byte alignment, we avoid doing so for compatibility with
  // incorrect glTFs.
  /*
    SECTION("Buffer view offset is not a multiple of 8") {
      model.bufferViews[valueBufferViewIndex].byteOffset = 1;
      MetadataPropertyView<uint32_t> uint32Property =
          view.getPropertyView<uint32_t>("TestClassProperty");
      REQUIRE(
          uint32Property.status() ==
          MetadataPropertyViewStatus::InvalidBufferViewNotAligned8Bytes);
    }
    */

  SECTION("Buffer view length isn't multiple of sizeof(T)") {
    model.bufferViews[valueBufferViewIndex].byteLength = 13;
    MetadataPropertyView<uint32_t> uint32Property =
        view.getPropertyView<uint32_t>("TestClassProperty");
    REQUIRE(
        uint32Property.status() ==
        MetadataPropertyViewStatus::
            InvalidBufferViewSizeNotDivisibleByTypeSize);
  }

  SECTION("Buffer view length doesn't match with featureTableCount") {
    model.bufferViews[valueBufferViewIndex].byteLength = 12;
    MetadataPropertyView<uint32_t> uint32Property =
        view.getPropertyView<uint32_t>("TestClassProperty");
    REQUIRE(
        uint32Property.status() ==
        MetadataPropertyViewStatus::InvalidBufferViewSizeNotFitInstanceCount);
  }
}

TEST_CASE("Test boolean properties") {
  Model model;

  // store property value
  int64_t instanceCount = 21;
  std::vector<bool> expected;
  std::vector<uint8_t> values;
  values.resize(3);
  for (int64_t i = 0; i < instanceCount; ++i) {
    if (i % 2 == 0) {
      expected.emplace_back(true);
    } else {
      expected.emplace_back(false);
    }

    uint8_t expectedValue = expected.back();
    int64_t byteIndex = i / 8;
    int64_t bitIndex = i % 8;
    values[static_cast<size_t>(byteIndex)] = static_cast<uint8_t>(
        (expectedValue << bitIndex) | values[static_cast<size_t>(byteIndex)]);
  }

  // Create buffers in the scope, so that tests below don't accidentally refer
  // to temp variable. Use index instead if the buffer is needed
  {
    Buffer& valueBuffer = model.buffers.emplace_back();
    valueBuffer.cesium.data.resize(values.size());
    valueBuffer.byteLength =
        static_cast<int64_t>(valueBuffer.cesium.data.size());
    std::memcpy(
        valueBuffer.cesium.data.data(),
        values.data(),
        valueBuffer.cesium.data.size());

    BufferView& valueBufferView = model.bufferViews.emplace_back();
    valueBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
    valueBufferView.byteOffset = 0;
    valueBufferView.byteLength = valueBuffer.byteLength;
  }

  // setup metadata
  ExtensionModelExtFeatureMetadata& metadata =
      model.addExtension<ExtensionModelExtFeatureMetadata>();

  // setup schema
  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::BOOLEAN;

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
  MetadataFeatureTableView view(&model, &featureTable);
  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty->type == ClassProperty::Type::BOOLEAN);
  REQUIRE(classProperty->componentCount == std::nullopt);
  REQUIRE(classProperty->componentType == std::nullopt);

  SECTION("Access correct type") {
    MetadataPropertyView<bool> boolProperty =
        view.getPropertyView<bool>("TestClassProperty");
    REQUIRE(boolProperty.status() == MetadataPropertyViewStatus::Valid);
    REQUIRE(boolProperty.size() == instanceCount);
    for (int64_t i = 0; i < boolProperty.size(); ++i) {
      bool expectedValue = expected[static_cast<size_t>(i)];
      REQUIRE(boolProperty.get(i) == expectedValue);
    }
  }

  SECTION("Buffer size doesn't match with feature table count") {
    featureTable.count = 66;
    MetadataPropertyView<bool> boolProperty =
        view.getPropertyView<bool>("TestClassProperty");
    REQUIRE(
        boolProperty.status() ==
        MetadataPropertyViewStatus::InvalidBufferViewSizeNotFitInstanceCount);
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

  // Create buffers in the scope, so that tests below don't accidentally refer
  // to temp variable. Use index instead if the buffer is needed.
  // Store property value
  size_t valueBufferIndex = 0;
  size_t valueBufferViewIndex = 0;
  {
    Buffer& valueBuffer = model.buffers.emplace_back();
    valueBuffer.byteLength = static_cast<int64_t>(values.size());
    valueBuffer.cesium.data = std::move(values);
    valueBufferIndex = model.buffers.size() - 1;

    BufferView& valueBufferView = model.bufferViews.emplace_back();
    valueBufferView.buffer = static_cast<int32_t>(valueBufferIndex);
    valueBufferView.byteOffset = 0;
    valueBufferView.byteLength = valueBuffer.byteLength;
    valueBufferViewIndex = model.bufferViews.size() - 1;
  }

  // Create buffers in the scope, so that tests below don't accidentally refer
  // to temp variable. Use index instead if the buffer is needed.
  // Store string offset buffer
  size_t offsetBufferIndex = 0;
  size_t offsetBufferViewIndex = 0;
  {
    Buffer& offsetBuffer = model.buffers.emplace_back();
    offsetBuffer.byteLength = static_cast<int64_t>(offsets.size());
    offsetBuffer.cesium.data = std::move(offsets);
    offsetBufferIndex = model.buffers.size() - 1;

    BufferView& offsetBufferView = model.bufferViews.emplace_back();
    offsetBufferView.buffer = static_cast<int32_t>(offsetBufferIndex);
    offsetBufferView.byteOffset = 0;
    offsetBufferView.byteLength = offsetBuffer.byteLength;
    offsetBufferViewIndex = model.bufferViews.size() - 1;
  }

  // setup metadata
  ExtensionModelExtFeatureMetadata& metadata =
      model.addExtension<ExtensionModelExtFeatureMetadata>();

  // setup schema
  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::STRING;

  // setup feature table
  FeatureTable& featureTable = metadata.featureTables["TestFeatureTable"];
  featureTable.classProperty = "TestClass";
  featureTable.count = static_cast<int64_t>(expected.size());

  // setup feature table property
  FeatureTableProperty& featureTableProperty =
      featureTable.properties["TestClassProperty"];
  featureTableProperty.offsetType = FeatureTableProperty::OffsetType::UINT32;
  featureTableProperty.bufferView = static_cast<int32_t>(valueBufferViewIndex);
  featureTableProperty.stringOffsetBufferView =
      static_cast<int32_t>(offsetBufferViewIndex);

  // test feature table view
  MetadataFeatureTableView view(&model, &featureTable);
  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty->type == ClassProperty::Type::STRING);
  REQUIRE(classProperty->componentCount == std::nullopt);
  REQUIRE(classProperty->componentType == std::nullopt);

  SECTION("Access correct type") {
    MetadataPropertyView<std::string_view> stringProperty =
        view.getPropertyView<std::string_view>("TestClassProperty");
    REQUIRE(stringProperty.status() == MetadataPropertyViewStatus::Valid);
    for (size_t i = 0; i < expected.size(); ++i) {
      REQUIRE(stringProperty.get(static_cast<int64_t>(i)) == expected[i]);
    }
  }

  SECTION("Wrong offset type") {
    featureTableProperty.offsetType = FeatureTableProperty::OffsetType::UINT8;
    MetadataPropertyView<std::string_view> stringProperty =
        view.getPropertyView<std::string_view>("TestClassProperty");
    REQUIRE(
        stringProperty.status() ==
        MetadataPropertyViewStatus::InvalidBufferViewSizeNotFitInstanceCount);

    featureTableProperty.offsetType = FeatureTableProperty::OffsetType::UINT64;
    stringProperty =
        view.getPropertyView<std::string_view>("TestClassProperty");
    REQUIRE(
        stringProperty.status() ==
        MetadataPropertyViewStatus::InvalidBufferViewSizeNotFitInstanceCount);

    featureTableProperty.offsetType = "NONSENSE";
    stringProperty =
        view.getPropertyView<std::string_view>("TestClassProperty");
    REQUIRE(
        stringProperty.status() ==
        MetadataPropertyViewStatus::InvalidOffsetType);
  }

  SECTION("Offset values are not sorted ascending") {
    uint32_t* offset = reinterpret_cast<uint32_t*>(
        model.buffers[offsetBufferIndex].cesium.data.data());
    offset[2] =
        static_cast<uint32_t>(model.buffers[valueBufferIndex].byteLength + 4);
    MetadataPropertyView<std::string_view> stringProperty =
        view.getPropertyView<std::string_view>("TestClassProperty");
    REQUIRE(
        stringProperty.status() ==
        MetadataPropertyViewStatus::InvalidOffsetValuesNotSortedAscending);
  }

  SECTION("Offset value points outside of value buffer") {
    uint32_t* offset = reinterpret_cast<uint32_t*>(
        model.buffers[offsetBufferIndex].cesium.data.data());
    offset[featureTable.count] =
        static_cast<uint32_t>(model.buffers[valueBufferIndex].byteLength + 4);
    MetadataPropertyView<std::string_view> stringProperty =
        view.getPropertyView<std::string_view>("TestClassProperty");
    REQUIRE(
        stringProperty.status() ==
        MetadataPropertyViewStatus::InvalidOffsetValuePointsToOutOfBoundBuffer);
  }
}

TEST_CASE("Test fixed numeric array") {
  Model model;

  // store property value
  std::vector<uint32_t> values =
      {12, 34, 30, 11, 34, 34, 11, 33, 122, 33, 223, 11};

  size_t valueBufferViewIndex = 0;
  {
    Buffer& valueBuffer = model.buffers.emplace_back();
    valueBuffer.cesium.data.resize(values.size() * sizeof(uint32_t));
    valueBuffer.byteLength =
        static_cast<int64_t>(valueBuffer.cesium.data.size());
    std::memcpy(
        valueBuffer.cesium.data.data(),
        values.data(),
        valueBuffer.cesium.data.size());

    BufferView& valueBufferView = model.bufferViews.emplace_back();
    valueBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
    valueBufferView.byteOffset = 0;
    valueBufferView.byteLength = valueBuffer.byteLength;
    valueBufferViewIndex = model.bufferViews.size() - 1;
  }

  // setup metadata
  ExtensionModelExtFeatureMetadata& metadata =
      model.addExtension<ExtensionModelExtFeatureMetadata>();

  // setup schema
  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::ARRAY;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT32;
  testClassProperty.componentCount = 3;

  // setup feature table
  FeatureTable& featureTable = metadata.featureTables["TestFeatureTable"];
  featureTable.classProperty = "TestClass";
  featureTable.count = static_cast<int64_t>(
      values.size() /
      static_cast<size_t>(testClassProperty.componentCount.value()));

  // setup feature table property
  FeatureTableProperty& featureTableProperty =
      featureTable.properties["TestClassProperty"];
  featureTableProperty.bufferView =
      static_cast<int32_t>(model.bufferViews.size() - 1);

  // test feature table view
  MetadataFeatureTableView view(&model, &featureTable);
  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty->type == ClassProperty::Type::ARRAY);
  REQUIRE(classProperty->componentCount == 3);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT32);

  SECTION("Access the right type") {
    MetadataPropertyView<MetadataArrayView<uint32_t>> arrayProperty =
        view.getPropertyView<MetadataArrayView<uint32_t>>("TestClassProperty");
    REQUIRE(arrayProperty.status() == MetadataPropertyViewStatus::Valid);

    for (int64_t i = 0; i < arrayProperty.size(); ++i) {
      MetadataArrayView<uint32_t> member = arrayProperty.get(i);
      for (int64_t j = 0; j < member.size(); ++j) {
        REQUIRE(member[j] == values[static_cast<size_t>(i * 3 + j)]);
      }
    }
  }

  SECTION("Wrong component type") {
    testClassProperty.componentType = ClassProperty::ComponentType::UINT8;
    MetadataPropertyView<MetadataArrayView<uint32_t>> arrayProperty =
        view.getPropertyView<MetadataArrayView<uint32_t>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::InvalidTypeMismatch);
  }

  SECTION("Buffer size is not a multiple of type size") {
    model.bufferViews[valueBufferViewIndex].byteLength = 13;
    MetadataPropertyView<MetadataArrayView<uint32_t>> arrayProperty =
        view.getPropertyView<MetadataArrayView<uint32_t>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::
            InvalidBufferViewSizeNotDivisibleByTypeSize);
  }

  SECTION("Negative component count") {
    testClassProperty.componentCount = -1;
    MetadataPropertyView<MetadataArrayView<uint32_t>> arrayProperty =
        view.getPropertyView<MetadataArrayView<uint32_t>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::
            InvalidArrayComponentCountOrOffsetBufferNotExist);
  }

  SECTION("Value buffer doesn't fit into feature table count") {
    testClassProperty.componentCount = 55;
    MetadataPropertyView<MetadataArrayView<uint32_t>> arrayProperty =
        view.getPropertyView<MetadataArrayView<uint32_t>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::InvalidBufferViewSizeNotFitInstanceCount);
  }
}

TEST_CASE("Test dynamic numeric array") {
  Model model;

  // store property value
  std::vector<std::vector<uint16_t>> expected{
      {12, 33, 11, 344, 112, 444, 1},
      {},
      {},
      {122, 23, 333, 12},
      {},
      {333, 311, 22, 34},
      {},
      {33, 1888, 233, 33019}};
  size_t numOfElements = 0;
  for (const auto& expectedMember : expected) {
    numOfElements += expectedMember.size();
  }

  std::vector<std::byte> values(numOfElements * sizeof(uint16_t));
  std::vector<std::byte> offsets((expected.size() + 1) * sizeof(uint64_t));
  uint64_t* offsetValue = reinterpret_cast<uint64_t*>(offsets.data());
  for (size_t i = 0; i < expected.size(); ++i) {
    std::memcpy(
        values.data() + offsetValue[i],
        expected[i].data(),
        expected[i].size() * sizeof(uint16_t));
    offsetValue[i + 1] = offsetValue[i] + expected[i].size() * sizeof(uint16_t);
  }

  // store property value
  size_t valueBufferViewIndex = 0;
  {
    Buffer& valueBuffer = model.buffers.emplace_back();
    valueBuffer.byteLength = static_cast<int64_t>(values.size());
    valueBuffer.cesium.data = std::move(values);

    BufferView& valueBufferView = model.bufferViews.emplace_back();
    valueBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
    valueBufferView.byteOffset = 0;
    valueBufferView.byteLength = valueBuffer.byteLength;
    valueBufferViewIndex = model.bufferViews.size() - 1;
  }

  // store string offset buffer
  size_t offsetBufferViewIndex = 0;
  {
    Buffer& offsetBuffer = model.buffers.emplace_back();
    offsetBuffer.byteLength = static_cast<int64_t>(offsets.size());
    offsetBuffer.cesium.data = std::move(offsets);

    BufferView& offsetBufferView = model.bufferViews.emplace_back();
    offsetBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
    offsetBufferView.byteOffset = 0;
    offsetBufferView.byteLength = offsetBuffer.byteLength;
    offsetBufferViewIndex = model.bufferViews.size() - 1;
  }

  // setup metadata
  ExtensionModelExtFeatureMetadata& metadata =
      model.addExtension<ExtensionModelExtFeatureMetadata>();

  // setup schema
  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::ARRAY;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT16;

  // setup feature table
  FeatureTable& featureTable = metadata.featureTables["TestFeatureTable"];
  featureTable.classProperty = "TestClass";
  featureTable.count = static_cast<int64_t>(expected.size());

  // setup feature table property
  FeatureTableProperty& featureTableProperty =
      featureTable.properties["TestClassProperty"];
  featureTableProperty.bufferView = static_cast<int32_t>(valueBufferViewIndex);
  featureTableProperty.arrayOffsetBufferView =
      static_cast<int32_t>(offsetBufferViewIndex);
  featureTableProperty.offsetType = FeatureTableProperty::OffsetType::UINT64;

  // test feature table view
  MetadataFeatureTableView view(&model, &featureTable);
  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty->type == ClassProperty::Type::ARRAY);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT16);

  SECTION("Access the correct type") {
    MetadataPropertyView<MetadataArrayView<uint16_t>> property =
        view.getPropertyView<MetadataArrayView<uint16_t>>("TestClassProperty");
    REQUIRE(property.status() == MetadataPropertyViewStatus::Valid);
    for (size_t i = 0; i < expected.size(); ++i) {
      MetadataArrayView<uint16_t> valueMember =
          property.get(static_cast<int64_t>(i));
      REQUIRE(valueMember.size() == static_cast<int64_t>(expected[i].size()));
      for (size_t j = 0; j < expected[i].size(); ++j) {
        REQUIRE(expected[i][j] == valueMember[static_cast<int64_t>(j)]);
      }
    }
  }

  SECTION("Component count and offset buffer appear at the same time") {
    testClassProperty.componentCount = 3;
    MetadataPropertyView<MetadataArrayView<uint16_t>> property =
        view.getPropertyView<MetadataArrayView<uint16_t>>("TestClassProperty");
    REQUIRE(
        property.status() ==
        MetadataPropertyViewStatus::
            InvalidArrayComponentCountAndOffsetBufferCoexist);
  }
}

TEST_CASE("Test fixed boolean array") {
  Model model;

  // store property value
  std::vector<bool> expected = {
      true,
      false,
      false,
      true,
      false,
      false,
      true,
      true,
      true,
      false,
      false,
      true};
  std::vector<uint8_t> values;
  size_t requiredBytesSize = static_cast<size_t>(
      glm::ceil(static_cast<double>(expected.size()) / 8.0));
  values.resize(requiredBytesSize);
  for (size_t i = 0; i < expected.size(); ++i) {
    uint8_t expectedValue = expected[i];
    size_t byteIndex = i / 8;
    size_t bitIndex = i % 8;
    values[byteIndex] =
        static_cast<uint8_t>((expectedValue << bitIndex) | values[byteIndex]);
  }

  {
    Buffer& valueBuffer = model.buffers.emplace_back();
    valueBuffer.cesium.data.resize(values.size());
    valueBuffer.byteLength =
        static_cast<int64_t>(valueBuffer.cesium.data.size());
    std::memcpy(
        valueBuffer.cesium.data.data(),
        values.data(),
        valueBuffer.cesium.data.size());

    BufferView& valueBufferView = model.bufferViews.emplace_back();
    valueBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
    valueBufferView.byteOffset = 0;
    valueBufferView.byteLength = valueBuffer.byteLength;
  }

  // setup metadata
  ExtensionModelExtFeatureMetadata& metadata =
      model.addExtension<ExtensionModelExtFeatureMetadata>();

  // setup schema
  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::ARRAY;
  testClassProperty.componentType = ClassProperty::ComponentType::BOOLEAN;
  testClassProperty.componentCount = 3;

  // setup feature table
  FeatureTable& featureTable = metadata.featureTables["TestFeatureTable"];
  featureTable.classProperty = "TestClass";
  featureTable.count = static_cast<int64_t>(
      expected.size() /
      static_cast<size_t>(testClassProperty.componentCount.value()));

  // setup feature table property
  FeatureTableProperty& featureTableProperty =
      featureTable.properties["TestClassProperty"];
  featureTableProperty.bufferView =
      static_cast<int32_t>(model.bufferViews.size() - 1);

  // test feature table view
  MetadataFeatureTableView view(&model, &featureTable);
  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty->type == ClassProperty::Type::ARRAY);
  REQUIRE(classProperty->componentCount == 3);
  REQUIRE(
      classProperty->componentType == ClassProperty::ComponentType::BOOLEAN);

  SECTION("Access correct type") {
    MetadataPropertyView<MetadataArrayView<bool>> boolProperty =
        view.getPropertyView<MetadataArrayView<bool>>("TestClassProperty");
    REQUIRE(boolProperty.status() == MetadataPropertyViewStatus::Valid);
    REQUIRE(boolProperty.size() == featureTable.count);
    REQUIRE(boolProperty.size() > 0);
    for (int64_t i = 0; i < boolProperty.size(); ++i) {
      MetadataArrayView<bool> valueMember = boolProperty.get(i);
      for (int64_t j = 0; j < valueMember.size(); ++j) {
        REQUIRE(valueMember[j] == expected[static_cast<size_t>(i * 3 + j)]);
      }
    }
  }

  SECTION("Value buffer doesn't have enough required bytes") {
    testClassProperty.componentCount = 11;
    MetadataPropertyView<MetadataArrayView<bool>> boolProperty =
        view.getPropertyView<MetadataArrayView<bool>>("TestClassProperty");
    REQUIRE(
        boolProperty.status() ==
        MetadataPropertyViewStatus::InvalidBufferViewSizeNotFitInstanceCount);
  }

  SECTION("Component count is negative") {
    testClassProperty.componentCount = -1;
    MetadataPropertyView<MetadataArrayView<bool>> boolProperty =
        view.getPropertyView<MetadataArrayView<bool>>("TestClassProperty");
    REQUIRE(
        boolProperty.status() ==
        MetadataPropertyViewStatus::
            InvalidArrayComponentCountOrOffsetBufferNotExist);
  }
}

TEST_CASE("Test dynamic bool array") {
  Model model;

  // store property value
  std::vector<std::vector<bool>> expected{
      {true, false, true, true, false, true, true},
      {},
      {},
      {},
      {false, false, false, false},
      {true, false, true},
      {false},
      {true, true, true, true, true, false, false}};
  size_t numOfElements = 0;
  for (const auto& expectedMember : expected) {
    numOfElements += expectedMember.size();
  }

  size_t requiredBytesSize =
      static_cast<size_t>(glm::ceil(static_cast<double>(numOfElements) / 8.0));
  std::vector<std::byte> values(requiredBytesSize);
  std::vector<std::byte> offsets((expected.size() + 1) * sizeof(uint64_t));
  uint64_t* offsetValue = reinterpret_cast<uint64_t*>(offsets.data());
  size_t indexSoFar = 0;
  for (size_t i = 0; i < expected.size(); ++i) {
    for (size_t j = 0; j < expected[i].size(); ++j) {
      uint8_t expectedValue = expected[i][j];
      size_t byteIndex = indexSoFar / 8;
      size_t bitIndex = indexSoFar % 8;
      values[byteIndex] = static_cast<std::byte>(
          (expectedValue << bitIndex) | static_cast<int>(values[byteIndex]));
      ++indexSoFar;
    }
    offsetValue[i + 1] = offsetValue[i] + expected[i].size();
  }

  // store property value
  size_t valueBufferViewIndex = 0;
  {
    Buffer& valueBuffer = model.buffers.emplace_back();
    valueBuffer.byteLength = static_cast<int64_t>(values.size());
    valueBuffer.cesium.data = std::move(values);

    BufferView& valueBufferView = model.bufferViews.emplace_back();
    valueBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
    valueBufferView.byteOffset = 0;
    valueBufferView.byteLength = valueBuffer.byteLength;
    valueBufferViewIndex = model.bufferViews.size() - 1;
  }

  // store string offset buffer
  size_t offsetBufferViewIndex = 0;
  {
    Buffer& offsetBuffer = model.buffers.emplace_back();
    offsetBuffer.byteLength = static_cast<int64_t>(offsets.size());
    offsetBuffer.cesium.data = std::move(offsets);

    BufferView& offsetBufferView = model.bufferViews.emplace_back();
    offsetBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
    offsetBufferView.byteOffset = 0;
    offsetBufferView.byteLength = offsetBuffer.byteLength;
    offsetBufferViewIndex = model.bufferViews.size() - 1;
  }

  // setup metadata
  ExtensionModelExtFeatureMetadata& metadata =
      model.addExtension<ExtensionModelExtFeatureMetadata>();

  // setup schema
  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::ARRAY;
  testClassProperty.componentType = ClassProperty::ComponentType::BOOLEAN;

  // setup feature table
  FeatureTable& featureTable = metadata.featureTables["TestFeatureTable"];
  featureTable.classProperty = "TestClass";
  featureTable.count = static_cast<int64_t>(expected.size());

  // setup feature table property
  FeatureTableProperty& featureTableProperty =
      featureTable.properties["TestClassProperty"];
  featureTableProperty.bufferView = static_cast<int32_t>(valueBufferViewIndex);
  featureTableProperty.arrayOffsetBufferView =
      static_cast<int32_t>(offsetBufferViewIndex);
  featureTableProperty.offsetType = FeatureTableProperty::OffsetType::UINT64;

  // test feature table view
  MetadataFeatureTableView view(&model, &featureTable);
  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty->type == ClassProperty::Type::ARRAY);
  REQUIRE(
      classProperty->componentType == ClassProperty::ComponentType::BOOLEAN);

  SECTION("Access correct type") {
    MetadataPropertyView<MetadataArrayView<bool>> boolProperty =
        view.getPropertyView<MetadataArrayView<bool>>("TestClassProperty");
    REQUIRE(boolProperty.status() == MetadataPropertyViewStatus::Valid);
    for (size_t i = 0; i < expected.size(); ++i) {
      MetadataArrayView<bool> arrayMember =
          boolProperty.get(static_cast<int64_t>(i));
      REQUIRE(arrayMember.size() == static_cast<int64_t>(expected[i].size()));
      for (size_t j = 0; j < expected[i].size(); ++j) {
        REQUIRE(expected[i][j] == arrayMember[static_cast<int64_t>(j)]);
      }
    }
  }

  SECTION("Component count and array offset appear at the same time") {
    testClassProperty.componentCount = 3;
    MetadataPropertyView<MetadataArrayView<bool>> boolProperty =
        view.getPropertyView<MetadataArrayView<bool>>("TestClassProperty");
    REQUIRE(
        boolProperty.status() ==
        MetadataPropertyViewStatus::
            InvalidArrayComponentCountAndOffsetBufferCoexist);
  }
}

TEST_CASE("Test fixed array of string") {
  Model model;

  std::vector<std::string> expected{
      "What's up",
      "Breaking news!!! Aliens no longer attacks the US first",
      "But they still abduct my cows! Those milk thiefs! 👽 🐮",
      "I'm not crazy. My mother had me tested 🤪",
      "I love you, meat bags! ❤️",
      "Book in the freezer"};

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
  size_t valueBufferViewIndex = 0;
  {
    Buffer& valueBuffer = model.buffers.emplace_back();
    valueBuffer.byteLength = static_cast<int64_t>(values.size());
    valueBuffer.cesium.data = std::move(values);

    BufferView& valueBufferView = model.bufferViews.emplace_back();
    valueBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
    valueBufferView.byteOffset = 0;
    valueBufferView.byteLength = valueBuffer.byteLength;
    valueBufferViewIndex = model.bufferViews.size() - 1;
  }

  // store string offset buffer
  size_t offsetBufferViewIndex = 0;
  {
    Buffer& offsetBuffer = model.buffers.emplace_back();
    offsetBuffer.byteLength = static_cast<int64_t>(offsets.size());
    offsetBuffer.cesium.data = std::move(offsets);

    BufferView& offsetBufferView = model.bufferViews.emplace_back();
    offsetBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
    offsetBufferView.byteOffset = 0;
    offsetBufferView.byteLength = offsetBuffer.byteLength;
    offsetBufferViewIndex = model.bufferViews.size() - 1;
  }

  // setup metadata
  ExtensionModelExtFeatureMetadata& metadata =
      model.addExtension<ExtensionModelExtFeatureMetadata>();

  // setup schema
  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::ARRAY;
  testClassProperty.componentType = ClassProperty::ComponentType::STRING;
  testClassProperty.componentCount = 2;

  // setup feature table
  FeatureTable& featureTable = metadata.featureTables["TestFeatureTable"];
  featureTable.classProperty = "TestClass";
  featureTable.count = static_cast<int64_t>(
      expected.size() /
      static_cast<size_t>(testClassProperty.componentCount.value()));

  // setup feature table property
  FeatureTableProperty& featureTableProperty =
      featureTable.properties["TestClassProperty"];
  featureTableProperty.offsetType = FeatureTableProperty::OffsetType::UINT32;
  featureTableProperty.bufferView = static_cast<int32_t>(valueBufferViewIndex);
  featureTableProperty.stringOffsetBufferView =
      static_cast<int32_t>(offsetBufferViewIndex);

  // test feature table view
  MetadataFeatureTableView view(&model, &featureTable);
  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty->type == ClassProperty::Type::ARRAY);
  REQUIRE(classProperty->componentCount == 2);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::STRING);

  SECTION("Access correct type") {
    MetadataPropertyView<MetadataArrayView<std::string_view>> stringProperty =
        view.getPropertyView<MetadataArrayView<std::string_view>>(
            "TestClassProperty");
    REQUIRE(stringProperty.status() == MetadataPropertyViewStatus::Valid);
    REQUIRE(stringProperty.size() == 3);

    MetadataArrayView<std::string_view> v0 = stringProperty.get(0);
    REQUIRE(v0.size() == 2);
    REQUIRE(v0[0] == "What's up");
    REQUIRE(v0[1] == "Breaking news!!! Aliens no longer attacks the US first");

    MetadataArrayView<std::string_view> v1 = stringProperty.get(1);
    REQUIRE(v1.size() == 2);
    REQUIRE(v1[0] == "But they still abduct my cows! Those milk thiefs! 👽 🐮");
    REQUIRE(v1[1] == "I'm not crazy. My mother had me tested 🤪");

    MetadataArrayView<std::string_view> v2 = stringProperty.get(2);
    REQUIRE(v2.size() == 2);
    REQUIRE(v2[0] == "I love you, meat bags! ❤️");
    REQUIRE(v2[1] == "Book in the freezer");
  }

  SECTION("Component count is negative") {
    testClassProperty.componentCount = -1;
    MetadataPropertyView<MetadataArrayView<std::string_view>> stringProperty =
        view.getPropertyView<MetadataArrayView<std::string_view>>(
            "TestClassProperty");
    REQUIRE(
        stringProperty.status() ==
        MetadataPropertyViewStatus::
            InvalidArrayComponentCountOrOffsetBufferNotExist);
  }

  SECTION("Offset type is unknown") {
    featureTableProperty.offsetType = "NONSENSE";
    MetadataPropertyView<MetadataArrayView<std::string_view>> stringProperty =
        view.getPropertyView<MetadataArrayView<std::string_view>>(
            "TestClassProperty");
    REQUIRE(
        stringProperty.status() ==
        MetadataPropertyViewStatus::InvalidOffsetType);
  }

  SECTION("string offset buffer doesn't exist") {
    featureTableProperty.stringOffsetBufferView = -1;
    MetadataPropertyView<MetadataArrayView<std::string_view>> stringProperty =
        view.getPropertyView<MetadataArrayView<std::string_view>>(
            "TestClassProperty");
    REQUIRE(
        stringProperty.status() ==
        MetadataPropertyViewStatus::InvalidStringOffsetBufferViewIndex);
  }
}

TEST_CASE("Test dynamic array of string") {
  Model model;

  std::vector<std::vector<std::string>> expected{
      {"What's up"},
      {"Breaking news!!! Aliens no longer attacks the US first",
       "But they still abduct my cows! Those milk thiefs! 👽 🐮"},
      {"I'm not crazy. My mother had me tested 🤪",
       "I love you, meat bags! ❤️",
       "Book in the freezer"}};

  size_t totalBytes = 0;
  size_t numOfElements = 0;
  for (const auto& expectedValues : expected) {
    for (const auto& value : expectedValues) {
      totalBytes += value.size();
    }

    numOfElements += expectedValues.size();
  }

  std::vector<std::byte> offsets((expected.size() + 1) * sizeof(uint32_t));
  std::vector<std::byte> stringOffsets((numOfElements + 1) * sizeof(uint32_t));
  std::vector<std::byte> values(totalBytes);
  uint32_t* offsetValue = reinterpret_cast<uint32_t*>(offsets.data());
  uint32_t* stringOffsetValue =
      reinterpret_cast<uint32_t*>(stringOffsets.data());
  size_t strOffsetIdx = 0;
  for (size_t i = 0; i < expected.size(); ++i) {
    for (size_t j = 0; j < expected[i].size(); ++j) {
      const std::string& expectedValue = expected[i][j];
      std::memcpy(
          values.data() + stringOffsetValue[strOffsetIdx],
          expectedValue.c_str(),
          expectedValue.size());

      stringOffsetValue[strOffsetIdx + 1] =
          stringOffsetValue[strOffsetIdx] +
          static_cast<uint32_t>(expectedValue.size());
      ++strOffsetIdx;
    }

    offsetValue[i + 1] =
        offsetValue[i] +
        static_cast<uint32_t>(expected[i].size() * sizeof(uint32_t));
  }

  // store property value
  size_t valueBufferViewIndex = 0;
  {
    Buffer& valueBuffer = model.buffers.emplace_back();
    valueBuffer.byteLength = static_cast<int64_t>(values.size());
    valueBuffer.cesium.data = std::move(values);

    BufferView& valueBufferView = model.bufferViews.emplace_back();
    valueBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
    valueBufferView.byteOffset = 0;
    valueBufferView.byteLength = valueBuffer.byteLength;
    valueBufferViewIndex = model.bufferViews.size() - 1;
  }

  // store array offset buffer
  size_t offsetBufferViewIndex = 0;
  {
    Buffer& offsetBuffer = model.buffers.emplace_back();
    offsetBuffer.byteLength = static_cast<int64_t>(offsets.size());
    offsetBuffer.cesium.data = std::move(offsets);

    BufferView& offsetBufferView = model.bufferViews.emplace_back();
    offsetBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
    offsetBufferView.byteOffset = 0;
    offsetBufferView.byteLength = offsetBuffer.byteLength;
    offsetBufferViewIndex = model.bufferViews.size() - 1;
  }

  // store string offset buffer
  size_t strOffsetBufferViewIndex = 0;
  {
    Buffer& strOffsetBuffer = model.buffers.emplace_back();
    strOffsetBuffer.byteLength = static_cast<int64_t>(stringOffsets.size());
    strOffsetBuffer.cesium.data = std::move(stringOffsets);

    BufferView& strOffsetBufferView = model.bufferViews.emplace_back();
    strOffsetBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
    strOffsetBufferView.byteOffset = 0;
    strOffsetBufferView.byteLength = strOffsetBuffer.byteLength;
    strOffsetBufferViewIndex = model.bufferViews.size() - 1;
  }

  // setup metadata
  ExtensionModelExtFeatureMetadata& metadata =
      model.addExtension<ExtensionModelExtFeatureMetadata>();

  // setup schema
  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::ARRAY;
  testClassProperty.componentType = ClassProperty::ComponentType::STRING;

  // setup feature table
  FeatureTable& featureTable = metadata.featureTables["TestFeatureTable"];
  featureTable.classProperty = "TestClass";
  featureTable.count = static_cast<int64_t>(expected.size());

  // setup feature table property
  FeatureTableProperty& featureTableProperty =
      featureTable.properties["TestClassProperty"];
  featureTableProperty.offsetType = FeatureTableProperty::OffsetType::UINT32;
  featureTableProperty.bufferView = static_cast<int32_t>(valueBufferViewIndex);
  featureTableProperty.arrayOffsetBufferView =
      static_cast<int32_t>(offsetBufferViewIndex);
  featureTableProperty.stringOffsetBufferView =
      static_cast<int32_t>(strOffsetBufferViewIndex);

  // test feature table view
  MetadataFeatureTableView view(&model, &featureTable);
  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty->type == ClassProperty::Type::ARRAY);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::STRING);

  SECTION("Access correct type") {
    MetadataPropertyView<MetadataArrayView<std::string_view>> stringProperty =
        view.getPropertyView<MetadataArrayView<std::string_view>>(
            "TestClassProperty");
    REQUIRE(stringProperty.status() == MetadataPropertyViewStatus::Valid);
    for (size_t i = 0; i < expected.size(); ++i) {
      MetadataArrayView<std::string_view> stringArray =
          stringProperty.get(static_cast<int64_t>(i));
      for (size_t j = 0; j < expected[i].size(); ++j) {
        REQUIRE(stringArray[static_cast<int64_t>(j)] == expected[i][j]);
      }
    }
  }
}
