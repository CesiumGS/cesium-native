#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/Class.h>
#include <CesiumGltf/ClassProperty.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/PropertyArrayView.h>
#include <CesiumGltf/PropertyTable.h>
#include <CesiumGltf/PropertyTableProperty.h>
#include <CesiumGltf/PropertyTablePropertyView.h>
#include <CesiumGltf/PropertyTableView.h>
#include <CesiumGltf/PropertyTransformations.h>
#include <CesiumGltf/Schema.h>

#include <doctest/doctest.h>
#include <glm/common.hpp>
#include <glm/ext/matrix_double2x2.hpp>
#include <glm/ext/matrix_float2x2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/ext/vector_int3.hpp>
#include <glm/ext/vector_int3_sized.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <glm/ext/vector_uint3.hpp>
#include <glm/ext/vector_uint3_sized.hpp>
#include <glm/fwd.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string_view>
#include <vector>

using namespace CesiumGltf;

template <typename T>
void addBufferToModel(Model& model, const std::vector<T>& values) {
  Buffer& valueBuffer = model.buffers.emplace_back();
  valueBuffer.cesium.data.resize(values.size() * sizeof(T));
  valueBuffer.byteLength = static_cast<int64_t>(valueBuffer.cesium.data.size());
  std::memcpy(
      valueBuffer.cesium.data.data(),
      values.data(),
      valueBuffer.cesium.data.size());

  BufferView& valueBufferView = model.bufferViews.emplace_back();
  valueBufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
  valueBufferView.byteOffset = 0;
  valueBufferView.byteLength = valueBuffer.byteLength;
}

TEST_CASE("Test PropertyTableView on model without EXT_structural_metadata "
          "extension") {
  Model model;

  // Create an erroneously isolated property table.
  PropertyTable propertyTable;
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(10);

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(0);

  PropertyTableView view(model, propertyTable);
  REQUIRE(
      view.status() == PropertyTableViewStatus::ErrorMissingMetadataExtension);
  REQUIRE(view.size() == 0);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(!classProperty);
}

TEST_CASE("Test PropertyTableView on model without metadata schema") {
  Model model;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(10);

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(0);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::ErrorMissingSchema);
  REQUIRE(view.size() == 0);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(!classProperty);
}

TEST_CASE("Test property table with nonexistent class") {
  Model model;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT32;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "I Don't Exist";
  propertyTable.count = static_cast<int64_t>(10);

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(0);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::ErrorClassNotFound);
  REQUIRE(view.size() == 0);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(!classProperty);
}

TEST_CASE("Test scalar PropertyTableProperty") {
  Model model;
  std::vector<uint32_t> values = {12, 34, 30, 11, 34, 34, 11, 33, 122, 33};

  addBufferToModel(model, values);
  size_t valueBufferIndex = model.buffers.size() - 1;
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT32;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(values.size());

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT32);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(!classProperty->normalized);

  SUBCASE("Access correct type") {
    PropertyTablePropertyView<uint32_t> uint32Property =
        view.getPropertyView<uint32_t>("TestClassProperty");
    REQUIRE(uint32Property.status() == PropertyTablePropertyViewStatus::Valid);
    REQUIRE(uint32Property.size() > 0);

    for (int64_t i = 0; i < uint32Property.size(); ++i) {
      REQUIRE(uint32Property.getRaw(i) == values[static_cast<size_t>(i)]);
      REQUIRE(uint32Property.get(i) == uint32Property.getRaw(i));
    }
  }

  SUBCASE("Access wrong type") {
    PropertyTablePropertyView<glm::uvec3> uvec3Invalid =
        view.getPropertyView<glm::uvec3>("TestClassProperty");
    REQUIRE(
        uvec3Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);

    PropertyTablePropertyView<glm::umat3x3> umat3x3Invalid =
        view.getPropertyView<glm::umat3x3>("TestClassProperty");
    REQUIRE(
        umat3x3Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);

    PropertyTablePropertyView<bool> boolInvalid =
        view.getPropertyView<bool>("TestClassProperty");
    REQUIRE(
        boolInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);

    PropertyTablePropertyView<std::string_view> stringInvalid =
        view.getPropertyView<std::string_view>("TestClassProperty");
    REQUIRE(
        stringInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Access wrong component type") {
    PropertyTablePropertyView<uint8_t> uint8Invalid =
        view.getPropertyView<uint8_t>("TestClassProperty");
    REQUIRE(
        uint8Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch);

    PropertyTablePropertyView<int32_t> int32Invalid =
        view.getPropertyView<int32_t>("TestClassProperty");
    REQUIRE(
        int32Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch);

    PropertyTablePropertyView<uint64_t> uint64Invalid =
        view.getPropertyView<uint64_t>("TestClassProperty");
    REQUIRE(
        uint64Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Access incorrectly as normalized") {
    PropertyTablePropertyView<uint32_t, true> uint32NormalizedInvalid =
        view.getPropertyView<uint32_t, true>("TestClassProperty");
    REQUIRE(
        uint32NormalizedInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Wrong buffer index") {
    model.bufferViews[valueBufferViewIndex].buffer = 2;
    PropertyTablePropertyView<uint32_t> uint32Property =
        view.getPropertyView<uint32_t>("TestClassProperty");
    REQUIRE(
        uint32Property.status() ==
        PropertyTablePropertyViewStatus::ErrorInvalidValueBuffer);
  }

  SUBCASE("Wrong buffer view index") {
    propertyTableProperty.values = -1;
    PropertyTablePropertyView<uint32_t> uint32Property =
        view.getPropertyView<uint32_t>("TestClassProperty");
    REQUIRE(
        uint32Property.status() ==
        PropertyTablePropertyViewStatus::ErrorInvalidValueBufferView);
  }

  SUBCASE("Buffer view points outside of the real buffer length") {
    model.buffers[valueBufferIndex].cesium.data.resize(12);
    PropertyTablePropertyView<uint32_t> uint32Property =
        view.getPropertyView<uint32_t>("TestClassProperty");
    REQUIRE(
        uint32Property.status() ==
        PropertyTablePropertyViewStatus::ErrorBufferViewOutOfBounds);
  }

  SUBCASE("Buffer view length isn't multiple of sizeof(T)") {
    model.bufferViews[valueBufferViewIndex].byteLength = 13;
    PropertyTablePropertyView<uint32_t> uint32Property =
        view.getPropertyView<uint32_t>("TestClassProperty");
    REQUIRE(
        uint32Property.status() ==
        PropertyTablePropertyViewStatus::
            ErrorBufferViewSizeNotDivisibleByTypeSize);
  }

  SUBCASE("Buffer view length doesn't match with propertyTableCount") {
    model.bufferViews[valueBufferViewIndex].byteLength = 12;
    PropertyTablePropertyView<uint32_t> uint32Property =
        view.getPropertyView<uint32_t>("TestClassProperty");
    REQUIRE(
        uint32Property.status() ==
        PropertyTablePropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);
  }
}

TEST_CASE("Test scalar PropertyTableProperty (normalized)") {
  Model model;
  std::vector<int16_t> values = {-128, 0, 32, 2340, -1234, 127};

  addBufferToModel(model, values);
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::INT16;
  testClassProperty.normalized = true;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(values.size());

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT16);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(classProperty->normalized);
  REQUIRE(!classProperty->array);

  SUBCASE("Access correct type") {
    PropertyTablePropertyView<int16_t, true> int16Property =
        view.getPropertyView<int16_t, true>("TestClassProperty");
    REQUIRE(int16Property.status() == PropertyTablePropertyViewStatus::Valid);
    REQUIRE(int16Property.size() > 0);

    for (int64_t i = 0; i < int16Property.size(); ++i) {
      auto value = int16Property.getRaw(i);
      REQUIRE(value == values[static_cast<size_t>(i)]);
      REQUIRE(int16Property.get(i) == normalize(value));
    }
  }

  SUBCASE("Access wrong type") {
    PropertyTablePropertyView<glm::i16vec3, true> i16vec3Invalid =
        view.getPropertyView<glm::i16vec3, true>("TestClassProperty");
    REQUIRE(
        i16vec3Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);

    PropertyTablePropertyView<glm::i16mat3x3, true> i16mat3x3Invalid =
        view.getPropertyView<glm::i16mat3x3, true>("TestClassProperty");
    REQUIRE(
        i16mat3x3Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Access wrong component type") {
    PropertyTablePropertyView<uint16_t, true> uint16Invalid =
        view.getPropertyView<uint16_t, true>("TestClassProperty");
    REQUIRE(
        uint16Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch);

    PropertyTablePropertyView<int32_t, true> int32Invalid =
        view.getPropertyView<int32_t, true>("TestClassProperty");
    REQUIRE(
        int32Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Access incorrectly as array") {
    PropertyTablePropertyView<PropertyArrayView<int16_t>, true> arrayInvalid =
        view.getPropertyView<PropertyArrayView<int16_t>, true>(
            "TestClassProperty");
    REQUIRE(
        arrayInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Access incorrectly as non-normalized") {
    PropertyTablePropertyView<int16_t> nonNormalizedInvalid =
        view.getPropertyView<int16_t>("TestClassProperty");
    REQUIRE(
        nonNormalizedInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Access incorrectly as double") {
    PropertyTablePropertyView<double> doubleInvalid =
        view.getPropertyView<double>("TestClassProperty");
    REQUIRE(
        doubleInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch);
  }
}

TEST_CASE("Test vecN PropertyTableProperty") {
  Model model;
  std::vector<glm::ivec3> values = {
      glm::ivec3(-12, 34, 30),
      glm::ivec3(11, 73, 0),
      glm::ivec3(-2, 6, 12),
      glm::ivec3(-4, 8, -13)};

  addBufferToModel(model, values);
  size_t valueBufferIndex = model.buffers.size() - 1;
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::VEC3;
  testClassProperty.componentType = ClassProperty::ComponentType::INT32;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(values.size());

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::VEC3);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT32);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(!classProperty->normalized);

  SUBCASE("Access correct type") {
    PropertyTablePropertyView<glm::ivec3> ivec3Property =
        view.getPropertyView<glm::ivec3>("TestClassProperty");
    REQUIRE(ivec3Property.status() == PropertyTablePropertyViewStatus::Valid);
    REQUIRE(ivec3Property.size() > 0);

    for (int64_t i = 0; i < ivec3Property.size(); ++i) {
      REQUIRE(ivec3Property.getRaw(i) == values[static_cast<size_t>(i)]);
      REQUIRE(ivec3Property.get(i) == ivec3Property.getRaw(i));
    }
  }

  SUBCASE("Access wrong type") {
    PropertyTablePropertyView<int32_t> int32Invalid =
        view.getPropertyView<int32_t>("TestClassProperty");
    REQUIRE(
        int32Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);

    PropertyTablePropertyView<glm::ivec2> ivec2Invalid =
        view.getPropertyView<glm::ivec2>("TestClassProperty");
    REQUIRE(
        ivec2Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);

    PropertyTablePropertyView<glm::i32mat3x3> i32mat3x3Invalid =
        view.getPropertyView<glm::i32mat3x3>("TestClassProperty");
    REQUIRE(
        i32mat3x3Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);

    PropertyTablePropertyView<bool> boolInvalid =
        view.getPropertyView<bool>("TestClassProperty");
    REQUIRE(
        boolInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);

    PropertyTablePropertyView<std::string_view> stringInvalid =
        view.getPropertyView<std::string_view>("TestClassProperty");
    REQUIRE(
        stringInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Access wrong component type") {
    PropertyTablePropertyView<glm::u8vec3> u8vec3Invalid =
        view.getPropertyView<glm::u8vec3>("TestClassProperty");
    REQUIRE(
        u8vec3Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch);

    PropertyTablePropertyView<glm::i16vec3> i16vec3Invalid =
        view.getPropertyView<glm::i16vec3>("TestClassProperty");
    REQUIRE(
        i16vec3Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch);

    PropertyTablePropertyView<glm::vec3> vec3Invalid =
        view.getPropertyView<glm::vec3>("TestClassProperty");
    REQUIRE(
        vec3Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Access incorrectly as array") {
    PropertyTablePropertyView<PropertyArrayView<glm::ivec3>> ivec3ArrayInvalid =
        view.getPropertyView<PropertyArrayView<glm::ivec3>>(
            "TestClassProperty");
    REQUIRE(
        ivec3ArrayInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Access incorrectly as normalized") {
    PropertyTablePropertyView<glm::ivec3, true> normalizedInvalid =
        view.getPropertyView<glm::ivec3, true>("TestClassProperty");
    REQUIRE(
        normalizedInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Wrong buffer index") {
    model.bufferViews[valueBufferViewIndex].buffer = 2;
    PropertyTablePropertyView<glm::ivec3> ivec3Property =
        view.getPropertyView<glm::ivec3>("TestClassProperty");
    REQUIRE(
        ivec3Property.status() ==
        PropertyTablePropertyViewStatus::ErrorInvalidValueBuffer);
  }

  SUBCASE("Wrong buffer view index") {
    propertyTableProperty.values = -1;
    PropertyTablePropertyView<glm::ivec3> ivec3Property =
        view.getPropertyView<glm::ivec3>("TestClassProperty");
    REQUIRE(
        ivec3Property.status() ==
        PropertyTablePropertyViewStatus::ErrorInvalidValueBufferView);
  }

  SUBCASE("Buffer view points outside of the real buffer length") {
    model.buffers[valueBufferIndex].cesium.data.resize(12);
    PropertyTablePropertyView<glm::ivec3> ivec3Property =
        view.getPropertyView<glm::ivec3>("TestClassProperty");
    REQUIRE(
        ivec3Property.status() ==
        PropertyTablePropertyViewStatus::ErrorBufferViewOutOfBounds);
  }

  SUBCASE("Buffer view length isn't multiple of sizeof(T)") {
    model.bufferViews[valueBufferViewIndex].byteLength = 11;
    PropertyTablePropertyView<glm::ivec3> ivec3Property =
        view.getPropertyView<glm::ivec3>("TestClassProperty");
    REQUIRE(
        ivec3Property.status() ==
        PropertyTablePropertyViewStatus::
            ErrorBufferViewSizeNotDivisibleByTypeSize);
  }

  SUBCASE("Buffer view length doesn't match with propertyTableCount") {
    model.bufferViews[valueBufferViewIndex].byteLength = 12;
    PropertyTablePropertyView<glm::ivec3> ivec3Property =
        view.getPropertyView<glm::ivec3>("TestClassProperty");
    REQUIRE(
        ivec3Property.status() ==
        PropertyTablePropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);
  }
}

TEST_CASE("Test vecN PropertyTableProperty (normalized)") {
  Model model;
  std::vector<glm::ivec3> values = {
      glm::ivec3(-12, 34, 30),
      glm::ivec3(11, 73, 0),
      glm::ivec3(-2, 6, 12),
      glm::ivec3(-4, 8, -13)};

  addBufferToModel(model, values);
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::VEC3;
  testClassProperty.componentType = ClassProperty::ComponentType::INT32;
  testClassProperty.normalized = true;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(values.size());

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::VEC3);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT32);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(classProperty->normalized);

  SUBCASE("Access correct type") {
    PropertyTablePropertyView<glm::ivec3, true> ivec3Property =
        view.getPropertyView<glm::ivec3, true>("TestClassProperty");
    REQUIRE(ivec3Property.status() == PropertyTablePropertyViewStatus::Valid);
    REQUIRE(ivec3Property.size() > 0);

    for (int64_t i = 0; i < ivec3Property.size(); ++i) {
      auto value = ivec3Property.getRaw(i);
      REQUIRE(value == values[static_cast<size_t>(i)]);
      REQUIRE(ivec3Property.get(i) == normalize(value));
    }
  }

  SUBCASE("Access wrong type") {
    PropertyTablePropertyView<int32_t, true> int32Invalid =
        view.getPropertyView<int32_t, true>("TestClassProperty");
    REQUIRE(
        int32Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);

    PropertyTablePropertyView<glm::ivec2, true> ivec2Invalid =
        view.getPropertyView<glm::ivec2, true>("TestClassProperty");
    REQUIRE(
        ivec2Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);

    PropertyTablePropertyView<glm::i32mat3x3, true> i32mat3x3Invalid =
        view.getPropertyView<glm::i32mat3x3, true>("TestClassProperty");
    REQUIRE(
        i32mat3x3Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Access wrong component type") {
    PropertyTablePropertyView<glm::u8vec3, true> u8vec3Invalid =
        view.getPropertyView<glm::u8vec3, true>("TestClassProperty");
    REQUIRE(
        u8vec3Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch);

    PropertyTablePropertyView<glm::i16vec3, true> i16vec3Invalid =
        view.getPropertyView<glm::i16vec3, true>("TestClassProperty");
    REQUIRE(
        i16vec3Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Access incorrectly as array") {
    PropertyTablePropertyView<PropertyArrayView<glm::ivec3>> ivec3ArrayInvalid =
        view.getPropertyView<PropertyArrayView<glm::ivec3>>(
            "TestClassProperty");
    REQUIRE(
        ivec3ArrayInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Access incorrectly as non-normalized") {
    PropertyTablePropertyView<glm::ivec3> nonNormalizedInvalid =
        view.getPropertyView<glm::ivec3>("TestClassProperty");
    REQUIRE(
        nonNormalizedInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Access incorrectly as dvec3") {
    PropertyTablePropertyView<glm::dvec3> dvec3Invalid =
        view.getPropertyView<glm::dvec3>("TestClassProperty");
    REQUIRE(
        dvec3Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch);
  }
}

TEST_CASE("Test matN PropertyTableProperty") {
  Model model;
  // clang-format off
  std::vector<glm::umat2x2> values = {
      glm::umat2x2(
        12, 34,
        30, 1),
      glm::umat2x2(
        11, 8,
        73, 102),
      glm::umat2x2(
        1, 0,
        63, 2),
      glm::umat2x2(
        4, 8,
        3, 23)};
  // clang-format on

  addBufferToModel(model, values);
  size_t valueBufferIndex = model.buffers.size() - 1;
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::MAT2;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT32;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(values.size());

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::MAT2);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT32);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(!classProperty->normalized);

  SUBCASE("Access correct type") {
    PropertyTablePropertyView<glm::umat2x2> umat2x2Property =
        view.getPropertyView<glm::umat2x2>("TestClassProperty");
    REQUIRE(umat2x2Property.status() == PropertyTablePropertyViewStatus::Valid);
    REQUIRE(umat2x2Property.size() > 0);

    for (int64_t i = 0; i < umat2x2Property.size(); ++i) {
      REQUIRE(umat2x2Property.getRaw(i) == values[static_cast<size_t>(i)]);
      REQUIRE(umat2x2Property.get(i) == umat2x2Property.getRaw(i));
    }
  }

  SUBCASE("Access wrong type") {
    PropertyTablePropertyView<uint32_t> uint32Invalid =
        view.getPropertyView<uint32_t>("TestClassProperty");
    REQUIRE(
        uint32Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);

    PropertyTablePropertyView<glm::uvec2> uvec2Invalid =
        view.getPropertyView<glm::uvec2>("TestClassProperty");
    REQUIRE(
        uvec2Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);

    PropertyTablePropertyView<glm::umat4x4> umat4x4Invalid =
        view.getPropertyView<glm::umat4x4>("TestClassProperty");
    REQUIRE(
        umat4x4Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);

    PropertyTablePropertyView<bool> boolInvalid =
        view.getPropertyView<bool>("TestClassProperty");
    REQUIRE(
        boolInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);

    PropertyTablePropertyView<std::string_view> stringInvalid =
        view.getPropertyView<std::string_view>("TestClassProperty");
    REQUIRE(
        stringInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Access wrong component type") {
    PropertyTablePropertyView<glm::u8mat2x2> u8mat2x2Invalid =
        view.getPropertyView<glm::u8mat2x2>("TestClassProperty");
    REQUIRE(
        u8mat2x2Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch);

    PropertyTablePropertyView<glm::imat2x2> imat2x2Invalid =
        view.getPropertyView<glm::imat2x2>("TestClassProperty");
    REQUIRE(
        imat2x2Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch);

    PropertyTablePropertyView<glm::mat2> mat2Invalid =
        view.getPropertyView<glm::mat2>("TestClassProperty");
    REQUIRE(
        mat2Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Access incorrectly as array") {
    PropertyTablePropertyView<PropertyArrayView<glm::umat2x2>> arrayInvalid =
        view.getPropertyView<PropertyArrayView<glm::umat2x2>>(
            "TestClassProperty");
    REQUIRE(
        arrayInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Access incorrectly as normalized") {
    PropertyTablePropertyView<glm::umat2x2, true> normalizedInvalid =
        view.getPropertyView<glm::umat2x2, true>("TestClassProperty");
    REQUIRE(
        normalizedInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Wrong buffer index") {
    model.bufferViews[valueBufferViewIndex].buffer = 2;
    PropertyTablePropertyView<glm::umat2x2> umat2x2Property =
        view.getPropertyView<glm::umat2x2>("TestClassProperty");
    REQUIRE(
        umat2x2Property.status() ==
        PropertyTablePropertyViewStatus::ErrorInvalidValueBuffer);
  }

  SUBCASE("Wrong buffer view index") {
    propertyTableProperty.values = -1;
    PropertyTablePropertyView<glm::umat2x2> umat2x2Property =
        view.getPropertyView<glm::umat2x2>("TestClassProperty");
    REQUIRE(
        umat2x2Property.status() ==
        PropertyTablePropertyViewStatus::ErrorInvalidValueBufferView);
  }

  SUBCASE("Buffer view points outside of the real buffer length") {
    model.buffers[valueBufferIndex].cesium.data.resize(sizeof(glm::umat2x2));
    PropertyTablePropertyView<glm::umat2x2> umat2x2Property =
        view.getPropertyView<glm::umat2x2>("TestClassProperty");
    REQUIRE(
        umat2x2Property.status() ==
        PropertyTablePropertyViewStatus::ErrorBufferViewOutOfBounds);
  }

  SUBCASE("Buffer view length isn't multiple of sizeof(T)") {
    model.bufferViews[valueBufferViewIndex].byteLength =
        sizeof(glm::umat2x2) * 4 - 1;
    PropertyTablePropertyView<glm::umat2x2> umat2x2Property =
        view.getPropertyView<glm::umat2x2>("TestClassProperty");
    REQUIRE(
        umat2x2Property.status() ==
        PropertyTablePropertyViewStatus::
            ErrorBufferViewSizeNotDivisibleByTypeSize);
  }

  SUBCASE("Buffer view length doesn't match with propertyTableCount") {
    model.bufferViews[valueBufferViewIndex].byteLength = sizeof(glm::umat2x2);

    PropertyTablePropertyView<glm::umat2x2> umat2x2Property =
        view.getPropertyView<glm::umat2x2>("TestClassProperty");
    REQUIRE(
        umat2x2Property.status() ==
        PropertyTablePropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);
  }
}

TEST_CASE("Test matN PropertyTableProperty (normalized)") {
  Model model;
  // clang-format off
  std::vector<glm::umat2x2> values = {
      glm::umat2x2(
        12, 34,
        30, 1),
      glm::umat2x2(
        11, 8,
        73, 102),
      glm::umat2x2(
        1, 0,
        63, 2),
      glm::umat2x2(
        4, 8,
        3, 23)};
  // clang-format on

  addBufferToModel(model, values);
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::MAT2;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT32;
  testClassProperty.normalized = true;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(values.size());

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::MAT2);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT32);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(classProperty->normalized);

  SUBCASE("Access correct type") {
    PropertyTablePropertyView<glm::umat2x2, true> umat2x2Property =
        view.getPropertyView<glm::umat2x2, true>("TestClassProperty");
    REQUIRE(umat2x2Property.status() == PropertyTablePropertyViewStatus::Valid);
    REQUIRE(umat2x2Property.size() > 0);

    for (int64_t i = 0; i < umat2x2Property.size(); ++i) {
      auto value = umat2x2Property.getRaw(i);
      REQUIRE(value == values[static_cast<size_t>(i)]);
      REQUIRE(umat2x2Property.get(i) == normalize(value));
    }
  }

  SUBCASE("Access wrong type") {
    PropertyTablePropertyView<uint32_t, true> uint32Invalid =
        view.getPropertyView<uint32_t, true>("TestClassProperty");
    REQUIRE(
        uint32Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);

    PropertyTablePropertyView<glm::uvec2, true> uvec2Invalid =
        view.getPropertyView<glm::uvec2, true>("TestClassProperty");
    REQUIRE(
        uvec2Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);

    PropertyTablePropertyView<glm::umat4x4, true> umat4x4Invalid =
        view.getPropertyView<glm::umat4x4, true>("TestClassProperty");
    REQUIRE(
        umat4x4Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Access wrong component type") {
    PropertyTablePropertyView<glm::u8mat2x2, true> u8mat2x2Invalid =
        view.getPropertyView<glm::u8mat2x2, true>("TestClassProperty");
    REQUIRE(
        u8mat2x2Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch);

    PropertyTablePropertyView<glm::imat2x2, true> imat2x2Invalid =
        view.getPropertyView<glm::imat2x2, true>("TestClassProperty");
    REQUIRE(
        imat2x2Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Access incorrectly as array") {
    PropertyTablePropertyView<PropertyArrayView<glm::umat2x2>, true>
        arrayInvalid =
            view.getPropertyView<PropertyArrayView<glm::umat2x2>, true>(
                "TestClassProperty");
    REQUIRE(
        arrayInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Access incorrectly as non-normalized") {
    PropertyTablePropertyView<glm::umat2x2> nonNormalizedInvalid =
        view.getPropertyView<glm::umat2x2>("TestClassProperty");
    REQUIRE(
        nonNormalizedInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Access incorrectly as dmat2") {
    PropertyTablePropertyView<glm::dmat2> dmat2Invalid =
        view.getPropertyView<glm::dmat2>("TestClassProperty");
    REQUIRE(
        dmat2Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch);
  }
}

TEST_CASE("Test boolean PropertyTableProperty") {
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
  addBufferToModel(model, values);

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::BOOLEAN;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(instanceCount);

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values =
      static_cast<int32_t>(model.bufferViews.size() - 1);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::BOOLEAN);
  REQUIRE(classProperty->componentType == std::nullopt);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);

  SUBCASE("Access correct type") {
    PropertyTablePropertyView<bool> boolProperty =
        view.getPropertyView<bool>("TestClassProperty");
    REQUIRE(boolProperty.status() == PropertyTablePropertyViewStatus::Valid);
    REQUIRE(boolProperty.size() == instanceCount);
    for (int64_t i = 0; i < boolProperty.size(); ++i) {
      bool expectedValue = expected[static_cast<size_t>(i)];
      REQUIRE(boolProperty.getRaw(i) == expectedValue);
      REQUIRE(boolProperty.get(i) == expectedValue);
    }
  }

  SUBCASE("Buffer size doesn't match with propertyTableCount") {
    propertyTable.count = 66;
    PropertyTablePropertyView<bool> boolProperty =
        view.getPropertyView<bool>("TestClassProperty");
    REQUIRE(
        boolProperty.status() ==
        PropertyTablePropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);
  }
}

TEST_CASE("Test string PropertyTableProperty") {
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

  addBufferToModel(model, values);
  size_t valueBufferIndex = model.buffers.size() - 1;
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  addBufferToModel(model, stringOffsets);
  size_t offsetBufferIndex = model.buffers.size() - 1;
  size_t offsetBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::STRING;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(expected.size());

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.stringOffsetType =
      PropertyTableProperty::StringOffsetType::UINT32;
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);
  propertyTableProperty.stringOffsets =
      static_cast<int32_t>(offsetBufferViewIndex);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::STRING);
  REQUIRE(classProperty->componentType == std::nullopt);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);

  SUBCASE("Access correct type") {
    PropertyTablePropertyView<std::string_view> stringProperty =
        view.getPropertyView<std::string_view>("TestClassProperty");
    REQUIRE(stringProperty.status() == PropertyTablePropertyViewStatus::Valid);
    for (size_t i = 0; i < expected.size(); ++i) {
      REQUIRE(stringProperty.getRaw(static_cast<int64_t>(i)) == expected[i]);
      REQUIRE(stringProperty.get(static_cast<int64_t>(i)) == expected[i]);
    }
  }

  SUBCASE("Wrong array type") {
    PropertyTablePropertyView<PropertyArrayView<std::string_view>>
        stringArrayInvalid =
            view.getPropertyView<PropertyArrayView<std::string_view>>(
                "TestClassProperty");
    REQUIRE(
        stringArrayInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Wrong offset type") {
    propertyTableProperty.stringOffsetType =
        PropertyTableProperty::StringOffsetType::UINT8;
    PropertyTablePropertyView<std::string_view> stringProperty =
        view.getPropertyView<std::string_view>("TestClassProperty");
    REQUIRE(
        stringProperty.status() ==
        PropertyTablePropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);

    propertyTableProperty.stringOffsetType =
        PropertyTableProperty::StringOffsetType::UINT64;
    stringProperty =
        view.getPropertyView<std::string_view>("TestClassProperty");
    REQUIRE(
        stringProperty.status() ==
        PropertyTablePropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);

    propertyTableProperty.stringOffsetType = "NONSENSE";
    stringProperty =
        view.getPropertyView<std::string_view>("TestClassProperty");
    REQUIRE(
        stringProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorInvalidStringOffsetType);

    propertyTableProperty.stringOffsetType = "";
    propertyTableProperty.arrayOffsetType =
        PropertyTableProperty::StringOffsetType::UINT32;
    stringProperty =
        view.getPropertyView<std::string_view>("TestClassProperty");
    REQUIRE(
        stringProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorInvalidStringOffsetType);
  }

  SUBCASE("Offset values are not sorted ascending") {
    uint32_t* offset = reinterpret_cast<uint32_t*>(
        model.buffers[offsetBufferIndex].cesium.data.data());
    offset[2] =
        static_cast<uint32_t>(model.buffers[valueBufferIndex].byteLength + 4);
    PropertyTablePropertyView<std::string_view> stringProperty =
        view.getPropertyView<std::string_view>("TestClassProperty");
    REQUIRE(
        stringProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorStringOffsetsNotSorted);
  }

  SUBCASE("Offset value points outside of value buffer") {
    uint32_t* offset = reinterpret_cast<uint32_t*>(
        model.buffers[offsetBufferIndex].cesium.data.data());
    offset[propertyTable.count] =
        static_cast<uint32_t>(model.buffers[valueBufferIndex].byteLength + 4);
    PropertyTablePropertyView<std::string_view> stringProperty =
        view.getPropertyView<std::string_view>("TestClassProperty");
    REQUIRE(
        stringProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorStringOffsetOutOfBounds);
  }
}

TEST_CASE("Test fixed-length scalar array") {
  Model model;
  std::vector<uint32_t> values =
      {12, 34, 30, 11, 34, 34, 11, 33, 122, 33, 223, 11};

  addBufferToModel(model, values);
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT32;
  testClassProperty.array = true;
  testClassProperty.count = 3;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(
      values.size() / static_cast<size_t>(testClassProperty.count.value()));

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values =
      static_cast<int32_t>(model.bufferViews.size() - 1);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT32);
  REQUIRE(classProperty->array);
  REQUIRE(classProperty->count == 3);
  REQUIRE(!classProperty->normalized);

  SUBCASE("Access the right type") {
    PropertyTablePropertyView<PropertyArrayView<uint32_t>> arrayProperty =
        view.getPropertyView<PropertyArrayView<uint32_t>>("TestClassProperty");
    REQUIRE(arrayProperty.status() == PropertyTablePropertyViewStatus::Valid);

    for (int64_t i = 0; i < arrayProperty.size(); ++i) {
      PropertyArrayView<uint32_t> array = arrayProperty.getRaw(i);
      auto maybeArray = arrayProperty.get(i);
      REQUIRE(maybeArray);

      for (int64_t j = 0; j < array.size(); ++j) {
        REQUIRE(array[j] == values[static_cast<size_t>(i * 3 + j)]);
        REQUIRE((*maybeArray)[j] == array[j]);
      }
    }
  }

  SUBCASE("Wrong type") {
    PropertyTablePropertyView<PropertyArrayView<bool>> boolArrayInvalid =
        view.getPropertyView<PropertyArrayView<bool>>("TestClassProperty");
    REQUIRE(
        boolArrayInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);

    PropertyTablePropertyView<PropertyArrayView<glm::uvec2>> uvec2ArrayInvalid =
        view.getPropertyView<PropertyArrayView<glm::uvec2>>(
            "TestClassProperty");
    REQUIRE(
        uvec2ArrayInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Wrong component type") {
    PropertyTablePropertyView<PropertyArrayView<int32_t>> int32ArrayInvalid =
        view.getPropertyView<PropertyArrayView<int32_t>>("TestClassProperty");
    REQUIRE(
        int32ArrayInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Not an array type") {
    PropertyTablePropertyView<uint32_t> uint32Invalid =
        view.getPropertyView<uint32_t>("TestClassProperty");
    REQUIRE(
        uint32Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Incorrectly normalized") {
    PropertyTablePropertyView<PropertyArrayView<uint32_t>, true>
        normalizedInvalid =
            view.getPropertyView<PropertyArrayView<uint32_t>, true>(
                "TestClassProperty");
    REQUIRE(
        normalizedInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Buffer size is not a multiple of type size") {
    model.bufferViews[valueBufferViewIndex].byteLength = 13;
    PropertyTablePropertyView<PropertyArrayView<uint32_t>> arrayProperty =
        view.getPropertyView<PropertyArrayView<uint32_t>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::
            ErrorBufferViewSizeNotDivisibleByTypeSize);
  }

  SUBCASE("Negative count") {
    testClassProperty.count = -1;
    PropertyTablePropertyView<PropertyArrayView<uint32_t>> arrayProperty =
        view.getPropertyView<PropertyArrayView<uint32_t>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() == PropertyTablePropertyViewStatus::
                                      ErrorArrayCountAndOffsetBufferDontExist);
  }

  SUBCASE("Value buffer doesn't fit into property table count") {
    testClassProperty.count = 55;
    PropertyTablePropertyView<PropertyArrayView<uint32_t>> arrayProperty =
        view.getPropertyView<PropertyArrayView<uint32_t>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);
  }
}

TEST_CASE("Test fixed-length scalar array (normalized)") {
  Model model;
  std::vector<uint32_t> values =
      {12, 34, 30, 11, 34, 34, 11, 33, 122, 33, 223, 11};

  addBufferToModel(model, values);

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT32;
  testClassProperty.array = true;
  testClassProperty.count = 3;
  testClassProperty.normalized = true;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(
      values.size() / static_cast<size_t>(testClassProperty.count.value()));

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values =
      static_cast<int32_t>(model.bufferViews.size() - 1);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT32);
  REQUIRE(classProperty->array);
  REQUIRE(classProperty->count == 3);
  REQUIRE(classProperty->normalized);

  SUBCASE("Access the right type") {
    PropertyTablePropertyView<PropertyArrayView<uint32_t>, true> arrayProperty =
        view.getPropertyView<PropertyArrayView<uint32_t>, true>(
            "TestClassProperty");
    REQUIRE(arrayProperty.status() == PropertyTablePropertyViewStatus::Valid);

    for (int64_t i = 0; i < arrayProperty.size(); ++i) {
      PropertyArrayView<uint32_t> array = arrayProperty.getRaw(i);
      auto maybeArray = arrayProperty.get(i);
      REQUIRE(maybeArray);

      for (int64_t j = 0; j < array.size(); ++j) {
        REQUIRE(array[j] == values[static_cast<size_t>(i * 3 + j)]);
        REQUIRE((*maybeArray)[j] == normalize(array[j]));
      }
    }
  }

  SUBCASE("Wrong type") {
    PropertyTablePropertyView<PropertyArrayView<glm::uvec2>, true>
        uvec2ArrayInvalid =
            view.getPropertyView<PropertyArrayView<glm::uvec2>, true>(
                "TestClassProperty");
    REQUIRE(
        uvec2ArrayInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Wrong component type") {
    PropertyTablePropertyView<PropertyArrayView<int32_t>, true>
        int32ArrayInvalid =
            view.getPropertyView<PropertyArrayView<int32_t>, true>(
                "TestClassProperty");
    REQUIRE(
        int32ArrayInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Not an array type") {
    PropertyTablePropertyView<uint32_t, true> uint32Invalid =
        view.getPropertyView<uint32_t, true>("TestClassProperty");
    REQUIRE(
        uint32Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Incorrectly non-normalized") {
    PropertyTablePropertyView<PropertyArrayView<uint32_t>>
        nonNormalizedInvalid =
            view.getPropertyView<PropertyArrayView<uint32_t>>(
                "TestClassProperty");
    REQUIRE(
        nonNormalizedInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorNormalizationMismatch);
  }
}

TEST_CASE("Test variable-length scalar array") {
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
        values.data() + offsetValue[i] * sizeof(uint16_t),
        expected[i].data(),
        expected[i].size() * sizeof(uint16_t));
    offsetValue[i + 1] = offsetValue[i] + expected[i].size();
  }

  addBufferToModel(model, values);
  size_t valueBufferIndex = model.buffers.size() - 1;
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  addBufferToModel(model, offsets);
  size_t offsetBufferIndex = model.buffers.size() - 1;
  size_t offsetBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT16;
  testClassProperty.array = true;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(expected.size());

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);
  propertyTableProperty.arrayOffsets =
      static_cast<int32_t>(offsetBufferViewIndex);
  propertyTableProperty.arrayOffsetType =
      PropertyTableProperty::ArrayOffsetType::UINT64;

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT16);
  REQUIRE(classProperty->array);
  REQUIRE(!classProperty->count);
  REQUIRE(!classProperty->normalized);

  SUBCASE("Access the correct type") {
    PropertyTablePropertyView<PropertyArrayView<uint16_t>> property =
        view.getPropertyView<PropertyArrayView<uint16_t>>("TestClassProperty");
    REQUIRE(property.status() == PropertyTablePropertyViewStatus::Valid);
    for (size_t i = 0; i < expected.size(); ++i) {
      PropertyArrayView<uint16_t> array =
          property.getRaw(static_cast<int64_t>(i));
      REQUIRE(array.size() == static_cast<int64_t>(expected[i].size()));

      auto maybeArray = property.get(static_cast<int64_t>(i));
      REQUIRE(maybeArray);
      REQUIRE(maybeArray->size() == array.size());

      for (size_t j = 0; j < expected[i].size(); ++j) {
        REQUIRE(expected[i][j] == array[static_cast<int64_t>(j)]);
        REQUIRE(
            (*maybeArray)[static_cast<int64_t>(j)] ==
            array[static_cast<int64_t>(j)]);
      }
    }
  }

  SUBCASE("Incorrectly normalized") {
    PropertyTablePropertyView<PropertyArrayView<uint16_t>, true> property =
        view.getPropertyView<PropertyArrayView<uint16_t>, true>(
            "TestClassProperty");
    REQUIRE(
        property.status() ==
        PropertyTablePropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Wrong offset type") {
    propertyTableProperty.arrayOffsetType =
        PropertyTableProperty::ArrayOffsetType::UINT8;
    PropertyTablePropertyView<PropertyArrayView<uint16_t>> arrayProperty =
        view.getPropertyView<PropertyArrayView<uint16_t>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);

    propertyTableProperty.arrayOffsetType =
        PropertyTableProperty::ArrayOffsetType::UINT16;
    arrayProperty =
        view.getPropertyView<PropertyArrayView<uint16_t>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);

    propertyTableProperty.arrayOffsetType = "NONSENSE";
    arrayProperty =
        view.getPropertyView<PropertyArrayView<uint16_t>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorInvalidArrayOffsetType);

    propertyTableProperty.arrayOffsetType = "";
    propertyTableProperty.stringOffsetType =
        PropertyTableProperty::StringOffsetType::UINT64;
    arrayProperty =
        view.getPropertyView<PropertyArrayView<uint16_t>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorInvalidArrayOffsetType);
  }

  SUBCASE("Offset values are not sorted ascending") {
    propertyTableProperty.arrayOffsetType =
        PropertyTableProperty::ArrayOffsetType::UINT64;
    uint64_t* offset = reinterpret_cast<uint64_t*>(
        model.buffers[offsetBufferIndex].cesium.data.data());
    offset[propertyTable.count] = 0;
    PropertyTablePropertyView<PropertyArrayView<uint16_t>> arrayProperty =
        view.getPropertyView<PropertyArrayView<uint16_t>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorArrayOffsetsNotSorted);
  }

  SUBCASE("Offset value points outside of value buffer") {
    uint64_t* offset = reinterpret_cast<uint64_t*>(
        model.buffers[offsetBufferIndex].cesium.data.data());
    offset[propertyTable.count] =
        static_cast<uint32_t>(model.buffers[valueBufferIndex].byteLength + 4);
    PropertyTablePropertyView<PropertyArrayView<uint16_t>> arrayProperty =
        view.getPropertyView<PropertyArrayView<uint16_t>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorArrayOffsetOutOfBounds);
  }

  SUBCASE("Count and offset buffer are both present") {
    testClassProperty.count = 3;
    PropertyTablePropertyView<PropertyArrayView<uint16_t>> property =
        view.getPropertyView<PropertyArrayView<uint16_t>>("TestClassProperty");
    REQUIRE(
        property.status() ==
        PropertyTablePropertyViewStatus::ErrorArrayCountAndOffsetBufferCoexist);
  }
}

TEST_CASE("Test variable-length scalar array (normalized)") {
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
        values.data() + offsetValue[i] * sizeof(uint16_t),
        expected[i].data(),
        expected[i].size() * sizeof(uint16_t));
    offsetValue[i + 1] = offsetValue[i] + expected[i].size();
  }

  addBufferToModel(model, values);
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  addBufferToModel(model, offsets);
  size_t offsetBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT16;
  testClassProperty.array = true;
  testClassProperty.normalized = true;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(expected.size());

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);
  propertyTableProperty.arrayOffsets =
      static_cast<int32_t>(offsetBufferViewIndex);
  propertyTableProperty.arrayOffsetType =
      PropertyTableProperty::ArrayOffsetType::UINT64;

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT16);
  REQUIRE(classProperty->array);
  REQUIRE(!classProperty->count);
  REQUIRE(classProperty->normalized);

  SUBCASE("Access the correct type") {
    PropertyTablePropertyView<PropertyArrayView<uint16_t>, true> property =
        view.getPropertyView<PropertyArrayView<uint16_t>, true>(
            "TestClassProperty");
    REQUIRE(property.status() == PropertyTablePropertyViewStatus::Valid);
    for (size_t i = 0; i < expected.size(); ++i) {
      PropertyArrayView<uint16_t> array =
          property.getRaw(static_cast<int64_t>(i));
      REQUIRE(array.size() == static_cast<int64_t>(expected[i].size()));

      auto maybeArray = property.get(static_cast<int64_t>(i));
      REQUIRE(maybeArray);
      REQUIRE(maybeArray->size() == array.size());

      for (size_t j = 0; j < expected[i].size(); ++j) {
        auto value = array[static_cast<int64_t>(j)];
        REQUIRE(expected[i][j] == value);
        REQUIRE((*maybeArray)[static_cast<int64_t>(j)] == normalize(value));
      }
    }
  }

  SUBCASE("Incorrectly non-normalized") {
    PropertyTablePropertyView<PropertyArrayView<uint16_t>> property =
        view.getPropertyView<PropertyArrayView<uint16_t>>("TestClassProperty");
    REQUIRE(
        property.status() ==
        PropertyTablePropertyViewStatus::ErrorNormalizationMismatch);
  }
}

TEST_CASE("Test fixed-length vecN array") {
  Model model;
  std::vector<glm::ivec3> values = {
      glm::ivec3(12, 34, -30),
      glm::ivec3(-2, 0, 1),
      glm::ivec3(1, 2, 8),
      glm::ivec3(-100, 84, 6),
      glm::ivec3(2, -2, -2),
      glm::ivec3(40, 61, 3),
  };

  addBufferToModel(model, values);
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::VEC3;
  testClassProperty.componentType = ClassProperty::ComponentType::INT32;
  testClassProperty.array = true;
  testClassProperty.count = 2;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(
      values.size() / static_cast<size_t>(testClassProperty.count.value()));

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values =
      static_cast<int32_t>(model.bufferViews.size() - 1);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::VEC3);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT32);
  REQUIRE(classProperty->array);
  REQUIRE(classProperty->count == 2);
  REQUIRE(!classProperty->normalized);

  SUBCASE("Access the right type") {
    PropertyTablePropertyView<PropertyArrayView<glm::ivec3>> arrayProperty =
        view.getPropertyView<PropertyArrayView<glm::ivec3>>(
            "TestClassProperty");
    REQUIRE(arrayProperty.status() == PropertyTablePropertyViewStatus::Valid);

    for (int64_t i = 0; i < arrayProperty.size(); ++i) {
      PropertyArrayView<glm::ivec3> array = arrayProperty.getRaw(i);
      auto maybeArray = arrayProperty.get(i);
      REQUIRE(maybeArray);

      for (int64_t j = 0; j < array.size(); ++j) {
        REQUIRE(array[j] == values[static_cast<size_t>(i * 2 + j)]);
        REQUIRE((*maybeArray)[j] == array[j]);
      }
    }
  }

  SUBCASE("Wrong type") {
    PropertyTablePropertyView<PropertyArrayView<int32_t>> int32ArrayInvalid =
        view.getPropertyView<PropertyArrayView<int32_t>>("TestClassProperty");
    REQUIRE(
        int32ArrayInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);

    PropertyTablePropertyView<PropertyArrayView<glm::ivec2>> ivec2ArrayInvalid =
        view.getPropertyView<PropertyArrayView<glm::ivec2>>(
            "TestClassProperty");
    REQUIRE(
        ivec2ArrayInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Wrong component type") {
    PropertyTablePropertyView<PropertyArrayView<glm::uvec3>> uvec3ArrayInvalid =
        view.getPropertyView<PropertyArrayView<glm::uvec3>>(
            "TestClassProperty");
    REQUIRE(
        uvec3ArrayInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Not an array type") {
    PropertyTablePropertyView<glm::ivec3> ivec3Invalid =
        view.getPropertyView<glm::ivec3>("TestClassProperty");
    REQUIRE(
        ivec3Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Incorrect normalization") {
    PropertyTablePropertyView<PropertyArrayView<glm::ivec3>, true>
        normalizedInvalid =
            view.getPropertyView<PropertyArrayView<glm::ivec3>, true>(
                "TestClassProperty");
    REQUIRE(
        normalizedInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Buffer size is not a multiple of type size") {
    model.bufferViews[valueBufferViewIndex].byteLength = 13;
    PropertyTablePropertyView<PropertyArrayView<glm::ivec3>> arrayProperty =
        view.getPropertyView<PropertyArrayView<glm::ivec3>>(
            "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::
            ErrorBufferViewSizeNotDivisibleByTypeSize);
  }

  SUBCASE("Negative count") {
    testClassProperty.count = -1;
    PropertyTablePropertyView<PropertyArrayView<glm::ivec3>> arrayProperty =
        view.getPropertyView<PropertyArrayView<glm::ivec3>>(
            "TestClassProperty");
    REQUIRE(
        arrayProperty.status() == PropertyTablePropertyViewStatus::
                                      ErrorArrayCountAndOffsetBufferDontExist);
  }

  SUBCASE("Value buffer doesn't fit into property table count") {
    testClassProperty.count = 55;
    PropertyTablePropertyView<PropertyArrayView<glm::ivec3>> arrayProperty =
        view.getPropertyView<PropertyArrayView<glm::ivec3>>(
            "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);
  }
}

TEST_CASE("Test fixed-length vecN array (normalized)") {
  Model model;
  std::vector<glm::ivec3> values = {
      glm::ivec3(12, 34, -30),
      glm::ivec3(-2, 0, 1),
      glm::ivec3(1, 2, 8),
      glm::ivec3(-100, 84, 6),
      glm::ivec3(2, -2, -2),
      glm::ivec3(40, 61, 3),
  };

  addBufferToModel(model, values);

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::VEC3;
  testClassProperty.componentType = ClassProperty::ComponentType::INT32;
  testClassProperty.array = true;
  testClassProperty.count = 2;
  testClassProperty.normalized = true;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(
      values.size() / static_cast<size_t>(testClassProperty.count.value()));

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values =
      static_cast<int32_t>(model.bufferViews.size() - 1);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::VEC3);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT32);
  REQUIRE(classProperty->array);
  REQUIRE(classProperty->count == 2);
  REQUIRE(classProperty->normalized);

  SUBCASE("Access the right type") {
    PropertyTablePropertyView<PropertyArrayView<glm::ivec3>, true>
        arrayProperty =
            view.getPropertyView<PropertyArrayView<glm::ivec3>, true>(
                "TestClassProperty");
    REQUIRE(arrayProperty.status() == PropertyTablePropertyViewStatus::Valid);

    for (int64_t i = 0; i < arrayProperty.size(); ++i) {
      PropertyArrayView<glm::ivec3> array = arrayProperty.getRaw(i);
      auto maybeArray = arrayProperty.get(i);
      REQUIRE(maybeArray);

      for (int64_t j = 0; j < array.size(); ++j) {
        REQUIRE(array[j] == values[static_cast<size_t>(i * 2 + j)]);
        REQUIRE((*maybeArray)[j] == normalize(array[j]));
      }
    }
  }

  SUBCASE("Wrong type") {
    PropertyTablePropertyView<PropertyArrayView<int32_t>, true>
        int32ArrayInvalid =
            view.getPropertyView<PropertyArrayView<int32_t>, true>(
                "TestClassProperty");
    REQUIRE(
        int32ArrayInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);

    PropertyTablePropertyView<PropertyArrayView<glm::ivec2>, true>
        ivec2ArrayInvalid =
            view.getPropertyView<PropertyArrayView<glm::ivec2>, true>(
                "TestClassProperty");
    REQUIRE(
        ivec2ArrayInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Wrong component type") {
    PropertyTablePropertyView<PropertyArrayView<glm::uvec3>, true>
        uvec3ArrayInvalid =
            view.getPropertyView<PropertyArrayView<glm::uvec3>, true>(
                "TestClassProperty");
    REQUIRE(
        uvec3ArrayInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Not an array type") {
    PropertyTablePropertyView<glm::ivec3, true> ivec3Invalid =
        view.getPropertyView<glm::ivec3, true>("TestClassProperty");
    REQUIRE(
        ivec3Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Incorrect non-normalization") {
    PropertyTablePropertyView<PropertyArrayView<glm::ivec3>>
        nonNormalizedInvalid =
            view.getPropertyView<PropertyArrayView<glm::ivec3>>(
                "TestClassProperty");
    REQUIRE(
        nonNormalizedInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorNormalizationMismatch);
  }
}

TEST_CASE("Test variable-length vecN array") {
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
        values.data() + offsetValue[i] * sizeof(glm::ivec3),
        expected[i].data(),
        expected[i].size() * sizeof(glm::ivec3));
    offsetValue[i + 1] = offsetValue[i] + expected[i].size();
  }

  addBufferToModel(model, values);
  size_t valueBufferIndex = model.buffers.size() - 1;
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  addBufferToModel(model, offsets);
  size_t offsetBufferIndex = model.buffers.size() - 1;
  size_t offsetBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::VEC3;
  testClassProperty.componentType = ClassProperty::ComponentType::INT32;
  testClassProperty.array = true;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(expected.size());

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);
  propertyTableProperty.arrayOffsets =
      static_cast<int32_t>(offsetBufferViewIndex);
  propertyTableProperty.arrayOffsetType =
      PropertyTableProperty::ArrayOffsetType::UINT64;

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::VEC3);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT32);
  REQUIRE(classProperty->array);
  REQUIRE(!classProperty->count);
  REQUIRE(!classProperty->normalized);

  SUBCASE("Access the correct type") {
    PropertyTablePropertyView<PropertyArrayView<glm::ivec3>> property =
        view.getPropertyView<PropertyArrayView<glm::ivec3>>(
            "TestClassProperty");
    REQUIRE(property.status() == PropertyTablePropertyViewStatus::Valid);
    for (size_t i = 0; i < expected.size(); ++i) {
      PropertyArrayView<glm::ivec3> array =
          property.getRaw(static_cast<int64_t>(i));
      REQUIRE(array.size() == static_cast<int64_t>(expected[i].size()));

      auto maybeArray = property.get(static_cast<int64_t>(i));
      REQUIRE(maybeArray);
      for (size_t j = 0; j < expected[i].size(); ++j) {
        auto value = array[static_cast<int64_t>(j)];
        REQUIRE(expected[i][j] == value);
        REQUIRE((*maybeArray)[static_cast<int64_t>(j)] == value);
      }
    }
  }

  SUBCASE("Incorrectly normalized") {
    PropertyTablePropertyView<PropertyArrayView<glm::ivec3>, true> property =
        view.getPropertyView<PropertyArrayView<glm::ivec3>, true>(
            "TestClassProperty");
    REQUIRE(
        property.status() ==
        PropertyTablePropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Wrong offset type") {
    propertyTableProperty.arrayOffsetType =
        PropertyTableProperty::ArrayOffsetType::UINT8;
    PropertyTablePropertyView<PropertyArrayView<glm::ivec3>> arrayProperty =
        view.getPropertyView<PropertyArrayView<glm::ivec3>>(
            "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);

    propertyTableProperty.arrayOffsetType =
        PropertyTableProperty::ArrayOffsetType::UINT16;
    arrayProperty = view.getPropertyView<PropertyArrayView<glm::ivec3>>(
        "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);

    propertyTableProperty.arrayOffsetType = "NONSENSE";
    arrayProperty = view.getPropertyView<PropertyArrayView<glm::ivec3>>(
        "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorInvalidArrayOffsetType);

    propertyTableProperty.arrayOffsetType = "";
    propertyTableProperty.stringOffsetType =
        PropertyTableProperty::StringOffsetType::UINT64;
    arrayProperty = view.getPropertyView<PropertyArrayView<glm::ivec3>>(
        "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorInvalidArrayOffsetType);
  }

  SUBCASE("Offset values are not sorted ascending") {
    propertyTableProperty.arrayOffsetType =
        PropertyTableProperty::ArrayOffsetType::UINT64;
    uint64_t* offset = reinterpret_cast<uint64_t*>(
        model.buffers[offsetBufferIndex].cesium.data.data());
    offset[propertyTable.count] = 0;
    PropertyTablePropertyView<PropertyArrayView<glm::ivec3>> arrayProperty =
        view.getPropertyView<PropertyArrayView<glm::ivec3>>(
            "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorArrayOffsetsNotSorted);
  }

  SUBCASE("Offset value points outside of value buffer") {
    uint64_t* offset = reinterpret_cast<uint64_t*>(
        model.buffers[offsetBufferIndex].cesium.data.data());
    offset[propertyTable.count] =
        static_cast<uint32_t>(model.buffers[valueBufferIndex].byteLength + 4);
    PropertyTablePropertyView<PropertyArrayView<glm::ivec3>> arrayProperty =
        view.getPropertyView<PropertyArrayView<glm::ivec3>>(
            "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorArrayOffsetOutOfBounds);
  }

  SUBCASE("Count and offset buffer are both present") {
    testClassProperty.count = 3;
    PropertyTablePropertyView<PropertyArrayView<glm::ivec3>> property =
        view.getPropertyView<PropertyArrayView<glm::ivec3>>(
            "TestClassProperty");
    REQUIRE(
        property.status() ==
        PropertyTablePropertyViewStatus::ErrorArrayCountAndOffsetBufferCoexist);
  }
}

TEST_CASE("Test variable-length vecN array (normalized)") {
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
        values.data() + offsetValue[i] * sizeof(glm::ivec3),
        expected[i].data(),
        expected[i].size() * sizeof(glm::ivec3));
    offsetValue[i + 1] = offsetValue[i] + expected[i].size();
  }

  addBufferToModel(model, values);
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  addBufferToModel(model, offsets);
  size_t offsetBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::VEC3;
  testClassProperty.componentType = ClassProperty::ComponentType::INT32;
  testClassProperty.array = true;
  testClassProperty.normalized = true;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(expected.size());

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);
  propertyTableProperty.arrayOffsets =
      static_cast<int32_t>(offsetBufferViewIndex);
  propertyTableProperty.arrayOffsetType =
      PropertyTableProperty::ArrayOffsetType::UINT64;

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::VEC3);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT32);
  REQUIRE(classProperty->array);
  REQUIRE(!classProperty->count);
  REQUIRE(classProperty->normalized);

  SUBCASE("Access the correct type") {
    PropertyTablePropertyView<PropertyArrayView<glm::ivec3>, true> property =
        view.getPropertyView<PropertyArrayView<glm::ivec3>, true>(
            "TestClassProperty");
    REQUIRE(property.status() == PropertyTablePropertyViewStatus::Valid);
    for (size_t i = 0; i < expected.size(); ++i) {
      PropertyArrayView<glm::ivec3> array =
          property.getRaw(static_cast<int64_t>(i));
      REQUIRE(array.size() == static_cast<int64_t>(expected[i].size()));

      auto maybeArray = property.get(static_cast<int64_t>(i));
      REQUIRE(maybeArray);
      for (size_t j = 0; j < expected[i].size(); ++j) {
        auto value = array[static_cast<int64_t>(j)];
        REQUIRE(expected[i][j] == value);
        REQUIRE((*maybeArray)[static_cast<int64_t>(j)] == normalize(value));
      }
    }
  }

  SUBCASE("Incorrectly non-normalized") {
    PropertyTablePropertyView<PropertyArrayView<glm::ivec3>> property =
        view.getPropertyView<PropertyArrayView<glm::ivec3>>(
            "TestClassProperty");
    REQUIRE(
        property.status() ==
        PropertyTablePropertyViewStatus::ErrorNormalizationMismatch);
  }
}

TEST_CASE("Test fixed-length matN array") {
  Model model;
  // clang-format off
  std::vector<glm::imat2x2> values = {
      glm::imat2x2(
        12, 34,
        -30, 20),
      glm::imat2x2(
        -2, -2,
        0, 1),
      glm::imat2x2(
        1, 2,
        8, 5),
      glm::imat2x2(
        -100, 3,
        84, 6),
      glm::imat2x2(
        2, 12,
        -2, -2),
      glm::imat2x2(
        40, 61,
        7, -3),
  };
  // clang-format on

  addBufferToModel(model, values);
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::MAT2;
  testClassProperty.componentType = ClassProperty::ComponentType::INT32;
  testClassProperty.array = true;
  testClassProperty.count = 2;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(
      values.size() / static_cast<size_t>(testClassProperty.count.value()));

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values =
      static_cast<int32_t>(model.bufferViews.size() - 1);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::MAT2);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT32);
  REQUIRE(classProperty->array);
  REQUIRE(classProperty->count == 2);
  REQUIRE(!classProperty->normalized);

  SUBCASE("Access the right type") {
    PropertyTablePropertyView<PropertyArrayView<glm::imat2x2>> arrayProperty =
        view.getPropertyView<PropertyArrayView<glm::imat2x2>>(
            "TestClassProperty");
    REQUIRE(arrayProperty.status() == PropertyTablePropertyViewStatus::Valid);

    for (int64_t i = 0; i < arrayProperty.size(); ++i) {
      PropertyArrayView<glm::imat2x2> member = arrayProperty.getRaw(i);
      for (int64_t j = 0; j < member.size(); ++j) {
        REQUIRE(member[j] == values[static_cast<size_t>(i * 2 + j)]);
      }
    }
  }

  SUBCASE("Wrong type") {
    PropertyTablePropertyView<PropertyArrayView<int32_t>> int32ArrayInvalid =
        view.getPropertyView<PropertyArrayView<int32_t>>("TestClassProperty");
    REQUIRE(
        int32ArrayInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);

    PropertyTablePropertyView<PropertyArrayView<glm::ivec2>> ivec2ArrayInvalid =
        view.getPropertyView<PropertyArrayView<glm::ivec2>>(
            "TestClassProperty");
    REQUIRE(
        ivec2ArrayInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Wrong component type") {
    PropertyTablePropertyView<PropertyArrayView<glm::umat2x2>>
        umat2x2ArrayInvalid =
            view.getPropertyView<PropertyArrayView<glm::umat2x2>>(
                "TestClassProperty");
    REQUIRE(
        umat2x2ArrayInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Not an array type") {
    PropertyTablePropertyView<glm::imat2x2> ivec3Invalid =
        view.getPropertyView<glm::imat2x2>("TestClassProperty");
    REQUIRE(
        ivec3Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Incorrect normalization") {
    PropertyTablePropertyView<PropertyArrayView<glm::imat2x2>, true>
        normalizedInvalid =
            view.getPropertyView<PropertyArrayView<glm::imat2x2>, true>(
                "TestClassProperty");
    REQUIRE(
        normalizedInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Buffer size is not a multiple of type size") {
    model.bufferViews[valueBufferViewIndex].byteLength = 13;
    PropertyTablePropertyView<PropertyArrayView<glm::imat2x2>> arrayProperty =
        view.getPropertyView<PropertyArrayView<glm::imat2x2>>(
            "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::
            ErrorBufferViewSizeNotDivisibleByTypeSize);
  }

  SUBCASE("Negative count") {
    testClassProperty.count = -1;
    PropertyTablePropertyView<PropertyArrayView<glm::imat2x2>> arrayProperty =
        view.getPropertyView<PropertyArrayView<glm::imat2x2>>(
            "TestClassProperty");
    REQUIRE(
        arrayProperty.status() == PropertyTablePropertyViewStatus::
                                      ErrorArrayCountAndOffsetBufferDontExist);
  }

  SUBCASE("Value buffer doesn't fit into property table count") {
    testClassProperty.count = 55;
    PropertyTablePropertyView<PropertyArrayView<glm::imat2x2>> arrayProperty =
        view.getPropertyView<PropertyArrayView<glm::imat2x2>>(
            "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);
  }
}

TEST_CASE("Test fixed-length matN array (normalized)") {
  Model model;
  // clang-format off
  std::vector<glm::imat2x2> values = {
      glm::imat2x2(
        12, 34,
        -30, 20),
      glm::imat2x2(
        -2, -2,
        0, 1),
      glm::imat2x2(
        1, 2,
        8, 5),
      glm::imat2x2(
        -100, 3,
        84, 6),
      glm::imat2x2(
        2, 12,
        -2, -2),
      glm::imat2x2(
        40, 61,
        7, -3),
  };
  // clang-format on

  addBufferToModel(model, values);

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::MAT2;
  testClassProperty.componentType = ClassProperty::ComponentType::INT32;
  testClassProperty.array = true;
  testClassProperty.count = 2;
  testClassProperty.normalized = true;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(
      values.size() / static_cast<size_t>(testClassProperty.count.value()));

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values =
      static_cast<int32_t>(model.bufferViews.size() - 1);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::MAT2);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT32);
  REQUIRE(classProperty->array);
  REQUIRE(classProperty->count == 2);
  REQUIRE(classProperty->normalized);

  SUBCASE("Access the right type") {
    PropertyTablePropertyView<PropertyArrayView<glm::imat2x2>, true>
        arrayProperty =
            view.getPropertyView<PropertyArrayView<glm::imat2x2>, true>(
                "TestClassProperty");
    REQUIRE(arrayProperty.status() == PropertyTablePropertyViewStatus::Valid);

    for (int64_t i = 0; i < arrayProperty.size(); ++i) {
      PropertyArrayView<glm::imat2x2> array = arrayProperty.getRaw(i);
      auto maybeArray = arrayProperty.get(i);
      REQUIRE(maybeArray);

      for (int64_t j = 0; j < array.size(); ++j) {
        REQUIRE(array[j] == values[static_cast<size_t>(i * 2 + j)]);
        REQUIRE((*maybeArray)[j] == normalize(array[j]));
      }
    }
  }

  SUBCASE("Wrong type") {
    PropertyTablePropertyView<PropertyArrayView<int32_t>, true>
        int32ArrayInvalid =
            view.getPropertyView<PropertyArrayView<int32_t>, true>(
                "TestClassProperty");
    REQUIRE(
        int32ArrayInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);

    PropertyTablePropertyView<PropertyArrayView<glm::ivec2>, true>
        ivec2ArrayInvalid =
            view.getPropertyView<PropertyArrayView<glm::ivec2>, true>(
                "TestClassProperty");
    REQUIRE(
        ivec2ArrayInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Wrong component type") {
    PropertyTablePropertyView<PropertyArrayView<glm::umat2x2>, true>
        umat2x2ArrayInvalid =
            view.getPropertyView<PropertyArrayView<glm::umat2x2>, true>(
                "TestClassProperty");
    REQUIRE(
        umat2x2ArrayInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Not an array type") {
    PropertyTablePropertyView<glm::imat2x2, true> ivec3Invalid =
        view.getPropertyView<glm::imat2x2, true>("TestClassProperty");
    REQUIRE(
        ivec3Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Incorrect non-normalization") {
    PropertyTablePropertyView<PropertyArrayView<glm::imat2x2>>
        nonNormalizedInvalid =
            view.getPropertyView<PropertyArrayView<glm::imat2x2>>(
                "TestClassProperty");
    REQUIRE(
        nonNormalizedInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorNormalizationMismatch);
  }
}

TEST_CASE("Test variable-length matN array") {
  Model model;
  // clang-format off
    std::vector<glm::imat2x2> data0{
        glm::imat2x2(
          3, -2,
          1, 0),
        glm::imat2x2(
          40, 3,
          8, -9)
    };
    std::vector<glm::imat2x2> data1{
        glm::imat2x2(
          1, 10,
          7, 8),
    };
    std::vector<glm::imat2x2> data2{
        glm::imat2x2(
          18, 0,
          1, 17),
        glm::imat2x2(
          -4, -2,
          -9, 1),
        glm::imat2x2(
          1, 8,
          -99, 3),
    };
  // clang-format on

  std::vector<std::vector<glm::imat2x2>> expected{data0, {}, data1, data2, {}};

  size_t numOfElements = 0;
  for (const auto& expectedMember : expected) {
    numOfElements += expectedMember.size();
  }

  std::vector<std::byte> values(numOfElements * sizeof(glm::imat2x2));
  std::vector<std::byte> offsets((expected.size() + 1) * sizeof(uint64_t));
  uint64_t* offsetValue = reinterpret_cast<uint64_t*>(offsets.data());
  for (size_t i = 0; i < expected.size(); ++i) {
    std::memcpy(
        values.data() + offsetValue[i] * sizeof(glm::imat2x2),
        expected[i].data(),
        expected[i].size() * sizeof(glm::imat2x2));
    offsetValue[i + 1] = offsetValue[i] + expected[i].size();
  }

  addBufferToModel(model, values);
  size_t valueBufferIndex = model.buffers.size() - 1;
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  addBufferToModel(model, offsets);
  size_t offsetBufferIndex = model.buffers.size() - 1;
  size_t offsetBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::MAT2;
  testClassProperty.componentType = ClassProperty::ComponentType::INT32;
  testClassProperty.array = true;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(expected.size());

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);
  propertyTableProperty.arrayOffsets =
      static_cast<int32_t>(offsetBufferViewIndex);
  propertyTableProperty.arrayOffsetType =
      PropertyTableProperty::ArrayOffsetType::UINT64;

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::MAT2);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT32);
  REQUIRE(classProperty->array);
  REQUIRE(!classProperty->count);
  REQUIRE(!classProperty->normalized);

  SUBCASE("Access the correct type") {
    PropertyTablePropertyView<PropertyArrayView<glm::imat2x2>> property =
        view.getPropertyView<PropertyArrayView<glm::imat2x2>>(
            "TestClassProperty");
    REQUIRE(property.status() == PropertyTablePropertyViewStatus::Valid);
    for (size_t i = 0; i < expected.size(); ++i) {
      PropertyArrayView<glm::imat2x2> array =
          property.getRaw(static_cast<int64_t>(i));
      REQUIRE(array.size() == static_cast<int64_t>(expected[i].size()));

      auto maybeArray = property.get(static_cast<int64_t>(i));
      REQUIRE(maybeArray);
      for (size_t j = 0; j < expected[i].size(); ++j) {
        auto value = array[static_cast<int64_t>(j)];
        REQUIRE(expected[i][j] == value);
        REQUIRE((*maybeArray)[static_cast<int64_t>(j)] == value);
      }
    }
  }

  SUBCASE("Incorrectly normalized") {
    PropertyTablePropertyView<PropertyArrayView<glm::imat2x2>, true> property =
        view.getPropertyView<PropertyArrayView<glm::imat2x2>, true>(
            "TestClassProperty");
    REQUIRE(
        property.status() ==
        PropertyTablePropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Wrong offset type") {
    propertyTableProperty.arrayOffsetType =
        PropertyTableProperty::ArrayOffsetType::UINT8;
    PropertyTablePropertyView<PropertyArrayView<glm::imat2x2>> arrayProperty =
        view.getPropertyView<PropertyArrayView<glm::imat2x2>>(
            "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);

    propertyTableProperty.arrayOffsetType =
        PropertyTableProperty::ArrayOffsetType::UINT16;
    arrayProperty = view.getPropertyView<PropertyArrayView<glm::imat2x2>>(
        "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);

    propertyTableProperty.arrayOffsetType = "NONSENSE";
    arrayProperty = view.getPropertyView<PropertyArrayView<glm::imat2x2>>(
        "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorInvalidArrayOffsetType);

    propertyTableProperty.arrayOffsetType = "";
    propertyTableProperty.stringOffsetType =
        PropertyTableProperty::StringOffsetType::UINT64;
    arrayProperty = view.getPropertyView<PropertyArrayView<glm::imat2x2>>(
        "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorInvalidArrayOffsetType);
  }

  SUBCASE("Offset values are not sorted ascending") {
    propertyTableProperty.arrayOffsetType =
        PropertyTableProperty::ArrayOffsetType::UINT64;
    uint64_t* offset = reinterpret_cast<uint64_t*>(
        model.buffers[offsetBufferIndex].cesium.data.data());
    offset[propertyTable.count] = 0;
    PropertyTablePropertyView<PropertyArrayView<glm::imat2x2>> arrayProperty =
        view.getPropertyView<PropertyArrayView<glm::imat2x2>>(
            "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorArrayOffsetsNotSorted);
  }

  SUBCASE("Offset value points outside of value buffer") {
    uint64_t* offset = reinterpret_cast<uint64_t*>(
        model.buffers[offsetBufferIndex].cesium.data.data());
    offset[propertyTable.count] =
        static_cast<uint32_t>(model.buffers[valueBufferIndex].byteLength + 4);
    PropertyTablePropertyView<PropertyArrayView<glm::imat2x2>> arrayProperty =
        view.getPropertyView<PropertyArrayView<glm::imat2x2>>(
            "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorArrayOffsetOutOfBounds);
  }

  SUBCASE("Count and offset buffer are both present") {
    testClassProperty.count = 3;
    PropertyTablePropertyView<PropertyArrayView<glm::imat2x2>> property =
        view.getPropertyView<PropertyArrayView<glm::imat2x2>>(
            "TestClassProperty");
    REQUIRE(
        property.status() ==
        PropertyTablePropertyViewStatus::ErrorArrayCountAndOffsetBufferCoexist);
  }
}

TEST_CASE("Test variable-length matN array (normalized)") {
  Model model;
  // clang-format off
    std::vector<glm::imat2x2> data0{
        glm::imat2x2(
          3, -2,
          1, 0),
        glm::imat2x2(
          40, 3,
          8, -9)
    };
    std::vector<glm::imat2x2> data1{
        glm::imat2x2(
          1, 10,
          7, 8),
    };
    std::vector<glm::imat2x2> data2{
        glm::imat2x2(
          18, 0,
          1, 17),
        glm::imat2x2(
          -4, -2,
          -9, 1),
        glm::imat2x2(
          1, 8,
          -99, 3),
    };
  // clang-format on

  std::vector<std::vector<glm::imat2x2>> expected{data0, {}, data1, data2, {}};

  size_t numOfElements = 0;
  for (const auto& expectedMember : expected) {
    numOfElements += expectedMember.size();
  }

  std::vector<std::byte> values(numOfElements * sizeof(glm::imat2x2));
  std::vector<std::byte> offsets((expected.size() + 1) * sizeof(uint64_t));
  uint64_t* offsetValue = reinterpret_cast<uint64_t*>(offsets.data());
  for (size_t i = 0; i < expected.size(); ++i) {
    std::memcpy(
        values.data() + offsetValue[i] * sizeof(glm::imat2x2),
        expected[i].data(),
        expected[i].size() * sizeof(glm::imat2x2));
    offsetValue[i + 1] = offsetValue[i] + expected[i].size();
  }

  addBufferToModel(model, values);
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  addBufferToModel(model, offsets);
  size_t offsetBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::MAT2;
  testClassProperty.componentType = ClassProperty::ComponentType::INT32;
  testClassProperty.array = true;
  testClassProperty.normalized = true;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(expected.size());

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);
  propertyTableProperty.arrayOffsets =
      static_cast<int32_t>(offsetBufferViewIndex);
  propertyTableProperty.arrayOffsetType =
      PropertyTableProperty::ArrayOffsetType::UINT64;

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::MAT2);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT32);
  REQUIRE(classProperty->array);
  REQUIRE(!classProperty->count);
  REQUIRE(classProperty->normalized);

  SUBCASE("Access the correct type") {
    PropertyTablePropertyView<PropertyArrayView<glm::imat2x2>, true> property =
        view.getPropertyView<PropertyArrayView<glm::imat2x2>, true>(
            "TestClassProperty");
    REQUIRE(property.status() == PropertyTablePropertyViewStatus::Valid);
    for (size_t i = 0; i < expected.size(); ++i) {
      PropertyArrayView<glm::imat2x2> array =
          property.getRaw(static_cast<int64_t>(i));
      REQUIRE(array.size() == static_cast<int64_t>(expected[i].size()));

      auto maybeArray = property.get(static_cast<int64_t>(i));
      REQUIRE(maybeArray);
      for (size_t j = 0; j < expected[i].size(); ++j) {
        auto value = array[static_cast<int64_t>(j)];
        REQUIRE(expected[i][j] == value);
        REQUIRE((*maybeArray)[static_cast<int64_t>(j)] == normalize(value));
      }
    }
  }

  SUBCASE("Incorrectly non-normalized") {
    PropertyTablePropertyView<PropertyArrayView<glm::imat2x2>> property =
        view.getPropertyView<PropertyArrayView<glm::imat2x2>>(
            "TestClassProperty");
    REQUIRE(
        property.status() ==
        PropertyTablePropertyViewStatus::ErrorNormalizationMismatch);
  }
}

TEST_CASE("Test fixed-length boolean array") {
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

  addBufferToModel(model, values);

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::BOOLEAN;
  testClassProperty.array = true;
  testClassProperty.count = 3;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(
      expected.size() / static_cast<size_t>(testClassProperty.count.value()));

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values =
      static_cast<int32_t>(model.bufferViews.size() - 1);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::BOOLEAN);
  REQUIRE(classProperty->array);
  REQUIRE(classProperty->count == 3);

  SUBCASE("Access correct type") {
    PropertyTablePropertyView<PropertyArrayView<bool>> boolArrayProperty =
        view.getPropertyView<PropertyArrayView<bool>>("TestClassProperty");
    REQUIRE(
        boolArrayProperty.status() == PropertyTablePropertyViewStatus::Valid);
    REQUIRE(boolArrayProperty.size() == propertyTable.count);
    REQUIRE(boolArrayProperty.size() > 0);
    for (int64_t i = 0; i < boolArrayProperty.size(); ++i) {
      PropertyArrayView<bool> array = boolArrayProperty.getRaw(i);
      auto maybeArray = boolArrayProperty.get(i);
      REQUIRE(maybeArray);

      for (int64_t j = 0; j < array.size(); ++j) {
        REQUIRE(array[j] == expected[static_cast<size_t>(i * 3 + j)]);
        REQUIRE((*maybeArray)[j] == array[j]);
      }
    }
  }

  SUBCASE("Wrong type") {
    PropertyTablePropertyView<PropertyArrayView<uint8_t>> uint8ArrayInvalid =
        view.getPropertyView<PropertyArrayView<uint8_t>>("TestClassProperty");
    REQUIRE(
        uint8ArrayInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("View is not array type") {
    PropertyTablePropertyView<bool> boolInvalid =
        view.getPropertyView<bool>("TestClassProperty");
    REQUIRE(
        boolInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Value buffer doesn't have enough required bytes") {
    testClassProperty.count = 11;
    PropertyTablePropertyView<PropertyArrayView<bool>> boolArrayProperty =
        view.getPropertyView<PropertyArrayView<bool>>("TestClassProperty");
    REQUIRE(
        boolArrayProperty.status() ==
        PropertyTablePropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);
  }

  SUBCASE("Count is negative") {
    testClassProperty.count = -1;
    PropertyTablePropertyView<PropertyArrayView<bool>> boolArrayProperty =
        view.getPropertyView<PropertyArrayView<bool>>("TestClassProperty");
    REQUIRE(
        boolArrayProperty.status() ==
        PropertyTablePropertyViewStatus::
            ErrorArrayCountAndOffsetBufferDontExist);
  }
}

TEST_CASE("Test variable-length boolean array") {
  Model model;

  std::vector<std::vector<bool>> expected{
      {true, true, true, true, true},
      {},
      {},
      {},
      {false},
      {true, true},
      {false},
      {true, true, true, true, true}};
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

  addBufferToModel(model, values);
  size_t valueBufferIndex = model.buffers.size() - 1;
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  addBufferToModel(model, offsets);
  size_t offsetBufferIndex = model.buffers.size() - 1;
  size_t offsetBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::BOOLEAN;
  testClassProperty.array = true;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(expected.size());

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);
  propertyTableProperty.arrayOffsets =
      static_cast<int32_t>(offsetBufferViewIndex);
  propertyTableProperty.arrayOffsetType =
      PropertyTableProperty::ArrayOffsetType::UINT64;

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::BOOLEAN);
  REQUIRE(classProperty->array);
  REQUIRE(!classProperty->count);

  SUBCASE("Access correct type") {
    PropertyTablePropertyView<PropertyArrayView<bool>> boolArrayProperty =
        view.getPropertyView<PropertyArrayView<bool>>("TestClassProperty");
    REQUIRE(
        boolArrayProperty.status() == PropertyTablePropertyViewStatus::Valid);
    for (size_t i = 0; i < expected.size(); ++i) {
      PropertyArrayView<bool> array =
          boolArrayProperty.getRaw(static_cast<int64_t>(i));
      REQUIRE(array.size() == static_cast<int64_t>(expected[i].size()));

      auto maybeArray = boolArrayProperty.get(static_cast<int64_t>(i));
      REQUIRE(maybeArray);
      for (size_t j = 0; j < expected[i].size(); ++j) {
        auto value = array[static_cast<int64_t>(j)];
        REQUIRE(expected[i][j] == value);
        REQUIRE((*maybeArray)[static_cast<int64_t>(j)] == value);
      }
    }
  }

  SUBCASE("Wrong offset type") {
    propertyTableProperty.arrayOffsetType =
        PropertyTableProperty::ArrayOffsetType::UINT8;
    PropertyTablePropertyView<PropertyArrayView<bool>> arrayProperty =
        view.getPropertyView<PropertyArrayView<bool>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);

    propertyTableProperty.arrayOffsetType =
        PropertyTableProperty::ArrayOffsetType::UINT16;
    arrayProperty =
        view.getPropertyView<PropertyArrayView<bool>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);

    propertyTableProperty.arrayOffsetType = "NONSENSE";
    arrayProperty =
        view.getPropertyView<PropertyArrayView<bool>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorInvalidArrayOffsetType);

    propertyTableProperty.arrayOffsetType = "";
    propertyTableProperty.stringOffsetType =
        PropertyTableProperty::StringOffsetType::UINT64;
    arrayProperty =
        view.getPropertyView<PropertyArrayView<bool>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorInvalidArrayOffsetType);
  }

  SUBCASE("Offset values are not sorted ascending") {
    propertyTableProperty.arrayOffsetType =
        PropertyTableProperty::ArrayOffsetType::UINT64;
    uint64_t* offset = reinterpret_cast<uint64_t*>(
        model.buffers[offsetBufferIndex].cesium.data.data());
    offset[propertyTable.count] = 0;
    PropertyTablePropertyView<PropertyArrayView<bool>> arrayProperty =
        view.getPropertyView<PropertyArrayView<bool>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorArrayOffsetsNotSorted);
  }

  SUBCASE("Offset value points outside of value buffer") {
    uint64_t* offset = reinterpret_cast<uint64_t*>(
        model.buffers[offsetBufferIndex].cesium.data.data());
    offset[propertyTable.count] = static_cast<uint32_t>(
        model.buffers[valueBufferIndex].byteLength * 8 + 20);
    PropertyTablePropertyView<PropertyArrayView<bool>> arrayProperty =
        view.getPropertyView<PropertyArrayView<bool>>("TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorArrayOffsetOutOfBounds);
  }

  SUBCASE("Count and offset buffer both present") {
    testClassProperty.count = 3;
    PropertyTablePropertyView<PropertyArrayView<bool>> boolArrayProperty =
        view.getPropertyView<PropertyArrayView<bool>>("TestClassProperty");
    REQUIRE(
        boolArrayProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorArrayCountAndOffsetBufferCoexist);
  }
}

TEST_CASE("Test fixed-length arrays of strings") {
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

  addBufferToModel(model, values);
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  addBufferToModel(model, offsets);
  size_t offsetBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::STRING;
  testClassProperty.array = true;
  testClassProperty.count = 2;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(
      expected.size() / static_cast<size_t>(testClassProperty.count.value()));

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.stringOffsetType =
      PropertyTableProperty::StringOffsetType::UINT32;
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);
  propertyTableProperty.stringOffsets =
      static_cast<int32_t>(offsetBufferViewIndex);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::STRING);
  REQUIRE(classProperty->array);
  REQUIRE(classProperty->count == 2);

  SUBCASE("Access correct type") {
    PropertyTablePropertyView<PropertyArrayView<std::string_view>>
        stringProperty =
            view.getPropertyView<PropertyArrayView<std::string_view>>(
                "TestClassProperty");
    REQUIRE(stringProperty.status() == PropertyTablePropertyViewStatus::Valid);
    REQUIRE(stringProperty.size() == 3);

    PropertyArrayView<std::string_view> v0 = stringProperty.getRaw(0);
    REQUIRE(v0.size() == 2);
    REQUIRE(v0[0] == "What's up");
    REQUIRE(v0[1] == "Breaking news!!! Aliens no longer attacks the US first");

    PropertyArrayView<std::string_view> v1 = stringProperty.getRaw(1);
    REQUIRE(v1.size() == 2);
    REQUIRE(v1[0] == "But they still abduct my cows! Those milk thiefs! 👽 🐮");
    REQUIRE(v1[1] == "I'm not crazy. My mother had me tested 🤪");

    PropertyArrayView<std::string_view> v2 = stringProperty.getRaw(2);
    REQUIRE(v2.size() == 2);
    REQUIRE(v2[0] == "I love you, meat bags! ❤️");
    REQUIRE(v2[1] == "Book in the freezer");

    for (int64_t i = 0; i < stringProperty.size(); i++) {
      auto maybeValue = stringProperty.get(i);
      REQUIRE(maybeValue);

      auto value = stringProperty.getRaw(i);
      REQUIRE(maybeValue->size() == value.size());
      for (int64_t j = 0; j < value.size(); j++) {
        REQUIRE((*maybeValue)[j] == value[j]);
      }
    }
  }

  SUBCASE("Array type mismatch") {
    PropertyTablePropertyView<std::string_view> stringProperty =
        view.getPropertyView<std::string_view>("TestClassProperty");
    REQUIRE(
        stringProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Count is negative") {
    testClassProperty.count = -1;
    PropertyTablePropertyView<PropertyArrayView<std::string_view>>
        stringProperty =
            view.getPropertyView<PropertyArrayView<std::string_view>>(
                "TestClassProperty");
    REQUIRE(
        stringProperty.status() == PropertyTablePropertyViewStatus::
                                       ErrorArrayCountAndOffsetBufferDontExist);
  }

  SUBCASE("Offset type is unknown") {
    propertyTableProperty.stringOffsetType = "NONSENSE";
    PropertyTablePropertyView<PropertyArrayView<std::string_view>>
        stringProperty =
            view.getPropertyView<PropertyArrayView<std::string_view>>(
                "TestClassProperty");
    REQUIRE(
        stringProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorInvalidStringOffsetType);

    propertyTableProperty.stringOffsetType = "";
    propertyTableProperty.arrayOffsetType =
        PropertyTableProperty::ArrayOffsetType::UINT32;
    stringProperty = view.getPropertyView<PropertyArrayView<std::string_view>>(
        "TestClassProperty");
    REQUIRE(
        stringProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorInvalidStringOffsetType);
  }

  SUBCASE("String offsets don't exist") {
    propertyTableProperty.stringOffsets = -1;
    PropertyTablePropertyView<PropertyArrayView<std::string_view>>
        stringProperty =
            view.getPropertyView<PropertyArrayView<std::string_view>>(
                "TestClassProperty");
    REQUIRE(
        stringProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorInvalidStringOffsetBufferView);
  }
}

TEST_CASE("Test variable-length arrays of strings") {
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

  addBufferToModel(model, values);
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  addBufferToModel(model, offsets);
  size_t arrayOffsetBuffer = model.buffers.size() - 1;
  size_t arrayOffsetBufferView = model.bufferViews.size() - 1;

  addBufferToModel(model, stringOffsets);
  size_t stringOffsetBuffer = model.buffers.size() - 1;
  size_t stringOffsetBufferView = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::STRING;
  testClassProperty.array = true;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(expected.size());

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.arrayOffsetType =
      PropertyTableProperty::ArrayOffsetType::UINT32;
  propertyTableProperty.stringOffsetType =
      PropertyTableProperty::StringOffsetType::UINT32;
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);
  propertyTableProperty.arrayOffsets =
      static_cast<int32_t>(arrayOffsetBufferView);
  propertyTableProperty.stringOffsets =
      static_cast<int32_t>(stringOffsetBufferView);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::STRING);
  REQUIRE(classProperty->array);
  REQUIRE(!classProperty->componentType);
  REQUIRE(!classProperty->count);

  SUBCASE("Access correct type") {
    PropertyTablePropertyView<PropertyArrayView<std::string_view>>
        stringProperty =
            view.getPropertyView<PropertyArrayView<std::string_view>>(
                "TestClassProperty");
    REQUIRE(stringProperty.status() == PropertyTablePropertyViewStatus::Valid);

    for (size_t i = 0; i < expected.size(); ++i) {
      PropertyArrayView<std::string_view> stringArray =
          stringProperty.getRaw(static_cast<int64_t>(i));
      auto maybeArray = stringProperty.get(static_cast<int64_t>(i));
      REQUIRE(maybeArray);
      for (size_t j = 0; j < expected[i].size(); ++j) {
        REQUIRE(stringArray[static_cast<int64_t>(j)] == expected[i][j]);
        REQUIRE((*maybeArray)[static_cast<int64_t>(j)] == expected[i][j]);
      }
    }
  }

  SUBCASE("Wrong array offset type") {
    propertyTableProperty.arrayOffsetType =
        PropertyTableProperty::ArrayOffsetType::UINT8;
    PropertyTablePropertyView<PropertyArrayView<std::string_view>>
        arrayProperty =
            view.getPropertyView<PropertyArrayView<std::string_view>>(
                "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);

    propertyTableProperty.arrayOffsetType =
        PropertyTableProperty::ArrayOffsetType::UINT16;
    arrayProperty = view.getPropertyView<PropertyArrayView<std::string_view>>(
        "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);

    propertyTableProperty.arrayOffsetType = "NONSENSE";
    arrayProperty = view.getPropertyView<PropertyArrayView<std::string_view>>(
        "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorInvalidArrayOffsetType);
    propertyTableProperty.arrayOffsetType =
        PropertyTableProperty::ArrayOffsetType::UINT32;
  }

  SUBCASE("Wrong string offset type") {
    propertyTableProperty.stringOffsetType =
        PropertyTableProperty::StringOffsetType::UINT8;
    PropertyTablePropertyView<PropertyArrayView<std::string_view>>
        arrayProperty =
            view.getPropertyView<PropertyArrayView<std::string_view>>(
                "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);

    propertyTableProperty.stringOffsetType =
        PropertyTableProperty::StringOffsetType::UINT16;
    arrayProperty = view.getPropertyView<PropertyArrayView<std::string_view>>(
        "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::
            ErrorBufferViewSizeDoesNotMatchPropertyTableCount);

    propertyTableProperty.stringOffsetType = "NONSENSE";
    arrayProperty = view.getPropertyView<PropertyArrayView<std::string_view>>(
        "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorInvalidStringOffsetType);
    propertyTableProperty.stringOffsetType =
        PropertyTableProperty::StringOffsetType::UINT32;
  }

  SUBCASE("Array offset values are not sorted ascending") {
    uint32_t* offset = reinterpret_cast<uint32_t*>(
        model.buffers[arrayOffsetBuffer].cesium.data.data());
    offset[0] = static_cast<uint32_t>(1000);
    PropertyTablePropertyView<PropertyArrayView<std::string_view>>
        arrayProperty =
            view.getPropertyView<PropertyArrayView<std::string_view>>(
                "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorArrayOffsetsNotSorted);
    offset[0] = 0;
  }

  SUBCASE("String offset values are not sorted ascending") {
    uint32_t* offset = reinterpret_cast<uint32_t*>(
        model.buffers[stringOffsetBuffer].cesium.data.data());
    offset[0] = static_cast<uint32_t>(1000);
    PropertyTablePropertyView<PropertyArrayView<std::string_view>>
        arrayProperty =
            view.getPropertyView<PropertyArrayView<std::string_view>>(
                "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorStringOffsetsNotSorted);
    offset[0] = 0;
  }

  SUBCASE("Array offset value points outside of value buffer") {
    uint32_t* offset = reinterpret_cast<uint32_t*>(
        model.buffers[arrayOffsetBuffer].cesium.data.data());
    uint32_t previousValue = offset[propertyTable.count];
    offset[propertyTable.count] = static_cast<uint32_t>(100000);
    PropertyTablePropertyView<PropertyArrayView<std::string_view>>
        arrayProperty =
            view.getPropertyView<PropertyArrayView<std::string_view>>(
                "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorArrayOffsetOutOfBounds);
    offset[propertyTable.count] = previousValue;
  }

  SUBCASE("String offset value points outside of value buffer") {
    uint32_t* offset = reinterpret_cast<uint32_t*>(
        model.buffers[stringOffsetBuffer].cesium.data.data());
    uint32_t previousValue = offset[6];
    offset[6] = static_cast<uint32_t>(100000);
    PropertyTablePropertyView<PropertyArrayView<std::string_view>>
        arrayProperty =
            view.getPropertyView<PropertyArrayView<std::string_view>>(
                "TestClassProperty");
    REQUIRE(
        arrayProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorStringOffsetOutOfBounds);
    offset[6] = previousValue;
  }

  SUBCASE("Count and offset buffer both present") {
    testClassProperty.count = 3;
    PropertyTablePropertyView<PropertyArrayView<std::string_view>>
        boolArrayProperty =
            view.getPropertyView<PropertyArrayView<std::string_view>>(
                "TestClassProperty");
    REQUIRE(
        boolArrayProperty.status() ==
        PropertyTablePropertyViewStatus::ErrorArrayCountAndOffsetBufferCoexist);
  }
}

TEST_CASE("Test with PropertyTableProperty offset, scale, min, max") {
  Model model;
  std::vector<float> values = {1.0f, 2.0f, 3.0f, 4.0f};
  const float offset = 0.5f;
  const float scale = 2.0f;
  const float min = 3.5f;
  const float max = 8.5f;

  addBufferToModel(model, values);
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::FLOAT32;
  testClassProperty.offset = offset;
  testClassProperty.scale = scale;
  testClassProperty.min = min;
  testClassProperty.max = max;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(values.size());

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(
      classProperty->componentType == ClassProperty::ComponentType::FLOAT32);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(!classProperty->normalized);
  REQUIRE(classProperty->offset);
  REQUIRE(classProperty->scale);
  REQUIRE(classProperty->min);
  REQUIRE(classProperty->max);

  SUBCASE("Use class property values") {
    PropertyTablePropertyView<float> propertyView =
        view.getPropertyView<float>("TestClassProperty");
    REQUIRE(propertyView.status() == PropertyTablePropertyViewStatus::Valid);
    REQUIRE(propertyView.size() > 0);
    REQUIRE(propertyView.offset() == offset);
    REQUIRE(propertyView.scale() == scale);
    REQUIRE(propertyView.min() == min);
    REQUIRE(propertyView.max() == max);

    for (int64_t i = 0; i < propertyView.size(); ++i) {
      REQUIRE(propertyView.getRaw(i) == values[static_cast<size_t>(i)]);
      REQUIRE(propertyView.get(i) == propertyView.getRaw(i) * scale + offset);
    }
  }

  SUBCASE("Use own property values") {
    const float newOffset = 1.0f;
    const float newScale = -1.0f;
    const float newMin = -3.0f;
    const float newMax = 0.0f;
    propertyTableProperty.offset = newOffset;
    propertyTableProperty.scale = newScale;
    propertyTableProperty.min = newMin;
    propertyTableProperty.max = newMax;

    PropertyTablePropertyView<float> propertyView =
        view.getPropertyView<float>("TestClassProperty");
    REQUIRE(propertyView.status() == PropertyTablePropertyViewStatus::Valid);
    REQUIRE(propertyView.size() > 0);
    REQUIRE(propertyView.offset() == newOffset);
    REQUIRE(propertyView.scale() == newScale);
    REQUIRE(propertyView.min() == newMin);
    REQUIRE(propertyView.max() == newMax);

    for (int64_t i = 0; i < propertyView.size(); ++i) {
      REQUIRE(propertyView.getRaw(i) == values[static_cast<size_t>(i)]);
      REQUIRE(
          propertyView.get(i) == propertyView.getRaw(i) * newScale + newOffset);
    }
  }
}

TEST_CASE(
    "Test with PropertyTableProperty offset, scale, min, max (normalized)") {
  Model model;
  std::vector<int8_t> values = {-128, 0, 32, 127};
  const double offset = 0.5;
  const double scale = 2.0;
  const double min = 1.5;
  const double max = 2.5;

  addBufferToModel(model, values);
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::INT8;
  testClassProperty.normalized = true;
  testClassProperty.offset = offset;
  testClassProperty.scale = scale;
  testClassProperty.min = min;
  testClassProperty.max = max;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(values.size());

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT8);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(classProperty->normalized);
  REQUIRE(classProperty->offset);
  REQUIRE(classProperty->scale);
  REQUIRE(classProperty->min);
  REQUIRE(classProperty->max);

  SUBCASE("Use class property values") {
    PropertyTablePropertyView<int8_t, true> propertyView =
        view.getPropertyView<int8_t, true>("TestClassProperty");
    REQUIRE(propertyView.status() == PropertyTablePropertyViewStatus::Valid);
    REQUIRE(propertyView.size() > 0);
    REQUIRE(propertyView.offset() == offset);
    REQUIRE(propertyView.scale() == scale);
    REQUIRE(propertyView.min() == min);
    REQUIRE(propertyView.max() == max);

    for (int64_t i = 0; i < propertyView.size(); ++i) {
      REQUIRE(propertyView.getRaw(i) == values[static_cast<size_t>(i)]);
      REQUIRE(
          propertyView.get(i) ==
          normalize(propertyView.getRaw(i)) * scale + offset);
    }
  }

  SUBCASE("Use own property values") {
    const double newOffset = -0.5;
    const double newScale = 1.0;
    const double newMin = -1.5;
    const double newMax = 0.5;
    propertyTableProperty.offset = newOffset;
    propertyTableProperty.scale = newScale;
    propertyTableProperty.min = newMin;
    propertyTableProperty.max = newMax;

    PropertyTablePropertyView<int8_t, true> propertyView =
        view.getPropertyView<int8_t, true>("TestClassProperty");
    REQUIRE(propertyView.status() == PropertyTablePropertyViewStatus::Valid);
    REQUIRE(propertyView.size() > 0);
    REQUIRE(propertyView.offset() == newOffset);
    REQUIRE(propertyView.scale() == newScale);
    REQUIRE(propertyView.min() == newMin);
    REQUIRE(propertyView.max() == newMax);

    for (int64_t i = 0; i < propertyView.size(); ++i) {
      REQUIRE(propertyView.getRaw(i) == values[static_cast<size_t>(i)]);
      REQUIRE(
          propertyView.get(i) ==
          normalize(propertyView.getRaw(i)) * newScale + newOffset);
    }
  }
}

TEST_CASE("Test with PropertyTableProperty noData value") {
  Model model;
  std::vector<int8_t> values = {-128, 0, 32, -128, 127};
  const int8_t noData = -128;

  addBufferToModel(model, values);
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::INT8;
  testClassProperty.noData = noData;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(values.size());

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT8);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(!classProperty->normalized);
  REQUIRE(classProperty->noData);

  SUBCASE("Without default value") {
    PropertyTablePropertyView<int8_t> propertyView =
        view.getPropertyView<int8_t>("TestClassProperty");
    REQUIRE(propertyView.status() == PropertyTablePropertyViewStatus::Valid);
    REQUIRE(propertyView.size() > 0);
    REQUIRE(propertyView.noData() == noData);

    for (int64_t i = 0; i < propertyView.size(); ++i) {
      REQUIRE(propertyView.getRaw(i) == values[static_cast<size_t>(i)]);
      if (propertyView.getRaw(i) == noData) {
        REQUIRE(!propertyView.get(i));
      } else {
        REQUIRE(propertyView.get(i) == propertyView.getRaw(i));
      }
    }
  }

  SUBCASE("With default value") {
    const int8_t defaultValue = 100;
    testClassProperty.defaultProperty = defaultValue;
    classProperty = view.getClassProperty("TestClassProperty");
    REQUIRE(classProperty->defaultProperty);
    PropertyTablePropertyView<int8_t> propertyView =
        view.getPropertyView<int8_t>("TestClassProperty");
    REQUIRE(propertyView.status() == PropertyTablePropertyViewStatus::Valid);
    REQUIRE(propertyView.size() > 0);
    REQUIRE(propertyView.noData() == noData);
    REQUIRE(propertyView.defaultValue() == defaultValue);

    for (int64_t i = 0; i < propertyView.size(); ++i) {
      REQUIRE(propertyView.getRaw(i) == values[static_cast<size_t>(i)]);
      if (propertyView.getRaw(i) == noData) {
        REQUIRE(propertyView.get(i) == defaultValue);
      } else {
        REQUIRE(propertyView.get(i) == propertyView.getRaw(i));
      }
    }
  }
}

TEST_CASE("Test with PropertyTableProperty noData value (normalized)") {
  Model model;
  std::vector<int8_t> values = {-128, 0, 32, -128, 127};
  const int8_t noData = -128;

  addBufferToModel(model, values);
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::INT8;
  testClassProperty.normalized = true;
  testClassProperty.noData = noData;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(values.size());

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT8);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(classProperty->normalized);
  REQUIRE(classProperty->noData);

  SUBCASE("Without default value") {
    PropertyTablePropertyView<int8_t, true> propertyView =
        view.getPropertyView<int8_t, true>("TestClassProperty");
    REQUIRE(propertyView.status() == PropertyTablePropertyViewStatus::Valid);
    REQUIRE(propertyView.size() > 0);
    REQUIRE(propertyView.noData() == noData);

    for (int64_t i = 0; i < propertyView.size(); ++i) {
      REQUIRE(propertyView.getRaw(i) == values[static_cast<size_t>(i)]);
      if (propertyView.getRaw(i) == noData) {
        REQUIRE(!propertyView.get(i));
      } else {
        REQUIRE(propertyView.get(i) == normalize(propertyView.getRaw(i)));
      }
    }
  }

  SUBCASE("With default value") {
    const double defaultValue = 10.5;
    testClassProperty.defaultProperty = defaultValue;
    classProperty = view.getClassProperty("TestClassProperty");
    REQUIRE(classProperty->defaultProperty);

    PropertyTablePropertyView<int8_t, true> propertyView =
        view.getPropertyView<int8_t, true>("TestClassProperty");
    REQUIRE(propertyView.status() == PropertyTablePropertyViewStatus::Valid);
    REQUIRE(propertyView.size() > 0);
    REQUIRE(propertyView.noData() == noData);
    REQUIRE(propertyView.defaultValue() == defaultValue);

    for (int64_t i = 0; i < propertyView.size(); ++i) {
      REQUIRE(propertyView.getRaw(i) == values[static_cast<size_t>(i)]);
      if (propertyView.getRaw(i) == noData) {
        REQUIRE(propertyView.get(i) == defaultValue);
      } else {
        REQUIRE(propertyView.get(i) == normalize(propertyView.getRaw(i)));
      }
    }
  }
}

TEST_CASE(
    "Test nonexistent PropertyTableProperty with class property default") {
  Model model;
  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT32;

  const uint32_t defaultValue = 10;
  testClassProperty.defaultProperty = defaultValue;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = 4;

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT32);
  REQUIRE(!classProperty->array);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->normalized);
  REQUIRE(classProperty->defaultProperty);

  SUBCASE("Access correct type") {
    PropertyTablePropertyView<uint32_t> uint32Property =
        view.getPropertyView<uint32_t>("TestClassProperty");
    REQUIRE(
        uint32Property.status() ==
        PropertyTablePropertyViewStatus::EmptyPropertyWithDefault);
    REQUIRE(uint32Property.size() == propertyTable.count);
    REQUIRE(uint32Property.defaultValue() == defaultValue);

    for (int64_t i = 0; i < uint32Property.size(); ++i) {
      REQUIRE(uint32Property.get(i) == defaultValue);
    }
  }

  SUBCASE("Access wrong type") {
    PropertyTablePropertyView<glm::uvec3> uvec3Invalid =
        view.getPropertyView<glm::uvec3>("TestClassProperty");
    REQUIRE(
        uvec3Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Access wrong component type") {
    PropertyTablePropertyView<uint8_t> uint8Invalid =
        view.getPropertyView<uint8_t>("TestClassProperty");
    REQUIRE(
        uint8Invalid.status() ==
        PropertyTablePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Access incorrectly as normalized") {
    PropertyTablePropertyView<uint32_t, true> uint32NormalizedInvalid =
        view.getPropertyView<uint32_t, true>("TestClassProperty");
    REQUIRE(
        uint32NormalizedInvalid.status() ==
        PropertyTablePropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Invalid default value") {
    testClassProperty.defaultProperty = "not a number";
    PropertyTablePropertyView<uint32_t> uint32Property =
        view.getPropertyView<uint32_t>("TestClassProperty");
    REQUIRE(
        uint32Property.status() ==
        PropertyTablePropertyViewStatus::ErrorInvalidDefaultValue);
  }

  SUBCASE("No default value") {
    testClassProperty.defaultProperty.reset();
    PropertyTablePropertyView<uint32_t> uint32Property =
        view.getPropertyView<uint32_t>("TestClassProperty");
    REQUIRE(
        uint32Property.status() ==
        PropertyTablePropertyViewStatus::ErrorNonexistentProperty);
  }
}

TEST_CASE("Test callback on invalid property table view") {
  Model model;
  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();
  metadata.schema.emplace();

  // Property table has a nonexistent class.
  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(5);

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(-1);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::ErrorClassNotFound);
  REQUIRE(view.size() == 0);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(!classProperty);

  uint32_t invokedCallbackCount = 0;
  auto callback = [&invokedCallbackCount](
                      const std::string& /*propertyId*/,
                      auto property) mutable {
    invokedCallbackCount++;
    REQUIRE(
        property.status() ==
        PropertyTablePropertyViewStatus::ErrorInvalidPropertyTable);
    REQUIRE(property.size() == 0);
  };

  view.getPropertyView("TestClassProperty", callback);

  REQUIRE(invokedCallbackCount == 1);
}

TEST_CASE("Test callback for invalid PropertyTableProperty") {
  Model model;
  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["InvalidProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT32;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(5);

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["InvalidProperty"];
  propertyTableProperty.values = static_cast<int32_t>(-1);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty = view.getClassProperty("InvalidProperty");
  REQUIRE(classProperty);

  classProperty = view.getClassProperty("NonexistentProperty");
  REQUIRE(!classProperty);

  uint32_t invokedCallbackCount = 0;
  auto testCallback = [&invokedCallbackCount](
                          const std::string& /*propertyId*/,
                          auto propertyValue) mutable {
    invokedCallbackCount++;
    REQUIRE(propertyValue.status() != PropertyTablePropertyViewStatus::Valid);
    REQUIRE(propertyValue.size() == 0);
  };

  view.getPropertyView("InvalidProperty", testCallback);
  view.getPropertyView("NonexistentProperty", testCallback);

  REQUIRE(invokedCallbackCount == 2);
}

TEST_CASE("Test callback for invalid normalized PropertyTableProperty") {
  Model model;
  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::FLOAT32;
  testClassProperty.normalized = true; // This is erroneous.

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(5);

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = 0;

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(
      classProperty->componentType == ClassProperty::ComponentType::FLOAT32);
  REQUIRE(!classProperty->array);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(classProperty->normalized);

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      "TestClassProperty",
      [&invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) mutable {
        invokedCallbackCount++;
        REQUIRE(
            propertyValue.status() ==
            PropertyTablePropertyViewStatus::ErrorInvalidNormalization);
        REQUIRE(propertyValue.size() == 0);
      });

  REQUIRE(invokedCallbackCount == 1);
}

TEST_CASE("Test callback for scalar PropertyTableProperty") {
  Model model;
  std::vector<uint32_t> values = {12, 34, 30, 11, 34, 34, 11, 33, 122, 33};

  addBufferToModel(model, values);
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT32;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(values.size());

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT32);
  REQUIRE(!classProperty->array);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->normalized);

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      "TestClassProperty",
      [&values, &invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) mutable {
        invokedCallbackCount++;
        if constexpr (std::is_same_v<
                          PropertyTablePropertyView<uint32_t>,
                          decltype(propertyValue)>) {
          REQUIRE(
              propertyValue.status() == PropertyTablePropertyViewStatus::Valid);
          REQUIRE(propertyValue.size() > 0);

          for (int64_t i = 0; i < propertyValue.size(); ++i) {
            auto expectedValue = values[static_cast<size_t>(i)];
            REQUIRE(
                static_cast<uint32_t>(propertyValue.getRaw(i)) ==
                expectedValue);
            REQUIRE(propertyValue.get(i) == expectedValue);
          }
        } else {
          FAIL("getPropertyView returned PropertyTablePropertyView of "
               "incorrect type for TestClassProperty.");
        }
      });

  REQUIRE(invokedCallbackCount == 1);
}

TEST_CASE("Test callback for scalar PropertyTableProperty (normalized)") {
  Model model;
  std::vector<uint32_t> values = {12, 34, 30, 11, 34, 34, 11, 33, 122, 33};

  addBufferToModel(model, values);
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT32;
  testClassProperty.normalized = true;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(values.size());

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT32);
  REQUIRE(!classProperty->array);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(classProperty->normalized);

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      "TestClassProperty",
      [&values, &invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) mutable {
        invokedCallbackCount++;
        if constexpr (std::is_same_v<
                          PropertyTablePropertyView<uint32_t, true>,
                          decltype(propertyValue)>) {
          REQUIRE(
              propertyValue.status() == PropertyTablePropertyViewStatus::Valid);
          REQUIRE(propertyValue.size() > 0);

          for (int64_t i = 0; i < propertyValue.size(); ++i) {
            auto expectedValue = values[static_cast<size_t>(i)];
            REQUIRE(
                static_cast<uint32_t>(propertyValue.getRaw(i)) ==
                expectedValue);
            REQUIRE(propertyValue.get(i) == normalize(expectedValue));
          }
        } else {
          FAIL("getPropertyView returned PropertyTablePropertyView of "
               "incorrect type for TestClassProperty.");
        }
      });

  REQUIRE(invokedCallbackCount == 1);
}

TEST_CASE("Test callback for vecN PropertyTableProperty") {
  Model model;
  std::vector<glm::ivec3> values = {
      glm::ivec3(-12, 34, 30),
      glm::ivec3(11, 73, 0),
      glm::ivec3(-2, 6, 12),
      glm::ivec3(-4, 8, -13)};

  addBufferToModel(model, values);
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::VEC3;
  testClassProperty.componentType = ClassProperty::ComponentType::INT32;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(values.size());

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::VEC3);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT32);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(!classProperty->normalized);

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      "TestClassProperty",
      [&values, &invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) mutable {
        invokedCallbackCount++;
        REQUIRE(
            propertyValue.status() == PropertyTablePropertyViewStatus::Valid);
        REQUIRE(propertyValue.size() > 0);

        if constexpr (std::is_same_v<
                          PropertyTablePropertyView<glm::ivec3>,
                          decltype(propertyValue)>) {
          for (int64_t i = 0; i < propertyValue.size(); ++i) {
            auto expectedValue = values[static_cast<size_t>(i)];
            REQUIRE(
                static_cast<glm::ivec3>(propertyValue.getRaw(i)) ==
                expectedValue);
            REQUIRE(propertyValue.get(i) == expectedValue);
          }
        } else {
          FAIL("getPropertyView returned PropertyTablePropertyView of "
               "incorrect type for TestClassProperty.");
        }
      });

  REQUIRE(invokedCallbackCount == 1);
}

TEST_CASE("Test callback for vecN PropertyTableProperty (normalized)") {
  Model model;
  std::vector<glm::ivec3> values = {
      glm::ivec3(-12, 34, 30),
      glm::ivec3(11, 73, 0),
      glm::ivec3(-2, 6, 12),
      glm::ivec3(-4, 8, -13)};

  addBufferToModel(model, values);
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::VEC3;
  testClassProperty.componentType = ClassProperty::ComponentType::INT32;
  testClassProperty.normalized = true;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(values.size());

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::VEC3);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT32);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(classProperty->normalized);

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      "TestClassProperty",
      [&values, &invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) mutable {
        invokedCallbackCount++;
        REQUIRE(
            propertyValue.status() == PropertyTablePropertyViewStatus::Valid);
        REQUIRE(propertyValue.size() > 0);

        if constexpr (std::is_same_v<
                          PropertyTablePropertyView<glm::ivec3, true>,
                          decltype(propertyValue)>) {
          for (int64_t i = 0; i < propertyValue.size(); ++i) {
            auto expectedValue = values[static_cast<size_t>(i)];
            REQUIRE(propertyValue.getRaw(i) == expectedValue);
            REQUIRE(propertyValue.get(i) == normalize(expectedValue));
          }
        } else {
          FAIL("getPropertyView returned PropertyTablePropertyView of "
               "incorrect type for TestClassProperty.");
        }
      });

  REQUIRE(invokedCallbackCount == 1);
}

TEST_CASE("Test callback for matN PropertyTableProperty") {
  Model model;
  // clang-format off
  std::vector<glm::umat2x2> values = {
      glm::umat2x2(
        12, 34,
        30, 1),
      glm::umat2x2(
        11, 8,
        73, 102),
      glm::umat2x2(
        1, 0,
        63, 2),
      glm::umat2x2(
        4, 8,
        3, 23)};
  // clang-format on

  addBufferToModel(model, values);
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::MAT2;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT32;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(values.size());

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::MAT2);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT32);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(!classProperty->normalized);

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      "TestClassProperty",
      [&values, &invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) mutable {
        REQUIRE(
            propertyValue.status() == PropertyTablePropertyViewStatus::Valid);
        REQUIRE(propertyValue.size() > 0);

        if constexpr (std::is_same_v<
                          PropertyTablePropertyView<glm::umat2x2>,
                          decltype(propertyValue)>) {
          for (int64_t i = 0; i < propertyValue.size(); ++i) {
            auto expectedValue = values[static_cast<size_t>(i)];
            REQUIRE(propertyValue.getRaw(i) == expectedValue);
            REQUIRE(propertyValue.get(i) == expectedValue);
          }
        } else {
          FAIL("getPropertyView returned PropertyTablePropertyView of "
               "incorrect type for TestClassProperty.");
        }
        invokedCallbackCount++;
      });

  REQUIRE(invokedCallbackCount == 1);
}

TEST_CASE("Test callback for matN PropertyTableProperty (normalized)") {
  Model model;
  // clang-format off
  std::vector<glm::umat2x2> values = {
      glm::umat2x2(
        12, 34,
        30, 1),
      glm::umat2x2(
        11, 8,
        73, 102),
      glm::umat2x2(
        1, 0,
        63, 2),
      glm::umat2x2(
        4, 8,
        3, 23)};
  // clang-format on

  addBufferToModel(model, values);
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::MAT2;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT32;
  testClassProperty.normalized = true;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(values.size());

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::MAT2);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT32);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(classProperty->normalized);

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      "TestClassProperty",
      [&values, &invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) mutable {
        REQUIRE(
            propertyValue.status() == PropertyTablePropertyViewStatus::Valid);
        REQUIRE(propertyValue.size() > 0);

        if constexpr (std::is_same_v<
                          PropertyTablePropertyView<glm::umat2x2, true>,
                          decltype(propertyValue)>) {
          for (int64_t i = 0; i < propertyValue.size(); ++i) {
            auto expectedValue = values[static_cast<size_t>(i)];
            REQUIRE(
                static_cast<glm::umat2x2>(propertyValue.getRaw(i)) ==
                expectedValue);
            REQUIRE(propertyValue.get(i) == normalize(expectedValue));
          }
        } else {
          FAIL("getPropertyView returned PropertyTablePropertyView of "
               "incorrect type for TestClassProperty.");
        }
        invokedCallbackCount++;
      });

  REQUIRE(invokedCallbackCount == 1);
}

TEST_CASE("Test callback for boolean PropertyTableProperty") {
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

  addBufferToModel(model, values);

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::BOOLEAN;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(instanceCount);

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values =
      static_cast<int32_t>(model.bufferViews.size() - 1);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::BOOLEAN);
  REQUIRE(classProperty->componentType == std::nullopt);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      "TestClassProperty",
      [&expected, &invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) mutable {
        invokedCallbackCount++;

        REQUIRE(
            propertyValue.status() == PropertyTablePropertyViewStatus::Valid);
        REQUIRE(propertyValue.size() > 0);

        if constexpr (std::is_same_v<
                          PropertyTablePropertyView<bool>,
                          decltype(propertyValue)>) {
          for (int64_t i = 0; i < propertyValue.size(); ++i) {
            auto expectedValue = expected[static_cast<size_t>(i)];
            REQUIRE(
                static_cast<bool>(propertyValue.getRaw(i)) == expectedValue);
            REQUIRE(propertyValue.get(i) == expectedValue);
          }
        } else {
          FAIL("getPropertyView returned PropertyTablePropertyView of "
               "incorrect type for TestClassProperty.");
        }
      });

  REQUIRE(invokedCallbackCount == 1);
}

TEST_CASE("Test callback for string PropertyTableProperty") {
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

  addBufferToModel(model, values);
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  addBufferToModel(model, stringOffsets);
  size_t offsetBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::STRING;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(expected.size());

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.stringOffsetType =
      PropertyTableProperty::StringOffsetType::UINT32;
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);
  propertyTableProperty.stringOffsets =
      static_cast<int32_t>(offsetBufferViewIndex);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::STRING);
  REQUIRE(classProperty->componentType == std::nullopt);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      "TestClassProperty",
      [&expected, &invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) mutable {
        invokedCallbackCount++;
        REQUIRE(
            propertyValue.status() == PropertyTablePropertyViewStatus::Valid);
        REQUIRE(propertyValue.size() > 0);

        if constexpr (std::is_same_v<
                          PropertyTablePropertyView<std::string_view>,
                          decltype(propertyValue)>) {
          for (int64_t i = 0; i < propertyValue.size(); ++i) {
            auto expectedValue = expected[static_cast<size_t>(i)];
            REQUIRE(
                static_cast<std::string_view>(propertyValue.getRaw(i)) ==
                expectedValue);
            REQUIRE(propertyValue.get(i) == expectedValue);
          }
        } else {
          FAIL("getPropertyView returned PropertyTablePropertyView of "
               "incorrect type for TestClassProperty.");
        }
      });

  REQUIRE(invokedCallbackCount == 1);
}

TEST_CASE("Test callback for scalar array PropertyTableProperty") {
  Model model;
  std::vector<uint32_t> values =
      {12, 34, 30, 11, 34, 34, 11, 33, 122, 33, 223, 11};

  addBufferToModel(model, values);

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT32;
  testClassProperty.array = true;
  testClassProperty.count = 3;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(
      values.size() / static_cast<size_t>(testClassProperty.count.value()));

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values =
      static_cast<int32_t>(model.bufferViews.size() - 1);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT32);
  REQUIRE(classProperty->array);
  REQUIRE(classProperty->count == 3);
  REQUIRE(!classProperty->normalized);

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      "TestClassProperty",
      [&values, &invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) {
        invokedCallbackCount++;
        REQUIRE(
            propertyValue.status() == PropertyTablePropertyViewStatus::Valid);
        REQUIRE(propertyValue.size() > 0);

        if constexpr (std::is_same_v<
                          PropertyTablePropertyView<
                              PropertyArrayView<uint32_t>>,
                          decltype(propertyValue)>) {
          for (int64_t i = 0; i < propertyValue.size(); ++i) {
            PropertyArrayView<uint32_t> array = propertyValue.getRaw(i);
            auto maybeArray = propertyValue.get(i);
            REQUIRE(maybeArray);

            for (int64_t j = 0; j < array.size(); ++j) {
              REQUIRE(array[j] == values[static_cast<size_t>(i * 3 + j)]);
              REQUIRE((*maybeArray)[j] == array[j]);
            }
          }
        } else {
          FAIL("getPropertyView returned PropertyTablePropertyView of "
               "incorrect type for TestClassProperty.");
        }
      });

  REQUIRE(invokedCallbackCount == 1);
}

TEST_CASE("Test callback for scalar array PropertyTableProperty (normalized)") {
  Model model;
  std::vector<uint32_t> values =
      {12, 34, 30, 11, 34, 34, 11, 33, 122, 33, 223, 11};

  addBufferToModel(model, values);

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT32;
  testClassProperty.array = true;
  testClassProperty.count = 3;
  testClassProperty.normalized = true;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(
      values.size() / static_cast<size_t>(testClassProperty.count.value()));

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values =
      static_cast<int32_t>(model.bufferViews.size() - 1);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT32);
  REQUIRE(classProperty->array);
  REQUIRE(classProperty->count == 3);
  REQUIRE(classProperty->normalized);

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      "TestClassProperty",
      [&values, &invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) {
        invokedCallbackCount++;
        REQUIRE(
            propertyValue.status() == PropertyTablePropertyViewStatus::Valid);
        REQUIRE(propertyValue.size() > 0);

        if constexpr (
            std::is_same_v<
                PropertyTablePropertyView<PropertyArrayView<uint32_t>, true>,
                decltype(propertyValue)>) {
          for (int64_t i = 0; i < propertyValue.size(); ++i) {
            PropertyArrayView<uint32_t> array = propertyValue.getRaw(i);
            auto maybeArray = propertyValue.get(i);
            REQUIRE(maybeArray);

            for (int64_t j = 0; j < array.size(); ++j) {
              REQUIRE(array[j] == values[static_cast<size_t>(i * 3 + j)]);
              REQUIRE((*maybeArray)[j] == normalize(array[j]));
            }
          }
        } else {
          FAIL("getPropertyView returned PropertyTablePropertyView of "
               "incorrect type for TestClassProperty.");
        }
      });

  REQUIRE(invokedCallbackCount == 1);
}

TEST_CASE("Test callback for vecN array PropertyTableProperty") {
  Model model;
  std::vector<glm::ivec3> values = {
      glm::ivec3(12, 34, -30),
      glm::ivec3(-2, 0, 1),
      glm::ivec3(1, 2, 8),
      glm::ivec3(-100, 84, 6),
      glm::ivec3(2, -2, -2),
      glm::ivec3(40, 61, 3),
  };

  addBufferToModel(model, values);

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::VEC3;
  testClassProperty.componentType = ClassProperty::ComponentType::INT32;
  testClassProperty.array = true;
  testClassProperty.count = 2;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(
      values.size() / static_cast<size_t>(testClassProperty.count.value()));

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values =
      static_cast<int32_t>(model.bufferViews.size() - 1);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::VEC3);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT32);
  REQUIRE(classProperty->array);
  REQUIRE(classProperty->count == 2);
  REQUIRE(!classProperty->normalized);

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      "TestClassProperty",
      [&values, &invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) {
        invokedCallbackCount++;
        REQUIRE(
            propertyValue.status() == PropertyTablePropertyViewStatus::Valid);
        REQUIRE(propertyValue.size() > 0);

        if constexpr (std::is_same_v<
                          PropertyTablePropertyView<
                              PropertyArrayView<glm::ivec3>>,
                          decltype(propertyValue)>) {
          for (int64_t i = 0; i < propertyValue.size(); ++i) {
            PropertyArrayView<glm::ivec3> array = propertyValue.getRaw(i);
            auto maybeArray = propertyValue.get(i);
            REQUIRE(maybeArray);

            for (int64_t j = 0; j < array.size(); ++j) {
              REQUIRE(array[j] == values[static_cast<size_t>(i * 2 + j)]);
              REQUIRE((*maybeArray)[j] == array[j]);
            }
          }
        } else {
          FAIL("getPropertyView returned PropertyTablePropertyView of "
               "incorrect type for TestClassProperty.");
        }
      });

  REQUIRE(invokedCallbackCount == 1);
}

TEST_CASE("Test callback for vecN array PropertyTableProperty (normalized)") {
  Model model;
  std::vector<glm::ivec3> values = {
      glm::ivec3(12, 34, -30),
      glm::ivec3(-2, 0, 1),
      glm::ivec3(1, 2, 8),
      glm::ivec3(-100, 84, 6),
      glm::ivec3(2, -2, -2),
      glm::ivec3(40, 61, 3),
  };

  addBufferToModel(model, values);

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::VEC3;
  testClassProperty.componentType = ClassProperty::ComponentType::INT32;
  testClassProperty.array = true;
  testClassProperty.count = 2;
  testClassProperty.normalized = true;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(
      values.size() / static_cast<size_t>(testClassProperty.count.value()));

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values =
      static_cast<int32_t>(model.bufferViews.size() - 1);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::VEC3);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT32);
  REQUIRE(classProperty->array);
  REQUIRE(classProperty->count == 2);
  REQUIRE(classProperty->normalized);

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      "TestClassProperty",
      [&values, &invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) {
        invokedCallbackCount++;
        REQUIRE(
            propertyValue.status() == PropertyTablePropertyViewStatus::Valid);
        REQUIRE(propertyValue.size() > 0);

        if constexpr (
            std::is_same_v<
                PropertyTablePropertyView<PropertyArrayView<glm::ivec3>, true>,
                decltype(propertyValue)>) {
          for (int64_t i = 0; i < propertyValue.size(); ++i) {
            PropertyArrayView<glm::ivec3> array = propertyValue.getRaw(i);
            auto maybeArray = propertyValue.get(i);
            REQUIRE(maybeArray);

            for (int64_t j = 0; j < array.size(); ++j) {
              REQUIRE(array[j] == values[static_cast<size_t>(i * 2 + j)]);
              REQUIRE((*maybeArray)[j] == normalize(array[j]));
            }
          }
        } else {
          FAIL("getPropertyView returned PropertyTablePropertyView of "
               "incorrect type for TestClassProperty.");
        }
      });

  REQUIRE(invokedCallbackCount == 1);
}

TEST_CASE("Test callback for matN array PropertyTableProperty") {
  Model model;
  // clang-format off
  std::vector<glm::imat2x2> values = {
      glm::imat2x2(
        12, 34,
        -30, 20),
      glm::imat2x2(
        -2, -2,
        0, 1),
      glm::imat2x2(
        1, 2,
        8, 5),
      glm::imat2x2(
        -100, 3,
        84, 6),
      glm::imat2x2(
        2, 12,
        -2, -2),
      glm::imat2x2(
        40, 61,
        7, -3),
  };
  // clang-format on

  addBufferToModel(model, values);

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::MAT2;
  testClassProperty.componentType = ClassProperty::ComponentType::INT32;
  testClassProperty.array = true;
  testClassProperty.count = 2;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(
      values.size() / static_cast<size_t>(testClassProperty.count.value()));

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values =
      static_cast<int32_t>(model.bufferViews.size() - 1);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::MAT2);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT32);
  REQUIRE(classProperty->array);
  REQUIRE(classProperty->count == 2);

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      "TestClassProperty",
      [&values, &invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) {
        invokedCallbackCount++;
        REQUIRE(
            propertyValue.status() == PropertyTablePropertyViewStatus::Valid);
        REQUIRE(propertyValue.size() > 0);

        if constexpr (std::is_same_v<
                          PropertyTablePropertyView<
                              PropertyArrayView<glm::imat2x2>>,
                          decltype(propertyValue)>) {
          for (int64_t i = 0; i < propertyValue.size(); ++i) {
            PropertyArrayView<glm::imat2x2> member = propertyValue.getRaw(i);
            for (int64_t j = 0; j < member.size(); ++j) {
              REQUIRE(member[j] == values[static_cast<size_t>(i * 2 + j)]);
            }
          }
        } else {
          FAIL("getPropertyView returned PropertyTablePropertyView of "
               "incorrect type for TestClassProperty.");
        }
      });

  REQUIRE(invokedCallbackCount == 1);
}

TEST_CASE("Test callback for boolean array PropertyTableProperty") {
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

  addBufferToModel(model, values);

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::BOOLEAN;
  testClassProperty.array = true;
  testClassProperty.count = 3;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(
      expected.size() / static_cast<size_t>(testClassProperty.count.value()));

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.values =
      static_cast<int32_t>(model.bufferViews.size() - 1);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::BOOLEAN);
  REQUIRE(classProperty->array);
  REQUIRE(classProperty->count == 3);

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      "TestClassProperty",
      [&expected, &invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) {
        invokedCallbackCount++;
        REQUIRE(
            propertyValue.status() == PropertyTablePropertyViewStatus::Valid);
        REQUIRE(propertyValue.size() > 0);

        if constexpr (std::is_same_v<
                          PropertyTablePropertyView<PropertyArrayView<bool>>,
                          decltype(propertyValue)>) {
          for (int64_t i = 0; i < propertyValue.size(); ++i) {
            PropertyArrayView<bool> array = propertyValue.getRaw(i);
            auto maybeArray = propertyValue.get(i);
            REQUIRE(maybeArray);

            for (int64_t j = 0; j < array.size(); ++j) {
              REQUIRE(array[j] == expected[static_cast<size_t>(i * 3 + j)]);
              REQUIRE((*maybeArray)[j] == array[j]);
            }
          }
        } else {
          FAIL("getPropertyView returned PropertyTablePropertyView of "
               "incorrect type for TestClassProperty.");
        }
      });

  REQUIRE(invokedCallbackCount == 1);
}

TEST_CASE("Test callback for string array PropertyTableProperty") {
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

  addBufferToModel(model, values);
  size_t valueBufferViewIndex = model.bufferViews.size() - 1;

  addBufferToModel(model, offsets);
  size_t offsetBufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::STRING;
  testClassProperty.array = true;
  testClassProperty.count = 2;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = static_cast<int64_t>(
      expected.size() / static_cast<size_t>(testClassProperty.count.value()));

  PropertyTableProperty& propertyTableProperty =
      propertyTable.properties["TestClassProperty"];
  propertyTableProperty.stringOffsetType =
      PropertyTableProperty::StringOffsetType::UINT32;
  propertyTableProperty.values = static_cast<int32_t>(valueBufferViewIndex);
  propertyTableProperty.stringOffsets =
      static_cast<int32_t>(offsetBufferViewIndex);

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::STRING);
  REQUIRE(classProperty->array);
  REQUIRE(classProperty->count == 2);

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      "TestClassProperty",
      [&invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) {
        invokedCallbackCount++;
        REQUIRE(
            propertyValue.status() == PropertyTablePropertyViewStatus::Valid);
        REQUIRE(propertyValue.size() == 3);

        if constexpr (std::is_same_v<
                          PropertyTablePropertyView<
                              PropertyArrayView<std::string_view>>,
                          decltype(propertyValue)>) {
          PropertyArrayView<std::string_view> v0 = propertyValue.getRaw(0);
          REQUIRE(v0.size() == 2);
          REQUIRE(v0[0] == "What's up");
          REQUIRE(
              v0[1] ==
              "Breaking news!!! Aliens no longer attacks the US first");

          PropertyArrayView<std::string_view> v1 = propertyValue.getRaw(1);
          REQUIRE(v1.size() == 2);
          REQUIRE(
              v1[0] == "But they still abduct my cows! Those milk thiefs! 👽 🐮");
          REQUIRE(v1[1] == "I'm not crazy. My mother had me tested 🤪");

          PropertyArrayView<std::string_view> v2 = propertyValue.getRaw(2);
          REQUIRE(v2.size() == 2);
          REQUIRE(v2[0] == "I love you, meat bags! ❤️");
          REQUIRE(v2[1] == "Book in the freezer");

          for (int64_t i = 0; i < propertyValue.size(); i++) {
            auto maybeValue = propertyValue.get(i);
            REQUIRE(maybeValue);

            auto value = propertyValue.getRaw(i);
            REQUIRE(maybeValue->size() == value.size());
            for (int64_t j = 0; j < value.size(); j++) {
              REQUIRE((*maybeValue)[j] == value[j]);
            }
          }
        } else {
          FAIL("getPropertyView returned PropertyTablePropertyView of "
               "incorrect type for TestClassProperty.");
        }
      });

  REQUIRE(invokedCallbackCount == 1);
}

TEST_CASE("Test callback for empty PropertyTableProperty with default value") {
  Model model;
  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT32;

  const uint32_t defaultValue = 10;
  testClassProperty.defaultProperty = defaultValue;

  PropertyTable& propertyTable = metadata.propertyTables.emplace_back();
  propertyTable.classProperty = "TestClass";
  propertyTable.count = 4;

  PropertyTableView view(model, propertyTable);
  REQUIRE(view.status() == PropertyTableViewStatus::Valid);
  REQUIRE(view.size() == propertyTable.count);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT32);
  REQUIRE(!classProperty->array);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->normalized);
  REQUIRE(classProperty->defaultProperty);

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      "TestClassProperty",
      [defaultValue, count = propertyTable.count, &invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) mutable {
        invokedCallbackCount++;
        if constexpr (std::is_same_v<
                          PropertyTablePropertyView<uint32_t>,
                          decltype(propertyValue)>) {
          REQUIRE(
              propertyValue.status() ==
              PropertyTablePropertyViewStatus::EmptyPropertyWithDefault);
          REQUIRE(propertyValue.size() == count);
          REQUIRE(propertyValue.defaultValue() == defaultValue);

          for (int64_t i = 0; i < propertyValue.size(); ++i) {
            REQUIRE(propertyValue.get(i) == defaultValue);
          }
        } else {
          FAIL("getPropertyView returned PropertyTablePropertyView of "
               "incorrect type for TestClassProperty.");
        }
      });

  REQUIRE(invokedCallbackCount == 1);
}
