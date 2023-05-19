#include "CesiumGltf/StructuralMetadataPropertyTableView.h"

#include <catch2/catch.hpp>

#include <cstring>

using namespace CesiumGltf;
using namespace CesiumGltf::StructuralMetadata;

TEST_CASE("Test StructuralMetadata scalar property") {
  Model model;

  std::vector<uint32_t> values = {12, 34, 30, 11, 34, 34, 11, 33, 122, 33};

  size_t valueBufferIndex = 0;
  size_t valueBufferViewIndex = 0;

  // Buffers are constructed in scope to ensure that the tests don't use their
  // temporary variables.
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

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  ExtensionExtStructuralMetadataSchema& schema = metadata.schema.emplace();
  ExtensionExtStructuralMetadataClass& testClass = schema.classes["TestClass"];
  ExtensionExtStructuralMetadataClassProperty& testClassProperty =
      testClass.properties["TestClassProperty"];
  testClassProperty.type =
      ExtensionExtStructuralMetadataClassProperty::Type::SCALAR;
  testClassProperty.componentType =
      ExtensionExtStructuralMetadataClassProperty::ComponentType::UINT32;

  ExtensionExtStructuralMetadataPropertyTable& propertyTable =
      metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(values.size());

  ExtensionExtStructuralMetadataPropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);

  MetadataPropertyTableView view(&model, &propertyTable);
  const ExtensionExtStructuralMetadataClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(
      classProperty->type ==
      ExtensionExtStructuralMetadataClassProperty::Type::SCALAR);
  REQUIRE(
      classProperty->componentType ==
      ExtensionExtStructuralMetadataClassProperty::ComponentType::UINT32);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);

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
    MetadataPropertyView<glm::uvec3> uvec3Invalid =
        view.getPropertyView<glm::uvec3>("TestClassProperty");
    REQUIRE(
        uvec3Invalid.status() == MetadataPropertyViewStatus::ErrorTypeMismatch);

    MetadataPropertyView<glm::u32mat3x3> u32mat3x3Invalid =
        view.getPropertyView<glm::u32mat3x3>("TestClassProperty");
    REQUIRE(
        u32mat3x3Invalid.status() ==
        MetadataPropertyViewStatus::ErrorTypeMismatch);

    MetadataPropertyView<bool> boolInvalid =
        view.getPropertyView<bool>("TestClassProperty");
    REQUIRE(
        boolInvalid.status() == MetadataPropertyViewStatus::ErrorTypeMismatch);

    MetadataPropertyView<std::string_view> stringInvalid =
        view.getPropertyView<std::string_view>("TestClassProperty");
    REQUIRE(
        stringInvalid.status() ==
        MetadataPropertyViewStatus::ErrorTypeMismatch);
  }

  SECTION("Access wrong component type") {
    MetadataPropertyView<uint8_t> uint8Invalid =
        view.getPropertyView<uint8_t>("TestClassProperty");
    REQUIRE(
        uint8Invalid.status() ==
        MetadataPropertyViewStatus::ErrorComponentTypeMismatch);

    MetadataPropertyView<int32_t> int32Invalid =
        view.getPropertyView<int32_t>("TestClassProperty");
    REQUIRE(
        int32Invalid.status() ==
        MetadataPropertyViewStatus::ErrorComponentTypeMismatch);

    MetadataPropertyView<uint64_t> uint64Invalid =
        view.getPropertyView<uint64_t>("TestClassProperty");
    REQUIRE(
        uint64Invalid.status() ==
        MetadataPropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SECTION("Access incorrectly as array") {
    MetadataPropertyView<MetadataArrayView<uint32_t>> uint32ArrayInvalid =
        view.getPropertyView<MetadataArrayView<uint32_t>>("TestClassProperty");
    REQUIRE(
        uint32ArrayInvalid.status() ==
        MetadataPropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SECTION("Wrong buffer index") {
    model.bufferViews[valueBufferViewIndex].buffer = 2;
    MetadataPropertyView<uint32_t> uint32Property =
        view.getPropertyView<uint32_t>("TestClassProperty");
    REQUIRE(
        uint32Property.status() ==
        MetadataPropertyViewStatus::ErrorInvalidValueBuffer);
  }

  SECTION("Wrong buffer view index") {
    propertyTableProperty.values = -1;
    MetadataPropertyView<uint32_t> uint32Property =
        view.getPropertyView<uint32_t>("TestClassProperty");
    REQUIRE(
        uint32Property.status() ==
        MetadataPropertyViewStatus::ErrorInvalidValueBufferView);
  }

  SECTION("Buffer view points outside of the real buffer length") {
    model.buffers[valueBufferIndex].cesium.data.resize(12);
    MetadataPropertyView<uint32_t> uint32Property =
        view.getPropertyView<uint32_t>("TestClassProperty");
    REQUIRE(
        uint32Property.status() ==
        MetadataPropertyViewStatus::ErrorBufferViewOutOfBounds);
  }

  SECTION("Buffer view length isn't multiple of sizeof(T)") {
    model.bufferViews[valueBufferViewIndex].byteLength = 13;
    MetadataPropertyView<uint32_t> uint32Property =
        view.getPropertyView<uint32_t>("TestClassProperty");
    REQUIRE(
        uint32Property.status() ==
        MetadataPropertyViewStatus::ErrorBufferViewSizeNotDivisibleByTypeSize);
  }

  SECTION("Buffer view length doesn't match with propertyTableCount") {
    model.bufferViews[valueBufferViewIndex].byteLength = 12;
    MetadataPropertyView<uint32_t> uint32Property =
        view.getPropertyView<uint32_t>("TestClassProperty");
    REQUIRE(
        uint32Property.status() ==
        MetadataPropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);
  }
}

TEST_CASE("Test StructuralMetadata vecN property") {
  Model model;

  std::vector<glm::ivec3> values = {
      glm::ivec3(-12, 34, 30),
      glm::ivec3(11, 73, 0),
      glm::ivec3(-2, 6, 12),
      glm::ivec3(-4, 8, -13)};

  size_t valueBufferIndex = 0;
  size_t valueBufferViewIndex = 0;

  // Buffers are constructed in scope to ensure that the tests don't use their
  // temporary variables.
  {
    Buffer& valueBuffer = model.buffers.emplace_back();
    valueBuffer.cesium.data.resize(values.size() * sizeof(glm::ivec3));
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

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  ExtensionExtStructuralMetadataSchema& schema = metadata.schema.emplace();
  ExtensionExtStructuralMetadataClass& testClass = schema.classes["TestClass"];
  ExtensionExtStructuralMetadataClassProperty& testClassProperty =
      testClass.properties["TestClassProperty"];
  testClassProperty.type =
      ExtensionExtStructuralMetadataClassProperty::Type::VEC3;
  testClassProperty.componentType =
      ExtensionExtStructuralMetadataClassProperty::ComponentType::INT32;

  ExtensionExtStructuralMetadataPropertyTable& propertyTable =
      metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(values.size());

  ExtensionExtStructuralMetadataPropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);

  MetadataPropertyTableView view(&model, &propertyTable);
  const ExtensionExtStructuralMetadataClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(
      classProperty->type ==
      ExtensionExtStructuralMetadataClassProperty::Type::VEC3);
  REQUIRE(
      classProperty->componentType ==
      ExtensionExtStructuralMetadataClassProperty::ComponentType::INT32);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);

  SECTION("Access correct type") {
    MetadataPropertyView<glm::ivec3> ivec3Property =
        view.getPropertyView<glm::ivec3>("TestClassProperty");
    REQUIRE(ivec3Property.status() == MetadataPropertyViewStatus::Valid);
    REQUIRE(ivec3Property.size() > 0);

    for (int64_t i = 0; i < ivec3Property.size(); ++i) {
      REQUIRE(ivec3Property.get(i) == values[i]);
    }
  }

  SECTION("Access wrong type") {
    MetadataPropertyView<int32_t> int32Invalid =
        view.getPropertyView<int32_t>("TestClassProperty");
    REQUIRE(
        int32Invalid.status() == MetadataPropertyViewStatus::ErrorTypeMismatch);

    MetadataPropertyView<glm::ivec2> ivec2Invalid =
        view.getPropertyView<glm::ivec2>("TestClassProperty");
    REQUIRE(
        ivec2Invalid.status() == MetadataPropertyViewStatus::ErrorTypeMismatch);

    MetadataPropertyView<glm::i32mat3x3> i32mat3x3Invalid =
        view.getPropertyView<glm::i32mat3x3>("TestClassProperty");
    REQUIRE(
        i32mat3x3Invalid.status() ==
        MetadataPropertyViewStatus::ErrorTypeMismatch);

    MetadataPropertyView<bool> boolInvalid =
        view.getPropertyView<bool>("TestClassProperty");
    REQUIRE(
        boolInvalid.status() == MetadataPropertyViewStatus::ErrorTypeMismatch);

    MetadataPropertyView<std::string_view> stringInvalid =
        view.getPropertyView<std::string_view>("TestClassProperty");
    REQUIRE(
        stringInvalid.status() ==
        MetadataPropertyViewStatus::ErrorTypeMismatch);
  }

  SECTION("Access wrong component type") {
    MetadataPropertyView<glm::u8vec3> u8vec3Invalid =
        view.getPropertyView<glm::u8vec3>("TestClassProperty");
    REQUIRE(
        u8vec3Invalid.status() ==
        MetadataPropertyViewStatus::ErrorComponentTypeMismatch);

    MetadataPropertyView<glm::i16vec3> i16vec3Invalid =
        view.getPropertyView<glm::i16vec3>("TestClassProperty");
    REQUIRE(
        i16vec3Invalid.status() ==
        MetadataPropertyViewStatus::ErrorComponentTypeMismatch);

    MetadataPropertyView<glm::vec3> vec3Invalid =
        view.getPropertyView<glm::vec3>("TestClassProperty");
    REQUIRE(
        vec3Invalid.status() ==
        MetadataPropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SECTION("Access incorrectly as array") {
    MetadataPropertyView<MetadataArrayView<glm::ivec3>> ivec3ArrayInvalid =
        view.getPropertyView<MetadataArrayView<glm::ivec3>>(
            "TestClassProperty");
    REQUIRE(
        ivec3ArrayInvalid.status() ==
        MetadataPropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SECTION("Wrong buffer index") {
    model.bufferViews[valueBufferViewIndex].buffer = 2;
    MetadataPropertyView<glm::ivec3> ivec3Property =
        view.getPropertyView<glm::ivec3>("TestClassProperty");
    REQUIRE(
        ivec3Property.status() ==
        MetadataPropertyViewStatus::ErrorInvalidValueBuffer);
  }

  SECTION("Wrong buffer view index") {
    propertyTableProperty.values = -1;
    MetadataPropertyView<glm::ivec3> ivec3Property =
        view.getPropertyView<glm::ivec3>("TestClassProperty");
    REQUIRE(
        ivec3Property.status() ==
        MetadataPropertyViewStatus::ErrorInvalidValueBufferView);
  }

  SECTION("Buffer view points outside of the real buffer length") {
    model.buffers[valueBufferIndex].cesium.data.resize(12);
    MetadataPropertyView<glm::ivec3> ivec3Property =
        view.getPropertyView<glm::ivec3>("TestClassProperty");
    REQUIRE(
        ivec3Property.status() ==
        MetadataPropertyViewStatus::ErrorBufferViewOutOfBounds);
  }

  SECTION("Buffer view length isn't multiple of sizeof(T)") {
    model.bufferViews[valueBufferViewIndex].byteLength = 11;
    MetadataPropertyView<glm::ivec3> ivec3Property =
        view.getPropertyView<glm::ivec3>("TestClassProperty");
    REQUIRE(
        ivec3Property.status() ==
        MetadataPropertyViewStatus::ErrorBufferViewSizeNotDivisibleByTypeSize);
  }

  SECTION("Buffer view length doesn't match with propertyTableCount") {
    model.bufferViews[valueBufferViewIndex].byteLength = 12;
    MetadataPropertyView<glm::ivec3> ivec3Property =
        view.getPropertyView<glm::ivec3>("TestClassProperty");
    REQUIRE(
        ivec3Property.status() ==
        MetadataPropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);
  }
}

TEST_CASE("Test StructuralMetadata matN property") {
  Model model;

  // clang-format off
  std::vector<glm::u32mat2x2> values = {
      glm::u32mat2x2(
        12, 34,
        30, 1),
      glm::u32mat2x2(
        11, 8,
        73, 102),
      glm::u32mat2x2(
        1, 0,
        63, 2),
      glm::u32mat2x2(
        4, 8,
        3, 23)};
  // clang-format on

  size_t valueBufferIndex = 0;
  size_t valueBufferViewIndex = 0;

  // Buffers are constructed in scope to ensure that the tests don't use their
  // temporary variables.
  {
    Buffer& valueBuffer = model.buffers.emplace_back();
    valueBuffer.cesium.data.resize(values.size() * sizeof(glm::u32mat2x2));
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

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  ExtensionExtStructuralMetadataSchema& schema = metadata.schema.emplace();
  ExtensionExtStructuralMetadataClass& testClass = schema.classes["TestClass"];
  ExtensionExtStructuralMetadataClassProperty& testClassProperty =
      testClass.properties["TestClassProperty"];
  testClassProperty.type =
      ExtensionExtStructuralMetadataClassProperty::Type::MAT2;
  testClassProperty.componentType =
      ExtensionExtStructuralMetadataClassProperty::ComponentType::UINT32;

  ExtensionExtStructuralMetadataPropertyTable& propertyTable =
      metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(values.size());

  ExtensionExtStructuralMetadataPropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);

  MetadataPropertyTableView view(&model, &propertyTable);
  const ExtensionExtStructuralMetadataClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(
      classProperty->type ==
      ExtensionExtStructuralMetadataClassProperty::Type::MAT2);
  REQUIRE(
      classProperty->componentType ==
      ExtensionExtStructuralMetadataClassProperty::ComponentType::UINT32);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);

  SECTION("Access correct type") {
    MetadataPropertyView<glm::u32mat2x2> u32mat2x2Property =
        view.getPropertyView<glm::u32mat2x2>("TestClassProperty");
    REQUIRE(u32mat2x2Property.status() == MetadataPropertyViewStatus::Valid);
    REQUIRE(u32mat2x2Property.size() > 0);

    for (int64_t i = 0; i < u32mat2x2Property.size(); ++i) {
      REQUIRE(u32mat2x2Property.get(i) == values[i]);
    }
  }

  SECTION("Access wrong type") {
    MetadataPropertyView<uint32_t> uint32Invalid =
        view.getPropertyView<uint32_t>("TestClassProperty");
    REQUIRE(
        uint32Invalid.status() ==
        MetadataPropertyViewStatus::ErrorTypeMismatch);

    MetadataPropertyView<glm::uvec2> uvec2Invalid =
        view.getPropertyView<glm::uvec2>("TestClassProperty");
    REQUIRE(
        uvec2Invalid.status() == MetadataPropertyViewStatus::ErrorTypeMismatch);

    MetadataPropertyView<glm::u32mat4x4> u32mat4x4Invalid =
        view.getPropertyView<glm::u32mat4x4>("TestClassProperty");
    REQUIRE(
        u32mat4x4Invalid.status() ==
        MetadataPropertyViewStatus::ErrorTypeMismatch);

    MetadataPropertyView<bool> boolInvalid =
        view.getPropertyView<bool>("TestClassProperty");
    REQUIRE(
        boolInvalid.status() == MetadataPropertyViewStatus::ErrorTypeMismatch);

    MetadataPropertyView<std::string_view> stringInvalid =
        view.getPropertyView<std::string_view>("TestClassProperty");
    REQUIRE(
        stringInvalid.status() ==
        MetadataPropertyViewStatus::ErrorTypeMismatch);
  }

  SECTION("Access wrong component type") {
    MetadataPropertyView<glm::u8mat2x2> u8mat2x2Invalid =
        view.getPropertyView<glm::u8mat2x2>("TestClassProperty");
    REQUIRE(
        u8mat2x2Invalid.status() ==
        MetadataPropertyViewStatus::ErrorComponentTypeMismatch);

    MetadataPropertyView<glm::i32mat2x2> i32mat2x2Invalid =
        view.getPropertyView<glm::i32mat2x2>("TestClassProperty");
    REQUIRE(
        i32mat2x2Invalid.status() ==
        MetadataPropertyViewStatus::ErrorComponentTypeMismatch);

    MetadataPropertyView<glm::mat2> mat2Invalid =
        view.getPropertyView<glm::mat2>("TestClassProperty");
    REQUIRE(
        mat2Invalid.status() ==
        MetadataPropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SECTION("Access incorrectly as array") {
    MetadataPropertyView<MetadataArrayView<glm::u32mat2x2>>
        u32mat2x2ArrayInvalid =
            view.getPropertyView<MetadataArrayView<glm::u32mat2x2>>(
                "TestClassProperty");
    REQUIRE(
        u32mat2x2ArrayInvalid.status() ==
        MetadataPropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SECTION("Wrong buffer index") {
    model.bufferViews[valueBufferViewIndex].buffer = 2;
    MetadataPropertyView<glm::u32mat2x2> u32mat2x2Property =
        view.getPropertyView<glm::u32mat2x2>("TestClassProperty");
    REQUIRE(
        u32mat2x2Property.status() ==
        MetadataPropertyViewStatus::ErrorInvalidValueBuffer);
  }

  SECTION("Wrong buffer view index") {
    propertyTableProperty.values = -1;
    MetadataPropertyView<glm::u32mat2x2> u32mat2x2Property =
        view.getPropertyView<glm::u32mat2x2>("TestClassProperty");
    REQUIRE(
        u32mat2x2Property.status() ==
        MetadataPropertyViewStatus::ErrorInvalidValueBufferView);
  }

  SECTION("Buffer view points outside of the real buffer length") {
    model.buffers[valueBufferIndex].cesium.data.resize(sizeof(glm::u32mat2x2));
    MetadataPropertyView<glm::u32mat2x2> u32mat2x2Property =
        view.getPropertyView<glm::u32mat2x2>("TestClassProperty");
    REQUIRE(
        u32mat2x2Property.status() ==
        MetadataPropertyViewStatus::ErrorBufferViewOutOfBounds);
  }

  SECTION("Buffer view length isn't multiple of sizeof(T)") {
    model.bufferViews[valueBufferViewIndex].byteLength =
        sizeof(glm::u32mat2x2) * 4 - 1;
    MetadataPropertyView<glm::u32mat2x2> u32mat2x2Property =
        view.getPropertyView<glm::u32mat2x2>("TestClassProperty");
    REQUIRE(
        u32mat2x2Property.status() ==
        MetadataPropertyViewStatus::ErrorBufferViewSizeNotDivisibleByTypeSize);
  }

  SECTION("Buffer view length doesn't match with propertyTableCount") {
    model.bufferViews[valueBufferViewIndex].byteLength = sizeof(glm::u32mat2x2);

    MetadataPropertyView<glm::u32mat2x2> u32mat2x2Property =
        view.getPropertyView<glm::u32mat2x2>("TestClassProperty");
    REQUIRE(
        u32mat2x2Property.status() ==
        MetadataPropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);
  }
}

TEST_CASE("Test StructuralMetadata boolean properties") {
  Model model;

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

  // Buffers are constructed in scope to ensure that the tests don't use their
  // temporary variables.
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

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  ExtensionExtStructuralMetadataSchema& schema = metadata.schema.emplace();
  ExtensionExtStructuralMetadataClass& testClass = schema.classes["TestClass"];
  ExtensionExtStructuralMetadataClassProperty& testClassProperty =
      testClass.properties["TestClassProperty"];
  testClassProperty.type =
      ExtensionExtStructuralMetadataClassProperty::Type::BOOLEAN;

  ExtensionExtStructuralMetadataPropertyTable& propertyTable =
      metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(instanceCount);

  ExtensionExtStructuralMetadataPropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values =
      static_cast<int32_t>(model.bufferViews.size() - 1);

  MetadataPropertyTableView view(&model, &propertyTable);
  const ExtensionExtStructuralMetadataClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(
      classProperty->type ==
      ExtensionExtStructuralMetadataClassProperty::Type::BOOLEAN);
  REQUIRE(classProperty->componentType == std::nullopt);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);

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

  SECTION("Buffer size doesn't match with propertyTableCount") {
    propertyTable.count = 66;
    MetadataPropertyView<bool> boolProperty =
        view.getPropertyView<bool>("TestClassProperty");
    REQUIRE(
        boolProperty.status() ==
        MetadataPropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);
  }
}

TEST_CASE("Test StructuralMetadata string property") {
  Model model;

  std::vector<std::string> expected{"What's up", "Test_0", "Test_1", "", "Hi"};
  size_t totalBytes = 0;
  for (const std::string& expectedValue : expected) {
    totalBytes += expectedValue.size();
  }

  std::vector<std::byte> stringOffsets(
      (expected.size() + 1) * sizeof(uint32_t));
  std::vector<std::byte> values(totalBytes);
  uint32_t* offsetValue = reinterpret_cast<uint32_t*>(stringOffsets.data());
  for (size_t i = 0; i < expected.size(); ++i) {
    const std::string& expectedValue = expected[i];
    std::memcpy(
        values.data() + offsetValue[i],
        expectedValue.c_str(),
        expectedValue.size());
    offsetValue[i + 1] =
        offsetValue[i] + static_cast<uint32_t>(expectedValue.size());
  }

  // Buffers are constructed in scope to ensure that the tests don't use their
  // temporary variables.
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

  size_t offsetBufferIndex = 0;
  size_t offsetBufferViewIndex = 0;
  {
    Buffer& offsetBuffer = model.buffers.emplace_back();
    offsetBuffer.byteLength = static_cast<int64_t>(stringOffsets.size());
    offsetBuffer.cesium.data = std::move(stringOffsets);
    offsetBufferIndex = model.buffers.size() - 1;

    BufferView& offsetBufferView = model.bufferViews.emplace_back();
    offsetBufferView.buffer = static_cast<int32_t>(offsetBufferIndex);
    offsetBufferView.byteOffset = 0;
    offsetBufferView.byteLength = offsetBuffer.byteLength;
    offsetBufferViewIndex = model.bufferViews.size() - 1;
  }

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  ExtensionExtStructuralMetadataSchema& schema = metadata.schema.emplace();
  ExtensionExtStructuralMetadataClass& testClass = schema.classes["TestClass"];
  ExtensionExtStructuralMetadataClassProperty& testClassProperty =
      testClass.properties["TestClassProperty"];
  testClassProperty.type =
      ExtensionExtStructuralMetadataClassProperty::Type::STRING;

  ExtensionExtStructuralMetadataPropertyTable& propertyTable =
      metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(expected.size());

  ExtensionExtStructuralMetadataPropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.stringOffsetType =
      ExtensionExtStructuralMetadataPropertyTableProperty::StringOffsetType::
          UINT32;
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);
  propertyTableProperty.stringOffsets =
      static_cast<int32_t>(offsetBufferViewIndex);

  MetadataPropertyTableView view(&model, &propertyTable);
  const ExtensionExtStructuralMetadataClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(
      classProperty->type ==
      ExtensionExtStructuralMetadataClassProperty::Type::STRING);
  REQUIRE(classProperty->componentType == std::nullopt);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);

  SECTION("Access correct type") {
    MetadataPropertyView<std::string_view> stringProperty =
        view.getPropertyView<std::string_view>("TestClassProperty");
    REQUIRE(stringProperty.status() == MetadataPropertyViewStatus::Valid);
    for (size_t i = 0; i < expected.size(); ++i) {
      REQUIRE(stringProperty.get(static_cast<int64_t>(i)) == expected[i]);
    }
  }

  SECTION("Wrong offset type") {
    propertyTableProperty.stringOffsetType =
        ExtensionExtStructuralMetadataPropertyTableProperty::StringOffsetType::
            UINT8;
    MetadataPropertyView<std::string_view> stringProperty =
        view.getPropertyView<std::string_view>("TestClassProperty");
    REQUIRE(
        stringProperty.status() ==
        MetadataPropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);

    propertyTableProperty.stringOffsetType =
        ExtensionExtStructuralMetadataPropertyTableProperty::StringOffsetType::
            UINT64;
    stringProperty =
        view.getPropertyView<std::string_view>("TestClassProperty");
    REQUIRE(
        stringProperty.status() ==
        MetadataPropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);

    propertyTableProperty.stringOffsetType = "NONSENSE";
    stringProperty =
        view.getPropertyView<std::string_view>("TestClassProperty");
    REQUIRE(
        stringProperty.status() ==
        MetadataPropertyViewStatus::ErrorInvalidStringOffsetType);

    propertyTableProperty.stringOffsetType = "";
    propertyTableProperty.arrayOffsetType =
        ExtensionExtStructuralMetadataPropertyTableProperty::StringOffsetType::
            UINT32;
    stringProperty =
        view.getPropertyView<std::string_view>("TestClassProperty");
    REQUIRE(
        stringProperty.status() ==
        MetadataPropertyViewStatus::ErrorInvalidStringOffsetType);
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
        MetadataPropertyViewStatus::ErrorStringOffsetsNotSorted);
  }

  SECTION("Offset value points outside of value buffer") {
    uint32_t* offset = reinterpret_cast<uint32_t*>(
        model.buffers[offsetBufferIndex].cesium.data.data());
    offset[propertyTable.count] =
        static_cast<uint32_t>(model.buffers[valueBufferIndex].byteLength + 4);
    MetadataPropertyView<std::string_view> stringProperty =
        view.getPropertyView<std::string_view>("TestClassProperty");
    REQUIRE(
        stringProperty.status() ==
        MetadataPropertyViewStatus::ErrorStringOffsetOutOfBounds);
  }
}

TEST_CASE("Test StructuralMetadata fixed-length scalar array") {
  Model model;

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

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  ExtensionExtStructuralMetadataSchema& schema = metadata.schema.emplace();
  ExtensionExtStructuralMetadataClass& testClass = schema.classes["TestClass"];
  ExtensionExtStructuralMetadataClassProperty& testClassProperty =
      testClass.properties["TestClassProperty"];
  testClassProperty.type =
      ExtensionExtStructuralMetadataClassProperty::Type::SCALAR;
  testClassProperty.componentType =
      ExtensionExtStructuralMetadataClassProperty::ComponentType::UINT32;
  testClassProperty.array = true;
  testClassProperty.count = 3;

  ExtensionExtStructuralMetadataPropertyTable& propertyTable =
      metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(
      values.size() / static_cast<size_t>(testClassProperty.count.value()));

  ExtensionExtStructuralMetadataPropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values =
      static_cast<int32_t>(model.bufferViews.size() - 1);

  MetadataPropertyTableView view(&model, &propertyTable);
  const ExtensionExtStructuralMetadataClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(
      classProperty->type ==
      ExtensionExtStructuralMetadataClassProperty::Type::SCALAR);
  REQUIRE(
      classProperty->componentType ==
      ExtensionExtStructuralMetadataClassProperty::ComponentType::UINT32);
  REQUIRE(classProperty->array);
  REQUIRE(classProperty->count == 3);

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

  SECTION("Wrong type") {
    MetadataPropertyView<MetadataArrayView<bool>> boolArrayInvalid =
        view.getPropertyView<MetadataArrayView<bool>>("TestClassProperty");
    REQUIRE(
        boolArrayInvalid.status() ==
        MetadataPropertyViewStatus::ErrorTypeMismatch);

    MetadataPropertyView<MetadataArrayView<glm::uvec2>> uvec2ArrayInvalid =
        view.getPropertyView<MetadataArrayView<glm::uvec2>>(
            "TestClassProperty");
    REQUIRE(
        uvec2ArrayInvalid.status() ==
        MetadataPropertyViewStatus::ErrorTypeMismatch);
  }

  SECTION("Wrong component type") {
    MetadataPropertyView<MetadataArrayView<int32_t>> int32ArrayInvalid =
        view.getPropertyView<MetadataArrayView<int32_t>>("TestClassProperty");
    REQUIRE(
        int32ArrayInvalid.status() ==
        MetadataPropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SECTION("Not an array type") {
    MetadataPropertyView<uint32_t> uint32Invalid =
        view.getPropertyView<uint32_t>("TestClassProperty");
    REQUIRE(
        uint32Invalid.status() ==
        MetadataPropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SECTION("Buffer size is not a multiple of type size") {
    model.bufferViews[valueBufferViewIndex].byteLength = 13;
    MetadataPropertyView<MetadataArrayView<uint32_t>> arrayProperty =
        view.getPropertyView<MetadataArrayView<uint32_t>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorBufferViewSizeNotDivisibleByTypeSize);
  }

  SECTION("Negative component count") {
    testClassProperty.count = -1;
    MetadataPropertyView<MetadataArrayView<uint32_t>> arrayProperty =
        view.getPropertyView<MetadataArrayView<uint32_t>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorArrayCountAndOffsetBufferDontExist);
  }

  SECTION("Value buffer doesn't fit into property table count") {
    testClassProperty.count = 55;
    MetadataPropertyView<MetadataArrayView<uint32_t>> arrayProperty =
        view.getPropertyView<MetadataArrayView<uint32_t>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);
  }
}

TEST_CASE("Test Structural Metadata variable-length scalar array") {
  Model model;

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

  size_t valueBufferIndex = 0;
  size_t valueBufferViewIndex = 0;
  {
    Buffer& valueBuffer = model.buffers.emplace_back();
    valueBuffer.byteLength = static_cast<int64_t>(values.size());
    valueBuffer.cesium.data = std::move(values);
    valueBufferIndex = model.buffers.size() - 1;

    BufferView& valueBufferView = model.bufferViews.emplace_back();
    valueBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
    valueBufferView.byteOffset = 0;
    valueBufferView.byteLength = valueBuffer.byteLength;
    valueBufferViewIndex = model.bufferViews.size() - 1;
  }

  size_t offsetBufferIndex = 0;
  size_t offsetBufferViewIndex = 0;
  {
    Buffer& offsetBuffer = model.buffers.emplace_back();
    offsetBuffer.byteLength = static_cast<int64_t>(offsets.size());
    offsetBuffer.cesium.data = std::move(offsets);
    offsetBufferIndex = model.buffers.size() - 1;

    BufferView& offsetBufferView = model.bufferViews.emplace_back();
    offsetBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
    offsetBufferView.byteOffset = 0;
    offsetBufferView.byteLength = offsetBuffer.byteLength;
    offsetBufferViewIndex = model.bufferViews.size() - 1;
  }

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  ExtensionExtStructuralMetadataSchema& schema = metadata.schema.emplace();
  ExtensionExtStructuralMetadataClass& testClass = schema.classes["TestClass"];
  ExtensionExtStructuralMetadataClassProperty& testClassProperty =
      testClass.properties["TestClassProperty"];
  testClassProperty.type =
      ExtensionExtStructuralMetadataClassProperty::Type::SCALAR;
  testClassProperty.componentType =
      ExtensionExtStructuralMetadataClassProperty::ComponentType::UINT16;
  testClassProperty.array = true;

  ExtensionExtStructuralMetadataPropertyTable& propertyTable =
      metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(expected.size());

  ExtensionExtStructuralMetadataPropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);
  propertyTableProperty.arrayOffsets =
      static_cast<int32_t>(offsetBufferViewIndex);
  propertyTableProperty.arrayOffsetType =
      ExtensionExtStructuralMetadataPropertyTableProperty::ArrayOffsetType::
          UINT64;

  MetadataPropertyTableView view(&model, &propertyTable);
  const ExtensionExtStructuralMetadataClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(
      classProperty->type ==
      ExtensionExtStructuralMetadataClassProperty::Type::SCALAR);
  REQUIRE(
      classProperty->componentType ==
      ExtensionExtStructuralMetadataClassProperty::ComponentType::UINT16);
  REQUIRE(classProperty->array);
  REQUIRE(!classProperty->count);

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
  SECTION("Wrong offset type") {
    propertyTableProperty.arrayOffsetType =
        ExtensionExtStructuralMetadataPropertyTableProperty::ArrayOffsetType::
            UINT8;
    MetadataPropertyView<MetadataArrayView<uint16_t>> arrayProperty =
        view.getPropertyView<MetadataArrayView<uint16_t>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);

    propertyTableProperty.arrayOffsetType =
        ExtensionExtStructuralMetadataPropertyTableProperty::ArrayOffsetType::
            UINT16;
    arrayProperty =
        view.getPropertyView<MetadataArrayView<uint16_t>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);

    propertyTableProperty.arrayOffsetType = "NONSENSE";
    arrayProperty =
        view.getPropertyView<MetadataArrayView<uint16_t>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorInvalidArrayOffsetType);

    propertyTableProperty.arrayOffsetType = "";
    propertyTableProperty.stringOffsetType =
        ExtensionExtStructuralMetadataPropertyTableProperty::StringOffsetType::
            UINT64;
    arrayProperty =
        view.getPropertyView<MetadataArrayView<uint16_t>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorInvalidArrayOffsetType);
  }

  SECTION("Offset values are not sorted ascending") {
    propertyTableProperty.arrayOffsetType =
        ExtensionExtStructuralMetadataPropertyTableProperty::ArrayOffsetType::
            UINT64;
    uint64_t* offset = reinterpret_cast<uint64_t*>(
        model.buffers[offsetBufferIndex].cesium.data.data());
    offset[propertyTable.count] = 0;
    MetadataPropertyView<MetadataArrayView<uint16_t>> arrayProperty =
        view.getPropertyView<MetadataArrayView<uint16_t>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorArrayOffsetsNotSorted);
  }

  SECTION("Offset value points outside of value buffer") {
    uint64_t* offset = reinterpret_cast<uint64_t*>(
        model.buffers[offsetBufferIndex].cesium.data.data());
    offset[propertyTable.count] =
        static_cast<uint32_t>(model.buffers[valueBufferIndex].byteLength + 4);
    MetadataPropertyView<MetadataArrayView<uint16_t>> arrayProperty =
        view.getPropertyView<MetadataArrayView<uint16_t>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorArrayOffsetOutOfBounds);
  }

  SECTION("Count and offset buffer are both present") {
    testClassProperty.count = 3;
    MetadataPropertyView<MetadataArrayView<uint16_t>> property =
        view.getPropertyView<MetadataArrayView<uint16_t>>("TestClassProperty");
    REQUIRE(
        property.status() ==
        MetadataPropertyViewStatus::ErrorArrayCountAndOffsetBufferCoexist);
  }
}

TEST_CASE("Test StructuralMetadata fixed-length vecN array") {
  Model model;

  std::vector<glm::ivec3> values = {
      glm::ivec3(12, 34, -30),
      glm::ivec3(-2, 0, 1),
      glm::ivec3(1, 2, 8),
      glm::ivec3(-100, 84, 6),
      glm::ivec3(2, -2, -2),
      glm::ivec3(40, 61, 3),
  };

  size_t valueBufferViewIndex = 0;
  {
    Buffer& valueBuffer = model.buffers.emplace_back();
    valueBuffer.cesium.data.resize(values.size() * sizeof(glm::ivec3));
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

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  ExtensionExtStructuralMetadataSchema& schema = metadata.schema.emplace();
  ExtensionExtStructuralMetadataClass& testClass = schema.classes["TestClass"];
  ExtensionExtStructuralMetadataClassProperty& testClassProperty =
      testClass.properties["TestClassProperty"];
  testClassProperty.type =
      ExtensionExtStructuralMetadataClassProperty::Type::VEC3;
  testClassProperty.componentType =
      ExtensionExtStructuralMetadataClassProperty::ComponentType::INT32;
  testClassProperty.array = true;
  testClassProperty.count = 2;

  ExtensionExtStructuralMetadataPropertyTable& propertyTable =
      metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(
      values.size() / static_cast<size_t>(testClassProperty.count.value()));

  ExtensionExtStructuralMetadataPropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values =
      static_cast<int32_t>(model.bufferViews.size() - 1);

  MetadataPropertyTableView view(&model, &propertyTable);
  const ExtensionExtStructuralMetadataClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(
      classProperty->type ==
      ExtensionExtStructuralMetadataClassProperty::Type::VEC3);
  REQUIRE(
      classProperty->componentType ==
      ExtensionExtStructuralMetadataClassProperty::ComponentType::INT32);
  REQUIRE(classProperty->array);
  REQUIRE(classProperty->count == 2);

  SECTION("Access the right type") {
    MetadataPropertyView<MetadataArrayView<glm::ivec3>> arrayProperty =
        view.getPropertyView<MetadataArrayView<glm::ivec3>>(
            "TestClassProperty");
    REQUIRE(arrayProperty.status() == MetadataPropertyViewStatus::Valid);

    for (int64_t i = 0; i < arrayProperty.size(); ++i) {
      MetadataArrayView<glm::ivec3> member = arrayProperty.get(i);
      for (int64_t j = 0; j < member.size(); ++j) {
        REQUIRE(member[j] == values[static_cast<size_t>(i * 2 + j)]);
      }
    }
  }

  SECTION("Wrong type") {
    MetadataPropertyView<MetadataArrayView<int32_t>> int32ArrayInvalid =
        view.getPropertyView<MetadataArrayView<int32_t>>("TestClassProperty");
    REQUIRE(
        int32ArrayInvalid.status() ==
        MetadataPropertyViewStatus::ErrorTypeMismatch);

    MetadataPropertyView<MetadataArrayView<glm::ivec2>> ivec2ArrayInvalid =
        view.getPropertyView<MetadataArrayView<glm::ivec2>>(
            "TestClassProperty");
    REQUIRE(
        ivec2ArrayInvalid.status() ==
        MetadataPropertyViewStatus::ErrorTypeMismatch);
  }

  SECTION("Wrong component type") {
    MetadataPropertyView<MetadataArrayView<glm::uvec3>> uvec3ArrayInvalid =
        view.getPropertyView<MetadataArrayView<glm::uvec3>>(
            "TestClassProperty");
    REQUIRE(
        uvec3ArrayInvalid.status() ==
        MetadataPropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SECTION("Not an array type") {
    MetadataPropertyView<glm::ivec3> ivec3Invalid =
        view.getPropertyView<glm::ivec3>("TestClassProperty");
    REQUIRE(
        ivec3Invalid.status() ==
        MetadataPropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SECTION("Buffer size is not a multiple of type size") {
    model.bufferViews[valueBufferViewIndex].byteLength = 13;
    MetadataPropertyView<MetadataArrayView<glm::ivec3>> arrayProperty =
        view.getPropertyView<MetadataArrayView<glm::ivec3>>(
            "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorBufferViewSizeNotDivisibleByTypeSize);
  }

  SECTION("Negative component count") {
    testClassProperty.count = -1;
    MetadataPropertyView<MetadataArrayView<glm::ivec3>> arrayProperty =
        view.getPropertyView<MetadataArrayView<glm::ivec3>>(
            "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorArrayCountAndOffsetBufferDontExist);
  }

  SECTION("Value buffer doesn't fit into property table count") {
    testClassProperty.count = 55;
    MetadataPropertyView<MetadataArrayView<glm::ivec3>> arrayProperty =
        view.getPropertyView<MetadataArrayView<glm::ivec3>>(
            "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);
  }
}

TEST_CASE("Test Structural Metadata variable-length vecN array") {
  Model model;

  // clang-format off
  std::vector<std::vector<glm::ivec3>> expected{
      { glm::ivec3(12, 34, -30), glm::ivec3(-2, 0, 1) },
      { glm::ivec3(1, 2, 8), },
      {},
      { glm::ivec3(-100, 84, 6), glm::ivec3(2, -2, -2), glm::ivec3(40, 61, 3) },
      { glm::ivec3(-1, 4, -7) },
  };
  // clang-format on

  size_t numOfElements = 0;
  for (const auto& expectedMember : expected) {
    numOfElements += expectedMember.size();
  }

  std::vector<std::byte> values(numOfElements * sizeof(glm::ivec3));
  std::vector<std::byte> offsets((expected.size() + 1) * sizeof(uint64_t));
  uint64_t* offsetValue = reinterpret_cast<uint64_t*>(offsets.data());
  for (size_t i = 0; i < expected.size(); ++i) {
    std::memcpy(
        values.data() + offsetValue[i],
        expected[i].data(),
        expected[i].size() * sizeof(glm::ivec3));
    offsetValue[i + 1] =
        offsetValue[i] + expected[i].size() * sizeof(glm::ivec3);
  }

  size_t valueBufferIndex = 0;
  size_t valueBufferViewIndex = 0;
  {
    Buffer& valueBuffer = model.buffers.emplace_back();
    valueBuffer.byteLength = static_cast<int64_t>(values.size());
    valueBuffer.cesium.data = std::move(values);
    valueBufferIndex = model.buffers.size() - 1;

    BufferView& valueBufferView = model.bufferViews.emplace_back();
    valueBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
    valueBufferView.byteOffset = 0;
    valueBufferView.byteLength = valueBuffer.byteLength;
    valueBufferViewIndex = model.bufferViews.size() - 1;
  }

  size_t offsetBufferIndex = 0;
  size_t offsetBufferViewIndex = 0;
  {
    Buffer& offsetBuffer = model.buffers.emplace_back();
    offsetBuffer.byteLength = static_cast<int64_t>(offsets.size());
    offsetBuffer.cesium.data = std::move(offsets);
    offsetBufferIndex = model.buffers.size() - 1;

    BufferView& offsetBufferView = model.bufferViews.emplace_back();
    offsetBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
    offsetBufferView.byteOffset = 0;
    offsetBufferView.byteLength = offsetBuffer.byteLength;
    offsetBufferViewIndex = model.bufferViews.size() - 1;
  }

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  ExtensionExtStructuralMetadataSchema& schema = metadata.schema.emplace();
  ExtensionExtStructuralMetadataClass& testClass = schema.classes["TestClass"];
  ExtensionExtStructuralMetadataClassProperty& testClassProperty =
      testClass.properties["TestClassProperty"];
  testClassProperty.type =
      ExtensionExtStructuralMetadataClassProperty::Type::VEC3;
  testClassProperty.componentType =
      ExtensionExtStructuralMetadataClassProperty::ComponentType::INT32;
  testClassProperty.array = true;

  ExtensionExtStructuralMetadataPropertyTable& propertyTable =
      metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(expected.size());

  ExtensionExtStructuralMetadataPropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);
  propertyTableProperty.arrayOffsets =
      static_cast<int32_t>(offsetBufferViewIndex);
  propertyTableProperty.arrayOffsetType =
      ExtensionExtStructuralMetadataPropertyTableProperty::ArrayOffsetType::
          UINT64;

  MetadataPropertyTableView view(&model, &propertyTable);
  const ExtensionExtStructuralMetadataClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(
      classProperty->type ==
      ExtensionExtStructuralMetadataClassProperty::Type::VEC3);
  REQUIRE(
      classProperty->componentType ==
      ExtensionExtStructuralMetadataClassProperty::ComponentType::INT32);
  REQUIRE(classProperty->array);
  REQUIRE(!classProperty->count);

  SECTION("Access the correct type") {
    MetadataPropertyView<MetadataArrayView<glm::ivec3>> property =
        view.getPropertyView<MetadataArrayView<glm::ivec3>>(
            "TestClassProperty");
    REQUIRE(property.status() == MetadataPropertyViewStatus::Valid);
    for (size_t i = 0; i < expected.size(); ++i) {
      MetadataArrayView<glm::ivec3> valueMember =
          property.get(static_cast<int64_t>(i));
      REQUIRE(valueMember.size() == static_cast<int64_t>(expected[i].size()));
      for (size_t j = 0; j < expected[i].size(); ++j) {
        REQUIRE(expected[i][j] == valueMember[static_cast<int64_t>(j)]);
      }
    }
  }

  SECTION("Wrong offset type") {
    propertyTableProperty.arrayOffsetType =
        ExtensionExtStructuralMetadataPropertyTableProperty::ArrayOffsetType::
            UINT8;
    MetadataPropertyView<MetadataArrayView<glm::ivec3>> arrayProperty =
        view.getPropertyView<MetadataArrayView<glm::ivec3>>(
            "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);

    propertyTableProperty.arrayOffsetType =
        ExtensionExtStructuralMetadataPropertyTableProperty::ArrayOffsetType::
            UINT16;
    arrayProperty = view.getPropertyView<MetadataArrayView<glm::ivec3>>(
        "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);

    propertyTableProperty.arrayOffsetType = "NONSENSE";
    arrayProperty = view.getPropertyView<MetadataArrayView<glm::ivec3>>(
        "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorInvalidArrayOffsetType);

    propertyTableProperty.arrayOffsetType = "";
    propertyTableProperty.stringOffsetType =
        ExtensionExtStructuralMetadataPropertyTableProperty::StringOffsetType::
            UINT64;
    arrayProperty = view.getPropertyView<MetadataArrayView<glm::ivec3>>(
        "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorInvalidArrayOffsetType);
  }

  SECTION("Offset values are not sorted ascending") {
    propertyTableProperty.arrayOffsetType =
        ExtensionExtStructuralMetadataPropertyTableProperty::ArrayOffsetType::
            UINT64;
    uint64_t* offset = reinterpret_cast<uint64_t*>(
        model.buffers[offsetBufferIndex].cesium.data.data());
    offset[propertyTable.count] = 0;
    MetadataPropertyView<MetadataArrayView<glm::ivec3>> arrayProperty =
        view.getPropertyView<MetadataArrayView<glm::ivec3>>(
            "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorArrayOffsetsNotSorted);
  }

  SECTION("Offset value points outside of value buffer") {
    uint64_t* offset = reinterpret_cast<uint64_t*>(
        model.buffers[offsetBufferIndex].cesium.data.data());
    offset[propertyTable.count] =
        static_cast<uint32_t>(model.buffers[valueBufferIndex].byteLength + 4);
    MetadataPropertyView<MetadataArrayView<glm::ivec3>> arrayProperty =
        view.getPropertyView<MetadataArrayView<glm::ivec3>>(
            "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorArrayOffsetOutOfBounds);
  }

  SECTION("Count and offset buffer are both present") {
    testClassProperty.count = 3;
    MetadataPropertyView<MetadataArrayView<glm::ivec3>> property =
        view.getPropertyView<MetadataArrayView<glm::ivec3>>(
            "TestClassProperty");
    REQUIRE(
        property.status() ==
        MetadataPropertyViewStatus::ErrorArrayCountAndOffsetBufferCoexist);
  }
}

TEST_CASE("Test StructuralMetadata fixed-length matN array") {
  Model model;

  // clang-format off
  std::vector<glm::i32mat2x2> values = {
      glm::i32mat2x2(
        12, 34,
        -30, 20),
      glm::i32mat2x2(
        -2, -2,
        0, 1),
      glm::i32mat2x2(
        1, 2,
        8, 5),
      glm::i32mat2x2(
        -100, 3,
        84, 6),
      glm::i32mat2x2(
        2, 12,
        -2, -2),
      glm::i32mat2x2(
        40, 61,
        7, -3),
  };
  // clang-format on

  size_t valueBufferViewIndex = 0;
  {
    Buffer& valueBuffer = model.buffers.emplace_back();
    valueBuffer.cesium.data.resize(values.size() * sizeof(glm::i32mat2x2));
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

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  ExtensionExtStructuralMetadataSchema& schema = metadata.schema.emplace();
  ExtensionExtStructuralMetadataClass& testClass = schema.classes["TestClass"];
  ExtensionExtStructuralMetadataClassProperty& testClassProperty =
      testClass.properties["TestClassProperty"];
  testClassProperty.type =
      ExtensionExtStructuralMetadataClassProperty::Type::MAT2;
  testClassProperty.componentType =
      ExtensionExtStructuralMetadataClassProperty::ComponentType::INT32;
  testClassProperty.array = true;
  testClassProperty.count = 2;

  ExtensionExtStructuralMetadataPropertyTable& propertyTable =
      metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(
      values.size() / static_cast<size_t>(testClassProperty.count.value()));

  ExtensionExtStructuralMetadataPropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values =
      static_cast<int32_t>(model.bufferViews.size() - 1);

  MetadataPropertyTableView view(&model, &propertyTable);
  const ExtensionExtStructuralMetadataClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(
      classProperty->type ==
      ExtensionExtStructuralMetadataClassProperty::Type::MAT2);
  REQUIRE(
      classProperty->componentType ==
      ExtensionExtStructuralMetadataClassProperty::ComponentType::INT32);
  REQUIRE(classProperty->array);
  REQUIRE(classProperty->count == 2);

  SECTION("Access the right type") {
    MetadataPropertyView<MetadataArrayView<glm::i32mat2x2>> arrayProperty =
        view.getPropertyView<MetadataArrayView<glm::i32mat2x2>>(
            "TestClassProperty");
    REQUIRE(arrayProperty.status() == MetadataPropertyViewStatus::Valid);

    for (int64_t i = 0; i < arrayProperty.size(); ++i) {
      MetadataArrayView<glm::i32mat2x2> member = arrayProperty.get(i);
      for (int64_t j = 0; j < member.size(); ++j) {
        REQUIRE(member[j] == values[static_cast<size_t>(i * 2 + j)]);
      }
    }
  }

  SECTION("Wrong type") {
    MetadataPropertyView<MetadataArrayView<int32_t>> int32ArrayInvalid =
        view.getPropertyView<MetadataArrayView<int32_t>>("TestClassProperty");
    REQUIRE(
        int32ArrayInvalid.status() ==
        MetadataPropertyViewStatus::ErrorTypeMismatch);

    MetadataPropertyView<MetadataArrayView<glm::ivec2>> ivec2ArrayInvalid =
        view.getPropertyView<MetadataArrayView<glm::ivec2>>(
            "TestClassProperty");
    REQUIRE(
        ivec2ArrayInvalid.status() ==
        MetadataPropertyViewStatus::ErrorTypeMismatch);
  }

  SECTION("Wrong component type") {
    MetadataPropertyView<MetadataArrayView<glm::u32mat2x2>>
        u32mat2x2ArrayInvalid =
            view.getPropertyView<MetadataArrayView<glm::u32mat2x2>>(
                "TestClassProperty");
    REQUIRE(
        u32mat2x2ArrayInvalid.status() ==
        MetadataPropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SECTION("Not an array type") {
    MetadataPropertyView<glm::i32mat2x2> ivec3Invalid =
        view.getPropertyView<glm::i32mat2x2>("TestClassProperty");
    REQUIRE(
        ivec3Invalid.status() ==
        MetadataPropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SECTION("Buffer size is not a multiple of type size") {
    model.bufferViews[valueBufferViewIndex].byteLength = 13;
    MetadataPropertyView<MetadataArrayView<glm::i32mat2x2>> arrayProperty =
        view.getPropertyView<MetadataArrayView<glm::i32mat2x2>>(
            "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorBufferViewSizeNotDivisibleByTypeSize);
  }

  SECTION("Negative component count") {
    testClassProperty.count = -1;
    MetadataPropertyView<MetadataArrayView<glm::i32mat2x2>> arrayProperty =
        view.getPropertyView<MetadataArrayView<glm::i32mat2x2>>(
            "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorArrayCountAndOffsetBufferDontExist);
  }

  SECTION("Value buffer doesn't fit into property table count") {
    testClassProperty.count = 55;
    MetadataPropertyView<MetadataArrayView<glm::i32mat2x2>> arrayProperty =
        view.getPropertyView<MetadataArrayView<glm::i32mat2x2>>(
            "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);
  }
}

TEST_CASE("Test Structural Metadata variable-length matN array") {
  Model model;

  // clang-format off
    std::vector<glm::i32mat2x2> data0{
        glm::i32mat2x2(
          3, -2,
          1, 0),
        glm::i32mat2x2(
          40, 3,
          8, -9)
    };
    std::vector<glm::i32mat2x2> data1{
        glm::i32mat2x2(
          1, 10,
          7, 8),
    };
    std::vector<glm::i32mat2x2> data2{
        glm::i32mat2x2(
          18, 0,
          1, 17),
        glm::i32mat2x2(
          -4, -2,
          -9, 1),
        glm::i32mat2x2(
          1, 8,
          -99, 3),
    };
  // clang-format on

  std::vector<std::vector<glm::i32mat2x2>>
      expected{data0, {}, data1, data2, {}};

  size_t numOfElements = 0;
  for (const auto& expectedMember : expected) {
    numOfElements += expectedMember.size();
  }

  std::vector<std::byte> values(numOfElements * sizeof(glm::i32mat2x2));
  std::vector<std::byte> offsets((expected.size() + 1) * sizeof(uint64_t));
  uint64_t* offsetValue = reinterpret_cast<uint64_t*>(offsets.data());
  for (size_t i = 0; i < expected.size(); ++i) {
    std::memcpy(
        values.data() + offsetValue[i],
        expected[i].data(),
        expected[i].size() * sizeof(glm::i32mat2x2));
    offsetValue[i + 1] =
        offsetValue[i] + expected[i].size() * sizeof(glm::i32mat2x2);
  }

  size_t valueBufferIndex = 0;
  size_t valueBufferViewIndex = 0;
  {
    Buffer& valueBuffer = model.buffers.emplace_back();
    valueBuffer.byteLength = static_cast<int64_t>(values.size());
    valueBuffer.cesium.data = std::move(values);
    valueBufferIndex = model.buffers.size() - 1;

    BufferView& valueBufferView = model.bufferViews.emplace_back();
    valueBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
    valueBufferView.byteOffset = 0;
    valueBufferView.byteLength = valueBuffer.byteLength;
    valueBufferViewIndex = model.bufferViews.size() - 1;
  }

  size_t offsetBufferIndex = 0;
  size_t offsetBufferViewIndex = 0;
  {
    Buffer& offsetBuffer = model.buffers.emplace_back();
    offsetBuffer.byteLength = static_cast<int64_t>(offsets.size());
    offsetBuffer.cesium.data = std::move(offsets);
    offsetBufferIndex = model.buffers.size() - 1;

    BufferView& offsetBufferView = model.bufferViews.emplace_back();
    offsetBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
    offsetBufferView.byteOffset = 0;
    offsetBufferView.byteLength = offsetBuffer.byteLength;
    offsetBufferViewIndex = model.bufferViews.size() - 1;
  }

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  ExtensionExtStructuralMetadataSchema& schema = metadata.schema.emplace();
  ExtensionExtStructuralMetadataClass& testClass = schema.classes["TestClass"];
  ExtensionExtStructuralMetadataClassProperty& testClassProperty =
      testClass.properties["TestClassProperty"];
  testClassProperty.type =
      ExtensionExtStructuralMetadataClassProperty::Type::MAT2;
  testClassProperty.componentType =
      ExtensionExtStructuralMetadataClassProperty::ComponentType::INT32;
  testClassProperty.array = true;

  ExtensionExtStructuralMetadataPropertyTable& propertyTable =
      metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(expected.size());

  ExtensionExtStructuralMetadataPropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);
  propertyTableProperty.arrayOffsets =
      static_cast<int32_t>(offsetBufferViewIndex);
  propertyTableProperty.arrayOffsetType =
      ExtensionExtStructuralMetadataPropertyTableProperty::ArrayOffsetType::
          UINT64;

  MetadataPropertyTableView view(&model, &propertyTable);
  const ExtensionExtStructuralMetadataClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(
      classProperty->type ==
      ExtensionExtStructuralMetadataClassProperty::Type::MAT2);
  REQUIRE(
      classProperty->componentType ==
      ExtensionExtStructuralMetadataClassProperty::ComponentType::INT32);
  REQUIRE(classProperty->array);
  REQUIRE(!classProperty->count);

  SECTION("Access the correct type") {
    MetadataPropertyView<MetadataArrayView<glm::i32mat2x2>> property =
        view.getPropertyView<MetadataArrayView<glm::i32mat2x2>>(
            "TestClassProperty");
    REQUIRE(property.status() == MetadataPropertyViewStatus::Valid);
    for (size_t i = 0; i < expected.size(); ++i) {
      MetadataArrayView<glm::i32mat2x2> valueMember =
          property.get(static_cast<int64_t>(i));
      REQUIRE(valueMember.size() == static_cast<int64_t>(expected[i].size()));
      for (size_t j = 0; j < expected[i].size(); ++j) {
        REQUIRE(expected[i][j] == valueMember[static_cast<int64_t>(j)]);
      }
    }
  }

  SECTION("Wrong offset type") {
    propertyTableProperty.arrayOffsetType =
        ExtensionExtStructuralMetadataPropertyTableProperty::ArrayOffsetType::
            UINT8;
    MetadataPropertyView<MetadataArrayView<glm::i32mat2x2>> arrayProperty =
        view.getPropertyView<MetadataArrayView<glm::i32mat2x2>>(
            "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);

    propertyTableProperty.arrayOffsetType =
        ExtensionExtStructuralMetadataPropertyTableProperty::ArrayOffsetType::
            UINT16;
    arrayProperty = view.getPropertyView<MetadataArrayView<glm::i32mat2x2>>(
        "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);

    propertyTableProperty.arrayOffsetType = "NONSENSE";
    arrayProperty = view.getPropertyView<MetadataArrayView<glm::i32mat2x2>>(
        "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorInvalidArrayOffsetType);

    propertyTableProperty.arrayOffsetType = "";
    propertyTableProperty.stringOffsetType =
        ExtensionExtStructuralMetadataPropertyTableProperty::StringOffsetType::
            UINT64;
    arrayProperty = view.getPropertyView<MetadataArrayView<glm::i32mat2x2>>(
        "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorInvalidArrayOffsetType);
  }

  SECTION("Offset values are not sorted ascending") {
    propertyTableProperty.arrayOffsetType =
        ExtensionExtStructuralMetadataPropertyTableProperty::ArrayOffsetType::
            UINT64;
    uint64_t* offset = reinterpret_cast<uint64_t*>(
        model.buffers[offsetBufferIndex].cesium.data.data());
    offset[propertyTable.count] = 0;
    MetadataPropertyView<MetadataArrayView<glm::i32mat2x2>> arrayProperty =
        view.getPropertyView<MetadataArrayView<glm::i32mat2x2>>(
            "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorArrayOffsetsNotSorted);
  }

  SECTION("Offset value points outside of value buffer") {
    uint64_t* offset = reinterpret_cast<uint64_t*>(
        model.buffers[offsetBufferIndex].cesium.data.data());
    offset[propertyTable.count] =
        static_cast<uint32_t>(model.buffers[valueBufferIndex].byteLength + 4);
    MetadataPropertyView<MetadataArrayView<glm::i32mat2x2>> arrayProperty =
        view.getPropertyView<MetadataArrayView<glm::i32mat2x2>>(
            "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorArrayOffsetOutOfBounds);
  }

  SECTION("Count and offset buffer are both present") {
    testClassProperty.count = 3;
    MetadataPropertyView<MetadataArrayView<glm::i32mat2x2>> property =
        view.getPropertyView<MetadataArrayView<glm::i32mat2x2>>(
            "TestClassProperty");
    REQUIRE(
        property.status() ==
        MetadataPropertyViewStatus::ErrorArrayCountAndOffsetBufferCoexist);
  }
}

TEST_CASE("Test StructuralMetadata fixed-length boolean array") {
  Model model;

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

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  ExtensionExtStructuralMetadataSchema& schema = metadata.schema.emplace();
  ExtensionExtStructuralMetadataClass& testClass = schema.classes["TestClass"];
  ExtensionExtStructuralMetadataClassProperty& testClassProperty =
      testClass.properties["TestClassProperty"];
  testClassProperty.type =
      ExtensionExtStructuralMetadataClassProperty::Type::BOOLEAN;
  testClassProperty.array = true;
  testClassProperty.count = 3;

  ExtensionExtStructuralMetadataPropertyTable& propertyTable =
      metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(
      expected.size() / static_cast<size_t>(testClassProperty.count.value()));

  ExtensionExtStructuralMetadataPropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values =
      static_cast<int32_t>(model.bufferViews.size() - 1);

  MetadataPropertyTableView view(&model, &propertyTable);
  const ExtensionExtStructuralMetadataClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(
      classProperty->type ==
      ExtensionExtStructuralMetadataClassProperty::Type::BOOLEAN);
  REQUIRE(classProperty->array);
  REQUIRE(classProperty->count == 3);

  SECTION("Access correct type") {
    MetadataPropertyView<MetadataArrayView<bool>> boolArrayProperty =
        view.getPropertyView<MetadataArrayView<bool>>("TestClassProperty");
    REQUIRE(boolArrayProperty.status() == MetadataPropertyViewStatus::Valid);
    REQUIRE(boolArrayProperty.size() == propertyTable.count);
    REQUIRE(boolArrayProperty.size() > 0);
    for (int64_t i = 0; i < boolArrayProperty.size(); ++i) {
      MetadataArrayView<bool> valueMember = boolArrayProperty.get(i);
      for (int64_t j = 0; j < valueMember.size(); ++j) {
        REQUIRE(valueMember[j] == expected[static_cast<size_t>(i * 3 + j)]);
      }
    }
  }

  SECTION("Wrong type") {
    MetadataPropertyView<MetadataArrayView<uint8_t>> uint8ArrayInvalid =
        view.getPropertyView<MetadataArrayView<uint8_t>>("TestClassProperty");
    REQUIRE(
        uint8ArrayInvalid.status() ==
        MetadataPropertyViewStatus::ErrorTypeMismatch);
  }

  SECTION("View is not array type") {
    MetadataPropertyView<bool> boolInvalid =
        view.getPropertyView<bool>("TestClassProperty");
    REQUIRE(
        boolInvalid.status() ==
        MetadataPropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SECTION("Value buffer doesn't have enough required bytes") {
    testClassProperty.count = 11;
    MetadataPropertyView<MetadataArrayView<bool>> boolArrayProperty =
        view.getPropertyView<MetadataArrayView<bool>>("TestClassProperty");
    REQUIRE(
        boolArrayProperty.status() ==
        MetadataPropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);
  }

  SECTION("Count is negative") {
    testClassProperty.count = -1;
    MetadataPropertyView<MetadataArrayView<bool>> boolArrayProperty =
        view.getPropertyView<MetadataArrayView<bool>>("TestClassProperty");
    REQUIRE(
        boolArrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorArrayCountAndOffsetBufferDontExist);
  }
}

TEST_CASE("Test StructuralMetadata variable-length boolean array") {
  Model model;

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

  size_t valueBufferViewIndex = 0;
  size_t valueBufferIndex = 0;
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

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  ExtensionExtStructuralMetadataSchema& schema = metadata.schema.emplace();
  ExtensionExtStructuralMetadataClass& testClass = schema.classes["TestClass"];
  ExtensionExtStructuralMetadataClassProperty& testClassProperty =
      testClass.properties["TestClassProperty"];
  testClassProperty.type =
      ExtensionExtStructuralMetadataClassProperty::Type::BOOLEAN;
  testClassProperty.array = true;

  ExtensionExtStructuralMetadataPropertyTable& propertyTable =
      metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(expected.size());

  ExtensionExtStructuralMetadataPropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);
  propertyTableProperty.arrayOffsets =
      static_cast<int32_t>(offsetBufferViewIndex);
  propertyTableProperty.arrayOffsetType =
      ExtensionExtStructuralMetadataPropertyTableProperty::ArrayOffsetType::
          UINT64;

  MetadataPropertyTableView view(&model, &propertyTable);
  const ExtensionExtStructuralMetadataClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(
      classProperty->type ==
      ExtensionExtStructuralMetadataClassProperty::Type::BOOLEAN);
  REQUIRE(classProperty->array);
  REQUIRE(!classProperty->count);

  SECTION("Access correct type") {
    MetadataPropertyView<MetadataArrayView<bool>> boolArrayProperty =
        view.getPropertyView<MetadataArrayView<bool>>("TestClassProperty");
    REQUIRE(boolArrayProperty.status() == MetadataPropertyViewStatus::Valid);
    for (size_t i = 0; i < expected.size(); ++i) {
      MetadataArrayView<bool> arrayMember =
          boolArrayProperty.get(static_cast<int64_t>(i));
      REQUIRE(arrayMember.size() == static_cast<int64_t>(expected[i].size()));
      for (size_t j = 0; j < expected[i].size(); ++j) {
        REQUIRE(expected[i][j] == arrayMember[static_cast<int64_t>(j)]);
      }
    }
  }

  SECTION("Wrong offset type") {
    propertyTableProperty.arrayOffsetType =
        ExtensionExtStructuralMetadataPropertyTableProperty::ArrayOffsetType::
            UINT8;
    MetadataPropertyView<MetadataArrayView<bool>> arrayProperty =
        view.getPropertyView<MetadataArrayView<bool>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);

    propertyTableProperty.arrayOffsetType =
        ExtensionExtStructuralMetadataPropertyTableProperty::ArrayOffsetType::
            UINT16;
    arrayProperty =
        view.getPropertyView<MetadataArrayView<bool>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);

    propertyTableProperty.arrayOffsetType = "NONSENSE";
    arrayProperty =
        view.getPropertyView<MetadataArrayView<bool>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorInvalidArrayOffsetType);

    propertyTableProperty.arrayOffsetType = "";
    propertyTableProperty.stringOffsetType =
        ExtensionExtStructuralMetadataPropertyTableProperty::StringOffsetType::
            UINT64;
    arrayProperty =
        view.getPropertyView<MetadataArrayView<bool>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorInvalidArrayOffsetType);
  }

  SECTION("Offset values are not sorted ascending") {
    propertyTableProperty.arrayOffsetType =
        ExtensionExtStructuralMetadataPropertyTableProperty::ArrayOffsetType::
            UINT64;
    uint64_t* offset = reinterpret_cast<uint64_t*>(
        model.buffers[offsetBufferIndex].cesium.data.data());
    offset[propertyTable.count] = 0;
    MetadataPropertyView<MetadataArrayView<bool>> arrayProperty =
        view.getPropertyView<MetadataArrayView<bool>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorArrayOffsetsNotSorted);
  }

  SECTION("Offset value points outside of value buffer") {
    uint64_t* offset = reinterpret_cast<uint64_t*>(
        model.buffers[offsetBufferIndex].cesium.data.data());
    offset[propertyTable.count] = static_cast<uint32_t>(
        model.buffers[valueBufferIndex].byteLength * 8 + 20);
    MetadataPropertyView<MetadataArrayView<bool>> arrayProperty =
        view.getPropertyView<MetadataArrayView<bool>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorArrayOffsetOutOfBounds);
  }

  SECTION("Count and offset buffer both present") {
    testClassProperty.count = 3;
    MetadataPropertyView<MetadataArrayView<bool>> boolArrayProperty =
        view.getPropertyView<MetadataArrayView<bool>>("TestClassProperty");
    REQUIRE(
        boolArrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorArrayCountAndOffsetBufferCoexist);
  }
}

TEST_CASE("Test StructuralMetadata fixed-length arrays of strings") {
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

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  ExtensionExtStructuralMetadataSchema& schema = metadata.schema.emplace();
  ExtensionExtStructuralMetadataClass& testClass = schema.classes["TestClass"];
  ExtensionExtStructuralMetadataClassProperty& testClassProperty =
      testClass.properties["TestClassProperty"];
  testClassProperty.type =
      ExtensionExtStructuralMetadataClassProperty::Type::STRING;
  testClassProperty.array = true;
  testClassProperty.count = 2;

  ExtensionExtStructuralMetadataPropertyTable& propertyTable =
      metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(
      expected.size() / static_cast<size_t>(testClassProperty.count.value()));

  ExtensionExtStructuralMetadataPropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.stringOffsetType =
      ExtensionExtStructuralMetadataPropertyTableProperty::StringOffsetType::
          UINT32;
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);
  propertyTableProperty.stringOffsets =
      static_cast<int32_t>(offsetBufferViewIndex);

  MetadataPropertyTableView view(&model, &propertyTable);
  const ExtensionExtStructuralMetadataClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(
      classProperty->type ==
      ExtensionExtStructuralMetadataClassProperty::Type::STRING);
  REQUIRE(classProperty->array);
  REQUIRE(classProperty->count == 2);

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

  SECTION("Array type mismatch") {
    MetadataPropertyView<std::string_view> stringProperty =
        view.getPropertyView<std::string_view>("TestClassProperty");
    REQUIRE(
        stringProperty.status() ==
        MetadataPropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SECTION("Count is negative") {
    testClassProperty.count = -1;
    MetadataPropertyView<MetadataArrayView<std::string_view>> stringProperty =
        view.getPropertyView<MetadataArrayView<std::string_view>>(
            "TestClassProperty");
    REQUIRE(
        stringProperty.status() ==
        MetadataPropertyViewStatus::ErrorArrayCountAndOffsetBufferDontExist);
  }

  SECTION("Offset type is unknown") {
    propertyTableProperty.stringOffsetType = "NONSENSE";
    MetadataPropertyView<MetadataArrayView<std::string_view>> stringProperty =
        view.getPropertyView<MetadataArrayView<std::string_view>>(
            "TestClassProperty");
    REQUIRE(
        stringProperty.status() ==
        MetadataPropertyViewStatus::ErrorInvalidStringOffsetType);

    propertyTableProperty.stringOffsetType = "";
    propertyTableProperty.arrayOffsetType =
        ExtensionExtStructuralMetadataPropertyTableProperty::ArrayOffsetType::
            UINT32;
    stringProperty = view.getPropertyView<MetadataArrayView<std::string_view>>(
        "TestClassProperty");
    REQUIRE(
        stringProperty.status() ==
        MetadataPropertyViewStatus::ErrorInvalidStringOffsetType);
  }

  SECTION("String offsets don't exist") {
    propertyTableProperty.stringOffsets = -1;
    MetadataPropertyView<MetadataArrayView<std::string_view>> stringProperty =
        view.getPropertyView<MetadataArrayView<std::string_view>>(
            "TestClassProperty");
    REQUIRE(
        stringProperty.status() ==
        MetadataPropertyViewStatus::ErrorInvalidStringOffsetBufferView);
  }
}

TEST_CASE("Test StructuralMetadata variable-length arrays of strings") {
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

  size_t arrayOffsetBuffer = 0;
  size_t arrayOffsetBufferView = 0;
  {
    Buffer& offsetBuffer = model.buffers.emplace_back();
    offsetBuffer.byteLength = static_cast<int64_t>(offsets.size());
    offsetBuffer.cesium.data = std::move(offsets);
    arrayOffsetBuffer = model.buffers.size() - 1;

    BufferView& offsetBufferView = model.bufferViews.emplace_back();
    offsetBufferView.buffer = static_cast<int32_t>(arrayOffsetBuffer);
    offsetBufferView.byteOffset = 0;
    offsetBufferView.byteLength = offsetBuffer.byteLength;
    arrayOffsetBufferView = model.bufferViews.size() - 1;
  }

  size_t stringOffsetBuffer = 0;
  size_t stringOffsetBufferView = 0;
  {
    Buffer& strOffsetBuffer = model.buffers.emplace_back();
    strOffsetBuffer.byteLength = static_cast<int64_t>(stringOffsets.size());
    strOffsetBuffer.cesium.data = std::move(stringOffsets);
    stringOffsetBuffer = model.buffers.size() - 1;

    BufferView& strOffsetBufferView = model.bufferViews.emplace_back();
    strOffsetBufferView.buffer = static_cast<int32_t>(stringOffsetBuffer);
    strOffsetBufferView.byteOffset = 0;
    strOffsetBufferView.byteLength = strOffsetBuffer.byteLength;
    stringOffsetBufferView = model.bufferViews.size() - 1;
  }

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  ExtensionExtStructuralMetadataSchema& schema = metadata.schema.emplace();
  ExtensionExtStructuralMetadataClass& testClass = schema.classes["TestClass"];
  ExtensionExtStructuralMetadataClassProperty& testClassProperty =
      testClass.properties["TestClassProperty"];
  testClassProperty.type =
      ExtensionExtStructuralMetadataClassProperty::Type::STRING;
  testClassProperty.array = true;

  ExtensionExtStructuralMetadataPropertyTable& propertyTable =
      metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(expected.size());

  ExtensionExtStructuralMetadataPropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.arrayOffsetType =
      ExtensionExtStructuralMetadataPropertyTableProperty::ArrayOffsetType::
          UINT32;
  propertyTableProperty.stringOffsetType =
      ExtensionExtStructuralMetadataPropertyTableProperty::StringOffsetType::
          UINT32;
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);
  propertyTableProperty.arrayOffsets =
      static_cast<int32_t>(arrayOffsetBufferView);
  propertyTableProperty.stringOffsets =
      static_cast<int32_t>(stringOffsetBufferView);

  MetadataPropertyTableView view(&model, &propertyTable);
  const ExtensionExtStructuralMetadataClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(
      classProperty->type ==
      ExtensionExtStructuralMetadataClassProperty::Type::STRING);
  REQUIRE(classProperty->array);
  REQUIRE(!classProperty->componentType);
  REQUIRE(!classProperty->count);

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

  SECTION("Wrong array offset type") {
    propertyTableProperty.arrayOffsetType =
        ExtensionExtStructuralMetadataPropertyTableProperty::ArrayOffsetType::
            UINT8;
    MetadataPropertyView<MetadataArrayView<std::string_view>> arrayProperty =
        view.getPropertyView<MetadataArrayView<std::string_view>>(
            "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);

    propertyTableProperty.arrayOffsetType =
        ExtensionExtStructuralMetadataPropertyTableProperty::ArrayOffsetType::
            UINT16;
    arrayProperty = view.getPropertyView<MetadataArrayView<std::string_view>>(
        "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);

    propertyTableProperty.arrayOffsetType = "NONSENSE";
    arrayProperty = view.getPropertyView<MetadataArrayView<std::string_view>>(
        "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorInvalidArrayOffsetType);
    propertyTableProperty.arrayOffsetType =
        ExtensionExtStructuralMetadataPropertyTableProperty::ArrayOffsetType::
            UINT32;
  }

  SECTION("Wrong string offset type") {
    propertyTableProperty.stringOffsetType =
        ExtensionExtStructuralMetadataPropertyTableProperty::StringOffsetType::
            UINT8;
    MetadataPropertyView<MetadataArrayView<std::string_view>> arrayProperty =
        view.getPropertyView<MetadataArrayView<std::string_view>>(
            "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);

    propertyTableProperty.stringOffsetType =
        ExtensionExtStructuralMetadataPropertyTableProperty::StringOffsetType::
            UINT16;
    arrayProperty = view.getPropertyView<MetadataArrayView<std::string_view>>(
        "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);

    propertyTableProperty.stringOffsetType = "NONSENSE";
    arrayProperty = view.getPropertyView<MetadataArrayView<std::string_view>>(
        "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorInvalidStringOffsetType);
    propertyTableProperty.stringOffsetType =
        ExtensionExtStructuralMetadataPropertyTableProperty::StringOffsetType::
            UINT32;
  }

  SECTION("Array offset values are not sorted ascending") {
    uint32_t* offset = reinterpret_cast<uint32_t*>(
        model.buffers[arrayOffsetBuffer].cesium.data.data());
    offset[0] = static_cast<uint32_t>(1000);
    MetadataPropertyView<MetadataArrayView<std::string_view>> arrayProperty =
        view.getPropertyView<MetadataArrayView<std::string_view>>(
            "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorArrayOffsetsNotSorted);
    offset[0] = 0;
  }

  SECTION("String offset values are not sorted ascending") {
    uint32_t* offset = reinterpret_cast<uint32_t*>(
        model.buffers[stringOffsetBuffer].cesium.data.data());
    offset[0] = static_cast<uint32_t>(1000);
    MetadataPropertyView<MetadataArrayView<std::string_view>> arrayProperty =
        view.getPropertyView<MetadataArrayView<std::string_view>>(
            "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorStringOffsetsNotSorted);
    offset[0] = 0;
  }

  SECTION("Array offset value points outside of value buffer") {
    uint32_t* offset = reinterpret_cast<uint32_t*>(
        model.buffers[arrayOffsetBuffer].cesium.data.data());
    uint32_t previousValue = offset[propertyTable.count];
    offset[propertyTable.count] = static_cast<uint32_t>(100000);
    MetadataPropertyView<MetadataArrayView<std::string_view>> arrayProperty =
        view.getPropertyView<MetadataArrayView<std::string_view>>(
            "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorArrayOffsetOutOfBounds);
    offset[propertyTable.count] = previousValue;
  }

  SECTION("String offset value points outside of value buffer") {
    uint32_t* offset = reinterpret_cast<uint32_t*>(
        model.buffers[stringOffsetBuffer].cesium.data.data());
    uint32_t previousValue = offset[6];
    offset[6] = static_cast<uint32_t>(100000);
    MetadataPropertyView<MetadataArrayView<std::string_view>> arrayProperty =
        view.getPropertyView<MetadataArrayView<std::string_view>>(
            "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorStringOffsetOutOfBounds);
    offset[6] = previousValue;
  }

  SECTION("Count and offset buffer both present") {
    testClassProperty.count = 3;
    MetadataPropertyView<MetadataArrayView<std::string_view>>
        boolArrayProperty =
            view.getPropertyView<MetadataArrayView<std::string_view>>(
                "TestClassProperty");
    REQUIRE(
        boolArrayProperty.status() ==
        MetadataPropertyViewStatus::ErrorArrayCountAndOffsetBufferCoexist);
  }
}
