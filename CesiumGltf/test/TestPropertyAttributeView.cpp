#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/Buffer.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/Class.h>
#include <CesiumGltf/ClassProperty.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/PropertyAttribute.h>
#include <CesiumGltf/PropertyAttributeProperty.h>
#include <CesiumGltf/PropertyAttributePropertyView.h>
#include <CesiumGltf/PropertyAttributeView.h>
#include <CesiumGltf/PropertyTransformations.h>
#include <CesiumGltf/PropertyType.h>
#include <CesiumGltf/PropertyTypeTraits.h>
#include <CesiumGltf/Schema.h>
#include <CesiumUtility/Assert.h>

#include <doctest/doctest.h>
#include <glm/ext/matrix_double2x2.hpp>
#include <glm/ext/matrix_float2x2.hpp>
#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_int2_sized.hpp>
#include <glm/ext/vector_uint2_sized.hpp>
#include <glm/ext/vector_uint3_sized.hpp>
#include <glm/fwd.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

using namespace CesiumGltf;

namespace {
template <typename T, bool Normalized = false>
void addAttributeToModel(
    Model& model,
    MeshPrimitive& primitive,
    const std::string& name,
    const std::vector<T>& values) {
  Buffer& buffer = model.buffers.emplace_back();
  buffer.cesium.data.resize(values.size() * sizeof(T));
  buffer.byteLength = static_cast<int64_t>(buffer.cesium.data.size());
  std::memcpy(
      buffer.cesium.data.data(),
      values.data(),
      buffer.cesium.data.size());

  BufferView& bufferView = model.bufferViews.emplace_back();
  bufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
  bufferView.byteOffset = 0;
  bufferView.byteLength = buffer.byteLength;

  Accessor& accessor = model.accessors.emplace_back();
  accessor.bufferView = static_cast<int32_t>(model.bufferViews.size() - 1);
  accessor.count = static_cast<int64_t>(values.size());
  accessor.byteOffset = 0;

  PropertyType type = TypeToPropertyType<T>::value;
  switch (type) {
  case PropertyType::Scalar:
    accessor.type = Accessor::Type::SCALAR;
    break;
  case PropertyType::Vec2:
    accessor.type = Accessor::Type::VEC2;
    break;
  case PropertyType::Vec3:
    accessor.type = Accessor::Type::VEC3;
    break;
  case PropertyType::Vec4:
    accessor.type = Accessor::Type::VEC4;
    break;
  case PropertyType::Mat2:
    accessor.type = Accessor::Type::MAT2;
    break;
  case PropertyType::Mat3:
    accessor.type = Accessor::Type::MAT3;
    break;
  case PropertyType::Mat4:
    accessor.type = Accessor::Type::MAT4;
    break;
  default:
    CESIUM_ASSERT(false && "Input type is not supported as an accessor type");
    break;
  }

  PropertyComponentType componentType = TypeToPropertyType<T>::component;
  switch (componentType) {
  case PropertyComponentType::Int8:
    accessor.componentType = Accessor::ComponentType::BYTE;
    break;
  case PropertyComponentType::Uint8:
    accessor.componentType = Accessor::ComponentType::UNSIGNED_BYTE;
    break;
  case PropertyComponentType::Int16:
    accessor.componentType = Accessor::ComponentType::SHORT;
    break;
  case PropertyComponentType::Uint16:
    accessor.componentType = Accessor::ComponentType::UNSIGNED_SHORT;
    break;
  case PropertyComponentType::Float32:
    accessor.componentType = Accessor::ComponentType::FLOAT;
    break;
  default:
    CESIUM_ASSERT(
        false &&
        "Input component type is not supported as an accessor component type");
    break;
  }

  accessor.normalized = Normalized;

  primitive.attributes[name] = static_cast<int32_t>(model.accessors.size() - 1);
}
} // namespace

TEST_CASE("Test PropertyAttributeView on model without EXT_structural_metadata "
          "extension") {
  Model model;

  // Create an erroneously isolated property Attribute.
  PropertyAttribute propertyAttribute;
  propertyAttribute.classProperty = "TestClass";

  PropertyAttributeProperty& propertyAttributeProperty =
      propertyAttribute.properties["TestClassProperty"];
  propertyAttributeProperty.attribute = "_ATTRIBUTE";

  PropertyAttributeView view(model, propertyAttribute);
  REQUIRE(
      view.status() ==
      PropertyAttributeViewStatus::ErrorMissingMetadataExtension);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(!classProperty);
}

TEST_CASE("Test PropertyAttributeView on model without metadata schema") {
  Model model;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  PropertyAttribute& propertyAttribute =
      metadata.propertyAttributes.emplace_back();
  propertyAttribute.classProperty = "TestClass";

  PropertyAttributeProperty& propertyAttributeProperty =
      propertyAttribute.properties["TestClassProperty"];
  propertyAttributeProperty.attribute = "_ATTRIBUTE";

  PropertyAttributeView view(model, propertyAttribute);
  REQUIRE(view.status() == PropertyAttributeViewStatus::ErrorMissingSchema);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(!classProperty);
}

TEST_CASE("Test property attribute with nonexistent class") {
  Model model;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT8;

  PropertyAttribute& propertyAttribute =
      metadata.propertyAttributes.emplace_back();
  propertyAttribute.classProperty = "I Don't Exist";

  PropertyAttributeProperty& propertyAttributeProperty =
      propertyAttribute.properties["TestClassProperty"];
  propertyAttributeProperty.attribute = "_ATTRIBUTE";

  PropertyAttributeView view(model, propertyAttribute);
  REQUIRE(view.status() == PropertyAttributeViewStatus::ErrorClassNotFound);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(!classProperty);
}

TEST_CASE("Test scalar PropertyAttributeProperty") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

  const std::string attributeName = "_ATTRIBUTE";
  std::vector<uint16_t> data = {12, 34, 30, 11};

  addAttributeToModel(model, primitive, attributeName, data);
  size_t accessorIndex = model.accessors.size() - 1;
  size_t bufferIndex = model.buffers.size() - 1;
  size_t bufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT16;

  PropertyAttribute& propertyAttribute =
      metadata.propertyAttributes.emplace_back();
  propertyAttribute.classProperty = "TestClass";

  PropertyAttributeProperty& propertyAttributeProperty =
      propertyAttribute.properties["TestClassProperty"];
  propertyAttributeProperty.attribute = attributeName;

  PropertyAttributeView view(model, propertyAttribute);
  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT16);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(!classProperty->normalized);

  SUBCASE("Access correct type") {
    PropertyAttributePropertyView<uint16_t> uint16Property =
        view.getPropertyView<uint16_t>(primitive, "TestClassProperty");
    REQUIRE(
        uint16Property.status() == PropertyAttributePropertyViewStatus::Valid);
    for (size_t i = 0; i < data.size(); ++i) {
      REQUIRE(uint16Property.getRaw(static_cast<int64_t>(i)) == data[i]);
      REQUIRE(uint16Property.get(static_cast<int64_t>(i)) == data[i]);
    }
  }

  SUBCASE("Access wrong type") {
    PropertyAttributePropertyView<glm::u16vec2> u16vec2Invalid =
        view.getPropertyView<glm::u16vec2>(primitive, "TestClassProperty");
    REQUIRE(
        u16vec2Invalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Access wrong component type") {
    PropertyAttributePropertyView<uint8_t> uint8Invalid =
        view.getPropertyView<uint8_t>(primitive, "TestClassProperty");
    REQUIRE(
        uint8Invalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorComponentTypeMismatch);

    PropertyAttributePropertyView<int32_t> int32Invalid =
        view.getPropertyView<int32_t>(primitive, "TestClassProperty");
    REQUIRE(
        int32Invalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorComponentTypeMismatch);

    PropertyAttributePropertyView<float> uint64Invalid =
        view.getPropertyView<float>(primitive, "TestClassProperty");
    REQUIRE(
        uint64Invalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Access incorrectly as normalized") {
    PropertyAttributePropertyView<uint16_t, true> normalizedInvalid =
        view.getPropertyView<uint16_t, true>(primitive, "TestClassProperty");
    REQUIRE(
        normalizedInvalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Buffer view points outside of the real buffer length") {
    model.buffers[bufferIndex].cesium.data.resize(4);
    PropertyAttributePropertyView<uint16_t> property =
        view.getPropertyView<uint16_t>(primitive, "TestClassProperty");
    REQUIRE(
        property.status() ==
        PropertyAttributePropertyViewStatus::ErrorBufferViewOutOfBounds);
  }

  SUBCASE("Wrong buffer index") {
    model.bufferViews[bufferViewIndex].buffer = 2;
    PropertyAttributePropertyView<uint16_t> property =
        view.getPropertyView<uint16_t>(primitive, "TestClassProperty");
    REQUIRE(
        property.status() ==
        PropertyAttributePropertyViewStatus::ErrorInvalidBuffer);
  }

  SUBCASE("Accessor view points outside of buffer viwe length") {
    model.accessors[accessorIndex].count = 10;
    PropertyAttributePropertyView<uint16_t> property =
        view.getPropertyView<uint16_t>(primitive, "TestClassProperty");
    REQUIRE(
        property.status() ==
        PropertyAttributePropertyViewStatus::ErrorAccessorOutOfBounds);
  }

  SUBCASE("Wrong buffer view index") {
    model.accessors[accessorIndex].bufferView = -1;
    PropertyAttributePropertyView<uint16_t> property =
        view.getPropertyView<uint16_t>(primitive, "TestClassProperty");
    REQUIRE(
        property.status() ==
        PropertyAttributePropertyViewStatus::ErrorInvalidBufferView);
  }

  SUBCASE("Wrong accessor normalization") {
    model.accessors[accessorIndex].normalized = true;
    PropertyAttributePropertyView<uint16_t> property =
        view.getPropertyView<uint16_t>(primitive, "TestClassProperty");
    REQUIRE(
        property.status() == PropertyAttributePropertyViewStatus::
                                 ErrorAccessorNormalizationMismatch);
  }

  SUBCASE("Wrong accessor component type") {
    model.accessors[accessorIndex].componentType =
        Accessor::ComponentType::SHORT;
    PropertyAttributePropertyView<uint16_t> property =
        view.getPropertyView<uint16_t>(primitive, "TestClassProperty");
    REQUIRE(
        property.status() == PropertyAttributePropertyViewStatus::
                                 ErrorAccessorComponentTypeMismatch);
  }

  SUBCASE("Wrong accessor type") {
    model.accessors[accessorIndex].type = Accessor::Type::VEC2;
    PropertyAttributePropertyView<uint16_t> property =
        view.getPropertyView<uint16_t>(primitive, "TestClassProperty");
    REQUIRE(
        property.status() ==
        PropertyAttributePropertyViewStatus::ErrorAccessorTypeMismatch);
  }

  SUBCASE("Wrong accessor index") {
    primitive.attributes[attributeName] = -1;
    PropertyAttributePropertyView<uint16_t> property =
        view.getPropertyView<uint16_t>(primitive, "TestClassProperty");
    REQUIRE(
        property.status() ==
        PropertyAttributePropertyViewStatus::ErrorInvalidAccessor);
  }

  SUBCASE("Missing attribute") {
    primitive.attributes.clear();
    PropertyAttributePropertyView<uint16_t> property =
        view.getPropertyView<uint16_t>(primitive, "TestClassProperty");
    REQUIRE(
        property.status() ==
        PropertyAttributePropertyViewStatus::ErrorMissingAttribute);
  }
}

TEST_CASE("Test scalar PropertyAttributeProperty (normalized)") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

  const std::string attributeName = "_ATTRIBUTE";
  std::vector<uint8_t> data = {12, 34, 30, 11};

  addAttributeToModel<uint8_t, true>(model, primitive, attributeName, data);
  size_t accessorIndex = model.accessors.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT8;
  testClassProperty.normalized = true;

  PropertyAttribute& propertyAttribute =
      metadata.propertyAttributes.emplace_back();
  propertyAttribute.classProperty = "TestClass";

  PropertyAttributeProperty& propertyAttributeProperty =
      propertyAttribute.properties["TestClassProperty"];
  propertyAttributeProperty.attribute = attributeName;

  PropertyAttributeView view(model, propertyAttribute);
  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT8);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(classProperty->normalized);

  SUBCASE("Access correct type") {
    PropertyAttributePropertyView<uint8_t, true> uint8Property =
        view.getPropertyView<uint8_t, true>(primitive, "TestClassProperty");
    REQUIRE(
        uint8Property.status() == PropertyAttributePropertyViewStatus::Valid);

    for (size_t i = 0; i < data.size(); ++i) {
      REQUIRE(uint8Property.getRaw(static_cast<int64_t>(i)) == data[i]);
      REQUIRE(uint8Property.get(static_cast<int64_t>(i)) == normalize(data[i]));
    }
  }

  SUBCASE("Access wrong type") {
    PropertyAttributePropertyView<glm::u8vec2, true> u8vec2Invalid =
        view.getPropertyView<glm::u8vec2, true>(primitive, "TestClassProperty");
    REQUIRE(
        u8vec2Invalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Access wrong component type") {
    PropertyAttributePropertyView<uint16_t, true> uint16Invalid =
        view.getPropertyView<uint16_t, true>(primitive, "TestClassProperty");
    REQUIRE(
        uint16Invalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorComponentTypeMismatch);

    PropertyAttributePropertyView<int32_t> int32Invalid =
        view.getPropertyView<int32_t>(primitive, "TestClassProperty");
    REQUIRE(
        int32Invalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Access incorrectly as non-normalized") {
    PropertyAttributePropertyView<uint8_t> normalizedInvalid =
        view.getPropertyView<uint8_t>(primitive, "TestClassProperty");
    REQUIRE(
        normalizedInvalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Access incorrectly as double") {
    PropertyAttributePropertyView<double> doubleInvalid =
        view.getPropertyView<double>(primitive, "TestClassProperty");
    REQUIRE(
        doubleInvalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Wrong accessor normalization") {
    model.accessors[accessorIndex].normalized = false;
    PropertyAttributePropertyView<uint8_t, true> property =
        view.getPropertyView<uint8_t, true>(primitive, "TestClassProperty");
    REQUIRE(
        property.status() == PropertyAttributePropertyViewStatus::
                                 ErrorAccessorNormalizationMismatch);
  }
}

TEST_CASE("Test vecN PropertyAttributeProperty") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

  const std::string attributeName = "_ATTRIBUTE";
  std::vector<glm::u8vec2> data = {
      glm::u8vec2(12, 34),
      glm::u8vec2(10, 3),
      glm::u8vec2(40, 0),
      glm::u8vec2(30, 11)};

  addAttributeToModel(model, primitive, attributeName, data);
  size_t accessorIndex = model.accessors.size() - 1;
  size_t bufferIndex = model.buffers.size() - 1;
  size_t bufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::VEC2;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT8;

  PropertyAttribute& propertyAttribute =
      metadata.propertyAttributes.emplace_back();
  propertyAttribute.classProperty = "TestClass";

  PropertyAttributeProperty& propertyAttributeProperty =
      propertyAttribute.properties["TestClassProperty"];
  propertyAttributeProperty.attribute = attributeName;

  PropertyAttributeView view(model, propertyAttribute);
  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::VEC2);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT8);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(!classProperty->normalized);

  SUBCASE("Access correct type") {
    PropertyAttributePropertyView<glm::u8vec2> u8vec2Property =
        view.getPropertyView<glm::u8vec2>(primitive, "TestClassProperty");
    REQUIRE(
        u8vec2Property.status() == PropertyAttributePropertyViewStatus::Valid);
    for (size_t i = 0; i < data.size(); ++i) {
      REQUIRE(u8vec2Property.getRaw(static_cast<int64_t>(i)) == data[i]);
      REQUIRE(u8vec2Property.get(static_cast<int64_t>(i)) == data[i]);
    }
  }

  SUBCASE("Access wrong type") {
    PropertyAttributePropertyView<uint8_t> uint8Invalid =
        view.getPropertyView<uint8_t>(primitive, "TestClassProperty");
    REQUIRE(
        uint8Invalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorTypeMismatch);

    PropertyAttributePropertyView<glm::u8vec3> u8vec3Invalid =
        view.getPropertyView<glm::u8vec3>(primitive, "TestClassProperty");
    REQUIRE(
        u8vec3Invalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Access wrong component type") {
    PropertyAttributePropertyView<glm::vec2> vec2Invalid =
        view.getPropertyView<glm::vec2>(primitive, "TestClassProperty");
    REQUIRE(
        vec2Invalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Access incorrectly as normalized") {
    PropertyAttributePropertyView<glm::u8vec2, true> normalizedInvalid =
        view.getPropertyView<glm::u8vec2, true>(primitive, "TestClassProperty");
    REQUIRE(
        normalizedInvalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Buffer view points outside of the real buffer length") {
    model.buffers[bufferIndex].cesium.data.resize(4);
    PropertyAttributePropertyView<glm::u8vec2> property =
        view.getPropertyView<glm::u8vec2>(primitive, "TestClassProperty");
    REQUIRE(
        property.status() ==
        PropertyAttributePropertyViewStatus::ErrorBufferViewOutOfBounds);
  }

  SUBCASE("Wrong buffer index") {
    model.bufferViews[bufferViewIndex].buffer = 2;
    PropertyAttributePropertyView<glm::u8vec2> property =
        view.getPropertyView<glm::u8vec2>(primitive, "TestClassProperty");
    REQUIRE(
        property.status() ==
        PropertyAttributePropertyViewStatus::ErrorInvalidBuffer);
  }

  SUBCASE("Accessor view points outside of buffer viwe length") {
    model.accessors[accessorIndex].count = 10;
    PropertyAttributePropertyView<glm::u8vec2> property =
        view.getPropertyView<glm::u8vec2>(primitive, "TestClassProperty");
    REQUIRE(
        property.status() ==
        PropertyAttributePropertyViewStatus::ErrorAccessorOutOfBounds);
  }

  SUBCASE("Wrong buffer view index") {
    model.accessors[accessorIndex].bufferView = -1;
    PropertyAttributePropertyView<glm::u8vec2> property =
        view.getPropertyView<glm::u8vec2>(primitive, "TestClassProperty");
    REQUIRE(
        property.status() ==
        PropertyAttributePropertyViewStatus::ErrorInvalidBufferView);
  }

  SUBCASE("Wrong accessor normalization") {
    model.accessors[accessorIndex].normalized = true;
    PropertyAttributePropertyView<glm::u8vec2> property =
        view.getPropertyView<glm::u8vec2>(primitive, "TestClassProperty");
    REQUIRE(
        property.status() == PropertyAttributePropertyViewStatus::
                                 ErrorAccessorNormalizationMismatch);
  }

  SUBCASE("Wrong accessor component type") {
    model.accessors[accessorIndex].componentType =
        Accessor::ComponentType::BYTE;
    PropertyAttributePropertyView<glm::u8vec2> property =
        view.getPropertyView<glm::u8vec2>(primitive, "TestClassProperty");
    REQUIRE(
        property.status() == PropertyAttributePropertyViewStatus::
                                 ErrorAccessorComponentTypeMismatch);
  }

  SUBCASE("Wrong accessor type") {
    model.accessors[accessorIndex].type = Accessor::Type::SCALAR;
    PropertyAttributePropertyView<glm::u8vec2> property =
        view.getPropertyView<glm::u8vec2>(primitive, "TestClassProperty");
    REQUIRE(
        property.status() ==
        PropertyAttributePropertyViewStatus::ErrorAccessorTypeMismatch);
  }

  SUBCASE("Wrong accessor index") {
    primitive.attributes[attributeName] = -1;
    PropertyAttributePropertyView<glm::u8vec2> property =
        view.getPropertyView<glm::u8vec2>(primitive, "TestClassProperty");
    REQUIRE(
        property.status() ==
        PropertyAttributePropertyViewStatus::ErrorInvalidAccessor);
  }

  SUBCASE("Missing attribute") {
    primitive.attributes.clear();
    PropertyAttributePropertyView<glm::u8vec2> property =
        view.getPropertyView<glm::u8vec2>(primitive, "TestClassProperty");
    REQUIRE(
        property.status() ==
        PropertyAttributePropertyViewStatus::ErrorMissingAttribute);
  }
}

TEST_CASE("Test vecN PropertyAttributeProperty (normalized)") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

  const std::string attributeName = "_ATTRIBUTE";
  std::vector<glm::u8vec2> data = {
      glm::u8vec2(12, 34),
      glm::u8vec2(10, 3),
      glm::u8vec2(40, 0),
      glm::u8vec2(30, 11)};

  addAttributeToModel<glm::u8vec2, true>(model, primitive, attributeName, data);
  size_t accessorIndex = model.accessors.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::VEC2;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT8;
  testClassProperty.normalized = true;

  PropertyAttribute& propertyAttribute =
      metadata.propertyAttributes.emplace_back();
  propertyAttribute.classProperty = "TestClass";

  PropertyAttributeProperty& propertyAttributeProperty =
      propertyAttribute.properties["TestClassProperty"];
  propertyAttributeProperty.attribute = attributeName;

  PropertyAttributeView view(model, propertyAttribute);
  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::VEC2);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT8);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(classProperty->normalized);

  SUBCASE("Access correct type") {
    PropertyAttributePropertyView<glm::u8vec2, true> u8vec2Property =
        view.getPropertyView<glm::u8vec2, true>(primitive, "TestClassProperty");
    REQUIRE(
        u8vec2Property.status() == PropertyAttributePropertyViewStatus::Valid);
    for (size_t i = 0; i < data.size(); ++i) {
      REQUIRE(u8vec2Property.getRaw(static_cast<int64_t>(i)) == data[i]);
      REQUIRE(
          u8vec2Property.get(static_cast<int64_t>(i)) == normalize(data[i]));
    }
  }

  SUBCASE("Access wrong type") {
    PropertyAttributePropertyView<uint8_t, true> uint8Invalid =
        view.getPropertyView<uint8_t, true>(primitive, "TestClassProperty");
    REQUIRE(
        uint8Invalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorTypeMismatch);

    PropertyAttributePropertyView<glm::u8vec3, true> u8vec3Invalid =
        view.getPropertyView<glm::u8vec3, true>(primitive, "TestClassProperty");
    REQUIRE(
        u8vec3Invalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Access wrong component type") {
    PropertyAttributePropertyView<glm::u16vec2, true> u16vec2Invalid =
        view.getPropertyView<glm::u16vec2, true>(
            primitive,
            "TestClassProperty");
    REQUIRE(
        u16vec2Invalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorComponentTypeMismatch);

    PropertyAttributePropertyView<glm::i8vec2, true> i8vec2Invalid =
        view.getPropertyView<glm::i8vec2, true>(primitive, "TestClassProperty");
    REQUIRE(
        i8vec2Invalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Access incorrectly as non-normalized") {
    PropertyAttributePropertyView<glm::u8vec2> normalizedInvalid =
        view.getPropertyView<glm::u8vec2>(primitive, "TestClassProperty");
    REQUIRE(
        normalizedInvalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Access incorrectly as dvec2") {
    PropertyAttributePropertyView<glm::dvec2> dvec2Invalid =
        view.getPropertyView<glm::dvec2>(primitive, "TestClassProperty");
    REQUIRE(
        dvec2Invalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Wrong accessor normalization") {
    model.accessors[accessorIndex].normalized = false;
    PropertyAttributePropertyView<glm::u8vec2, true> property =
        view.getPropertyView<glm::u8vec2, true>(primitive, "TestClassProperty");
    REQUIRE(
        property.status() == PropertyAttributePropertyViewStatus::
                                 ErrorAccessorNormalizationMismatch);
  }
}

TEST_CASE("Test matN PropertyAttributeProperty") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

  const std::string attributeName = "_ATTRIBUTE";
  // clang-format off
  std::vector<glm::u16mat2x2> data = {
      glm::u16mat2x2(
        12, 34,
        30, 1),
      glm::u16mat2x2(
        11, 8,
        73, 102),
      glm::u16mat2x2(
        1, 0,
        63, 2),
      glm::u16mat2x2(
        4, 8,
        3, 23)};
  // clang-format on

  addAttributeToModel(model, primitive, attributeName, data);
  size_t accessorIndex = model.accessors.size() - 1;
  size_t bufferIndex = model.buffers.size() - 1;
  size_t bufferViewIndex = model.bufferViews.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::MAT2;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT16;

  PropertyAttribute& propertyAttribute =
      metadata.propertyAttributes.emplace_back();
  propertyAttribute.classProperty = "TestClass";

  PropertyAttributeProperty& propertyAttributeProperty =
      propertyAttribute.properties["TestClassProperty"];
  propertyAttributeProperty.attribute = attributeName;

  PropertyAttributeView view(model, propertyAttribute);
  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::MAT2);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT16);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(!classProperty->normalized);

  SUBCASE("Access correct type") {
    PropertyAttributePropertyView<glm::u16mat2x2> u16mat2x2Property =
        view.getPropertyView<glm::u16mat2x2>(primitive, "TestClassProperty");
    REQUIRE(
        u16mat2x2Property.status() ==
        PropertyAttributePropertyViewStatus::Valid);
    for (size_t i = 0; i < data.size(); ++i) {
      REQUIRE(u16mat2x2Property.getRaw(static_cast<int64_t>(i)) == data[i]);
      REQUIRE(u16mat2x2Property.get(static_cast<int64_t>(i)) == data[i]);
    }
  }

  SUBCASE("Access wrong type") {
    PropertyAttributePropertyView<uint16_t> uint16Invalid =
        view.getPropertyView<uint16_t>(primitive, "TestClassProperty");
    REQUIRE(
        uint16Invalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorTypeMismatch);

    PropertyAttributePropertyView<glm::u16vec2> u16vec2Invalid =
        view.getPropertyView<glm::u16vec2>(primitive, "TestClassProperty");
    REQUIRE(
        u16vec2Invalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorTypeMismatch);

    PropertyAttributePropertyView<glm::u16mat4x4> u16mat4x4Invalid =
        view.getPropertyView<glm::u16mat4x4>(primitive, "TestClassProperty");
    REQUIRE(
        u16mat4x4Invalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Access wrong component type") {
    PropertyAttributePropertyView<glm::mat2> mat2Invalid =
        view.getPropertyView<glm::mat2>(primitive, "TestClassProperty");
    REQUIRE(
        mat2Invalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Access incorrectly as normalized") {
    PropertyAttributePropertyView<glm::u16mat2x2, true> normalizedInvalid =
        view.getPropertyView<glm::u16mat2x2, true>(
            primitive,
            "TestClassProperty");
    REQUIRE(
        normalizedInvalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Buffer view points outside of the real buffer length") {
    model.buffers[bufferIndex].cesium.data.resize(4);
    PropertyAttributePropertyView<glm::u16mat2x2> property =
        view.getPropertyView<glm::u16mat2x2>(primitive, "TestClassProperty");
    REQUIRE(
        property.status() ==
        PropertyAttributePropertyViewStatus::ErrorBufferViewOutOfBounds);
  }

  SUBCASE("Wrong buffer index") {
    model.bufferViews[bufferViewIndex].buffer = 2;
    PropertyAttributePropertyView<glm::u16mat2x2> property =
        view.getPropertyView<glm::u16mat2x2>(primitive, "TestClassProperty");
    REQUIRE(
        property.status() ==
        PropertyAttributePropertyViewStatus::ErrorInvalidBuffer);
  }

  SUBCASE("Accessor view points outside of buffer viwe length") {
    model.accessors[accessorIndex].count = 10;
    PropertyAttributePropertyView<glm::u16mat2x2> property =
        view.getPropertyView<glm::u16mat2x2>(primitive, "TestClassProperty");
    REQUIRE(
        property.status() ==
        PropertyAttributePropertyViewStatus::ErrorAccessorOutOfBounds);
  }

  SUBCASE("Wrong buffer view index") {
    model.accessors[accessorIndex].bufferView = -1;
    PropertyAttributePropertyView<glm::u16mat2x2> property =
        view.getPropertyView<glm::u16mat2x2>(primitive, "TestClassProperty");
    REQUIRE(
        property.status() ==
        PropertyAttributePropertyViewStatus::ErrorInvalidBufferView);
  }

  SUBCASE("Wrong accessor normalization") {
    model.accessors[accessorIndex].normalized = true;
    PropertyAttributePropertyView<glm::u16mat2x2> property =
        view.getPropertyView<glm::u16mat2x2>(primitive, "TestClassProperty");
    REQUIRE(
        property.status() == PropertyAttributePropertyViewStatus::
                                 ErrorAccessorNormalizationMismatch);
  }

  SUBCASE("Wrong accessor component type") {
    model.accessors[accessorIndex].componentType =
        Accessor::ComponentType::BYTE;
    PropertyAttributePropertyView<glm::u16mat2x2> property =
        view.getPropertyView<glm::u16mat2x2>(primitive, "TestClassProperty");
    REQUIRE(
        property.status() == PropertyAttributePropertyViewStatus::
                                 ErrorAccessorComponentTypeMismatch);
  }

  SUBCASE("Wrong accessor type") {
    model.accessors[accessorIndex].type = Accessor::Type::SCALAR;
    PropertyAttributePropertyView<glm::u16mat2x2> property =
        view.getPropertyView<glm::u16mat2x2>(primitive, "TestClassProperty");
    REQUIRE(
        property.status() ==
        PropertyAttributePropertyViewStatus::ErrorAccessorTypeMismatch);
  }

  SUBCASE("Wrong accessor index") {
    primitive.attributes[attributeName] = -1;
    PropertyAttributePropertyView<glm::u16mat2x2> property =
        view.getPropertyView<glm::u16mat2x2>(primitive, "TestClassProperty");
    REQUIRE(
        property.status() ==
        PropertyAttributePropertyViewStatus::ErrorInvalidAccessor);
  }

  SUBCASE("Missing attribute") {
    primitive.attributes.clear();
    PropertyAttributePropertyView<glm::u16mat2x2> property =
        view.getPropertyView<glm::u16mat2x2>(primitive, "TestClassProperty");
    REQUIRE(
        property.status() ==
        PropertyAttributePropertyViewStatus::ErrorMissingAttribute);
  }
}

TEST_CASE("Test matN PropertyAttributeProperty (normalized)") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

  const std::string attributeName = "_ATTRIBUTE";
  // clang-format off
  std::vector<glm::u16mat2x2> data = {
      glm::u16mat2x2(
        12, 34,
        30, 1),
      glm::u16mat2x2(
        11, 8,
        73, 102),
      glm::u16mat2x2(
        1, 0,
        63, 2),
      glm::u16mat2x2(
        4, 8,
        3, 23)};
  // clang-format on

  addAttributeToModel<glm::u16mat2x2, true>(
      model,
      primitive,
      attributeName,
      data);
  size_t accessorIndex = model.accessors.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::MAT2;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT16;
  testClassProperty.normalized = true;

  PropertyAttribute& propertyAttribute =
      metadata.propertyAttributes.emplace_back();
  propertyAttribute.classProperty = "TestClass";

  PropertyAttributeProperty& propertyAttributeProperty =
      propertyAttribute.properties["TestClassProperty"];
  propertyAttributeProperty.attribute = attributeName;

  PropertyAttributeView view(model, propertyAttribute);
  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::MAT2);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT16);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(classProperty->normalized);

  SUBCASE("Access correct type") {
    PropertyAttributePropertyView<glm::u16mat2x2, true> u16mat2x2Property =
        view.getPropertyView<glm::u16mat2x2, true>(
            primitive,
            "TestClassProperty");
    REQUIRE(
        u16mat2x2Property.status() ==
        PropertyAttributePropertyViewStatus::Valid);
    for (size_t i = 0; i < data.size(); ++i) {
      REQUIRE(u16mat2x2Property.getRaw(static_cast<int64_t>(i)) == data[i]);
      REQUIRE(
          u16mat2x2Property.get(static_cast<int64_t>(i)) == normalize(data[i]));
    }
  }

  SUBCASE("Access wrong type") {
    PropertyAttributePropertyView<uint16_t, true> uint16Invalid =
        view.getPropertyView<uint16_t, true>(primitive, "TestClassProperty");
    REQUIRE(
        uint16Invalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorTypeMismatch);

    PropertyAttributePropertyView<glm::u16vec2, true> u16vec2Invalid =
        view.getPropertyView<glm::u16vec2, true>(
            primitive,
            "TestClassProperty");
    REQUIRE(
        u16vec2Invalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorTypeMismatch);

    PropertyAttributePropertyView<glm::u16mat4x4, true> u16mat4x4Invalid =
        view.getPropertyView<glm::u16mat4x4, true>(
            primitive,
            "TestClassProperty");
    REQUIRE(
        u16mat4x4Invalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Access wrong component type") {
    PropertyAttributePropertyView<glm::imat2x2, true> imat2Invalid =
        view.getPropertyView<glm::imat2x2, true>(
            primitive,
            "TestClassProperty");
    REQUIRE(
        imat2Invalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Access incorrectly as non-normalized") {
    PropertyAttributePropertyView<glm::u16mat2x2> nonNormalizedInvalid =
        view.getPropertyView<glm::u16mat2x2>(primitive, "TestClassProperty");
    REQUIRE(
        nonNormalizedInvalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Access incorrectly as dmat2") {
    PropertyAttributePropertyView<glm::dmat2> dmat2Invalid =
        view.getPropertyView<glm::dmat2>(primitive, "TestClassProperty");
    REQUIRE(
        dmat2Invalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Wrong accessor normalization") {
    model.accessors[accessorIndex].normalized = false;
    PropertyAttributePropertyView<glm::u16mat2x2, true> property =
        view.getPropertyView<glm::u16mat2x2, true>(
            primitive,
            "TestClassProperty");
    REQUIRE(
        property.status() == PropertyAttributePropertyViewStatus::
                                 ErrorAccessorNormalizationMismatch);
  }
}

TEST_CASE("Test with PropertyAttributeProperty offset, scale, min, max") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

  std::vector<float> data{1.0f, 2.0f, 3.0f, 4.0f};

  const float offset = 1.0f;
  const float scale = 2.0f;
  const float min = 3.0f;
  const float max = 9.0f;

  const std::string attributeName = "_ATTRIBUTE";
  addAttributeToModel(model, primitive, attributeName, data);

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

  PropertyAttribute& propertyAttribute =
      metadata.propertyAttributes.emplace_back();
  propertyAttribute.classProperty = "TestClass";

  PropertyAttributeProperty& propertyAttributeProperty =
      propertyAttribute.properties["TestClassProperty"];
  propertyAttributeProperty.attribute = attributeName;

  PropertyAttributeView view(model, propertyAttribute);
  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);

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
    PropertyAttributePropertyView<float> property =
        view.getPropertyView<float>(primitive, "TestClassProperty");
    REQUIRE(property.status() == PropertyAttributePropertyViewStatus::Valid);
    REQUIRE(property.offset() == offset);
    REQUIRE(property.scale() == scale);
    REQUIRE(property.min() == min);
    REQUIRE(property.max() == max);

    std::vector<float> expected{3.0f, 5.0f, 7.0f, 9.0f};
    for (size_t i = 0; i < data.size(); ++i) {
      REQUIRE(property.getRaw(static_cast<int64_t>(i)) == data[i]);
      REQUIRE(property.get(static_cast<int64_t>(i)) == expected[i]);
    }
  }

  SUBCASE("Use own property values") {
    const float newOffset = 0.5f;
    const float newScale = -1.0f;
    const float newMin = -3.5f;
    const float newMax = -0.5f;
    propertyAttributeProperty.offset = newOffset;
    propertyAttributeProperty.scale = newScale;
    propertyAttributeProperty.min = newMin;
    propertyAttributeProperty.max = newMax;

    PropertyAttributePropertyView<float> property =
        view.getPropertyView<float>(primitive, "TestClassProperty");
    REQUIRE(property.status() == PropertyAttributePropertyViewStatus::Valid);
    REQUIRE(property.offset() == newOffset);
    REQUIRE(property.scale() == newScale);
    REQUIRE(property.min() == newMin);
    REQUIRE(property.max() == newMax);

    std::vector<float> expected{-0.5f, -1.5f, -2.5f, -3.5f};
    for (size_t i = 0; i < data.size(); ++i) {
      REQUIRE(property.getRaw(static_cast<int64_t>(i)) == data[i]);
      REQUIRE(property.get(static_cast<int64_t>(i)) == expected[i]);
    }
  }
}

TEST_CASE("Test with PropertyAttributeProperty offset, scale, min, max "
          "(normalized)") {
  Model model;

  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

  std::vector<uint8_t> data{0, 128, 255, 32};

  const double offset = 1.0;
  const double scale = 2.0;
  const double min = 1.0;
  const double max = 3.0;

  const std::string attributeName = "_ATTRIBUTE";
  addAttributeToModel<uint8_t, true>(model, primitive, attributeName, data);

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT8;
  testClassProperty.normalized = true;
  testClassProperty.offset = offset;
  testClassProperty.scale = scale;
  testClassProperty.min = min;
  testClassProperty.max = max;

  PropertyAttribute& propertyAttribute =
      metadata.propertyAttributes.emplace_back();
  propertyAttribute.classProperty = "TestClass";

  PropertyAttributeProperty& propertyAttributeProperty =
      propertyAttribute.properties["TestClassProperty"];
  propertyAttributeProperty.attribute = attributeName;

  PropertyAttributeView view(model, propertyAttribute);
  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT8);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(classProperty->normalized);

  std::vector<glm::dvec2> texCoords{
      glm::dvec2(0, 0),
      glm::dvec2(0.5, 0),
      glm::dvec2(0, 0.5),
      glm::dvec2(0.5, 0.5)};

  SUBCASE("Use class property values") {
    PropertyAttributePropertyView<uint8_t, true> property =
        view.getPropertyView<uint8_t, true>(primitive, "TestClassProperty");
    REQUIRE(property.status() == PropertyAttributePropertyViewStatus::Valid);
    REQUIRE(property.offset() == offset);
    REQUIRE(property.scale() == scale);
    REQUIRE(property.min() == min);
    REQUIRE(property.max() == max);

    for (size_t i = 0; i < data.size(); ++i) {
      REQUIRE(property.getRaw(static_cast<int64_t>(i)) == data[i]);
      REQUIRE(
          property.get(static_cast<int64_t>(i)) ==
          normalize(data[i]) * scale + offset);
    }
  }

  SUBCASE("Use own property values") {
    const double newOffset = 2.0;
    const double newScale = 5.0;
    const double newMin = 10.0;
    const double newMax = 11.0;
    propertyAttributeProperty.offset = newOffset;
    propertyAttributeProperty.scale = newScale;
    propertyAttributeProperty.min = newMin;
    propertyAttributeProperty.max = newMax;

    PropertyAttributePropertyView<uint8_t, true> property =
        view.getPropertyView<uint8_t, true>(primitive, "TestClassProperty");
    REQUIRE(property.status() == PropertyAttributePropertyViewStatus::Valid);
    REQUIRE(property.offset() == newOffset);
    REQUIRE(property.scale() == newScale);
    REQUIRE(property.min() == newMin);
    REQUIRE(property.max() == newMax);

    for (size_t i = 0; i < data.size(); ++i) {
      REQUIRE(property.getRaw(static_cast<int64_t>(i)) == data[i]);
      REQUIRE(
          property.get(static_cast<int64_t>(i)) ==
          normalize(data[i]) * newScale + newOffset);
    }
  }
}

TEST_CASE("Test with PropertyAttributeProperty noData") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

  std::vector<uint8_t> data = {12, 34, 30, 11};
  const uint8_t noData = 34;

  const std::string attributeName = "_ATTRIBUTE";
  addAttributeToModel(model, primitive, attributeName, data);

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT8;
  testClassProperty.noData = noData;

  PropertyAttribute& propertyAttribute =
      metadata.propertyAttributes.emplace_back();
  propertyAttribute.classProperty = "TestClass";

  PropertyAttributeProperty& propertyAttributeProperty =
      propertyAttribute.properties["TestClassProperty"];
  propertyAttributeProperty.attribute = attributeName;

  PropertyAttributeView view(model, propertyAttribute);
  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT8);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(!classProperty->normalized);

  SUBCASE("Without default value") {
    PropertyAttributePropertyView<uint8_t> property =
        view.getPropertyView<uint8_t>(primitive, "TestClassProperty");
    REQUIRE(property.status() == PropertyAttributePropertyViewStatus::Valid);

    for (size_t i = 0; i < data.size(); ++i) {
      auto value = property.getRaw(static_cast<int64_t>(i));
      REQUIRE(value == data[i]);

      auto maybeValue = property.get(static_cast<int64_t>(i));
      if (value == noData) {
        REQUIRE(!maybeValue);
      } else {
        REQUIRE(maybeValue == data[i]);
      }
    }
  }

  SUBCASE("With default value") {
    const uint8_t defaultValue = 255;
    testClassProperty.defaultProperty = defaultValue;

    PropertyAttributePropertyView<uint8_t> property =
        view.getPropertyView<uint8_t>(primitive, "TestClassProperty");
    REQUIRE(property.status() == PropertyAttributePropertyViewStatus::Valid);

    for (size_t i = 0; i < data.size(); ++i) {
      auto value = property.getRaw(static_cast<int64_t>(i));
      REQUIRE(value == data[i]);

      auto maybeValue = property.get(static_cast<int64_t>(i));
      if (value == noData) {
        REQUIRE(maybeValue == defaultValue);
      } else {
        REQUIRE(maybeValue == data[i]);
      }
    }
  }
}

TEST_CASE("Test with PropertyAttributeProperty noData (normalized)") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

  std::vector<uint8_t> data = {12, 34, 30, 11};
  const uint8_t noData = 34;

  const std::string attributeName = "_ATTRIBUTE";
  addAttributeToModel<uint8_t, true>(model, primitive, attributeName, data);

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT8;
  testClassProperty.normalized = true;
  testClassProperty.noData = noData;

  PropertyAttribute& propertyAttribute =
      metadata.propertyAttributes.emplace_back();
  propertyAttribute.classProperty = "TestClass";

  PropertyAttributeProperty& propertyAttributeProperty =
      propertyAttribute.properties["TestClassProperty"];
  propertyAttributeProperty.attribute = attributeName;

  PropertyAttributeView view(model, propertyAttribute);
  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT8);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(classProperty->normalized);

  SUBCASE("Without default value") {
    PropertyAttributePropertyView<uint8_t, true> property =
        view.getPropertyView<uint8_t, true>(primitive, "TestClassProperty");
    REQUIRE(property.status() == PropertyAttributePropertyViewStatus::Valid);

    for (size_t i = 0; i < data.size(); ++i) {
      auto value = property.getRaw(static_cast<int64_t>(i));
      REQUIRE(value == data[i]);

      auto maybeValue = property.get(static_cast<int64_t>(i));
      if (value == noData) {
        REQUIRE(!maybeValue);
      } else {
        REQUIRE(maybeValue == normalize(data[i]));
      }
    }
  }

  SUBCASE("With default value") {
    const double defaultValue = -1.0;
    testClassProperty.defaultProperty = defaultValue;

    PropertyAttributePropertyView<uint8_t, true> property =
        view.getPropertyView<uint8_t, true>(primitive, "TestClassProperty");
    REQUIRE(property.status() == PropertyAttributePropertyViewStatus::Valid);

    for (size_t i = 0; i < data.size(); ++i) {
      auto value = property.getRaw(static_cast<int64_t>(i));
      REQUIRE(value == data[i]);

      auto maybeValue = property.get(static_cast<int64_t>(i));
      if (value == noData) {
        REQUIRE(maybeValue == defaultValue);
      } else {
        REQUIRE(maybeValue == normalize(data[i]));
      }
    }
  }
}

TEST_CASE(
    "Test nonexistent PropertyAttributeProperty with class property default") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

  // Add a position attribute so it can be used to substitute the property
  // accessor size.
  const std::string attributeName = "POSITION";
  std::vector<glm::vec3> data = {
      glm::vec3(0),
      glm::vec3(1, 2, 3),
      glm::vec3(0, 1, 0)};

  addAttributeToModel(model, primitive, attributeName, data);

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT16;

  const uint16_t defaultValue = 10;
  testClassProperty.defaultProperty = defaultValue;

  PropertyAttribute& propertyAttribute =
      metadata.propertyAttributes.emplace_back();
  propertyAttribute.classProperty = "TestClass";

  PropertyAttributeView view(model, propertyAttribute);
  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT16);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(!classProperty->normalized);
  REQUIRE(classProperty->defaultProperty);

  SUBCASE("Access correct type") {
    PropertyAttributePropertyView<uint16_t> uint16Property =
        view.getPropertyView<uint16_t>(primitive, "TestClassProperty");
    REQUIRE(
        uint16Property.status() ==
        PropertyAttributePropertyViewStatus::EmptyPropertyWithDefault);
    REQUIRE(uint16Property.size() == static_cast<int64_t>(data.size()));
    REQUIRE(uint16Property.defaultValue() == defaultValue);

    for (size_t i = 0; i < data.size(); ++i) {
      REQUIRE(uint16Property.get(static_cast<int64_t>(i)) == defaultValue);
    }
  }

  SUBCASE("Access wrong type") {
    PropertyAttributePropertyView<glm::u16vec2> u16vec2Invalid =
        view.getPropertyView<glm::u16vec2>(primitive, "TestClassProperty");
    REQUIRE(
        u16vec2Invalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Access wrong component type") {
    PropertyAttributePropertyView<uint8_t> uint8Invalid =
        view.getPropertyView<uint8_t>(primitive, "TestClassProperty");
    REQUIRE(
        uint8Invalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Access incorrectly as normalized") {
    PropertyAttributePropertyView<uint16_t, true> normalizedInvalid =
        view.getPropertyView<uint16_t, true>(primitive, "TestClassProperty");
    REQUIRE(
        normalizedInvalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Invalid default value") {
    testClassProperty.defaultProperty = "not a number";
    PropertyAttributePropertyView<uint16_t> uint16Property =
        view.getPropertyView<uint16_t>(primitive, "TestClassProperty");
    REQUIRE(
        uint16Property.status() ==
        PropertyAttributePropertyViewStatus::ErrorInvalidDefaultValue);
  }

  SUBCASE("No default value") {
    testClassProperty.defaultProperty.reset();
    PropertyAttributePropertyView<uint16_t> uint16Property =
        view.getPropertyView<uint16_t>(primitive, "TestClassProperty");
    REQUIRE(
        uint16Property.status() ==
        PropertyAttributePropertyViewStatus::ErrorNonexistentProperty);
  }
}

TEST_CASE("Test callback on invalid property Attribute view") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();
  metadata.schema.emplace();

  // Property Attribute has a nonexistent class.
  PropertyAttribute& propertyAttribute =
      metadata.propertyAttributes.emplace_back();
  propertyAttribute.classProperty = "TestClass";

  PropertyAttributeProperty& propertyAttributeProperty =
      propertyAttribute.properties["TestClassProperty"];
  propertyAttributeProperty.attribute = "_INVALID";

  PropertyAttributeView view(model, propertyAttribute);
  REQUIRE(view.status() == PropertyAttributeViewStatus::ErrorClassNotFound);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(!classProperty);

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      primitive,
      "TestClassProperty",
      [&invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) mutable {
        invokedCallbackCount++;
        REQUIRE(
            propertyValue.status() ==
            PropertyAttributePropertyViewStatus::ErrorInvalidPropertyAttribute);
      });

  REQUIRE(invokedCallbackCount == 1);
}

TEST_CASE("Test callback on invalid PropertyAttributeProperty") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["InvalidProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT8;

  PropertyAttribute& propertyAttribute =
      metadata.propertyAttributes.emplace_back();
  propertyAttribute.classProperty = "TestClass";

  PropertyAttributeProperty& propertyAttributeProperty =
      propertyAttribute.properties["InvalidProperty"];
  propertyAttributeProperty.attribute = "_INVALID";

  PropertyAttributeView view(model, propertyAttribute);
  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);

  const ClassProperty* classProperty = view.getClassProperty("InvalidProperty");
  REQUIRE(classProperty);

  classProperty = view.getClassProperty("NonexistentProperty");
  REQUIRE(!classProperty);

  uint32_t invokedCallbackCount = 0;
  auto testCallback = [&invokedCallbackCount](
                          const std::string& /*propertyId*/,
                          auto propertyValue) mutable {
    invokedCallbackCount++;
    REQUIRE(
        propertyValue.status() != PropertyAttributePropertyViewStatus::Valid);
  };

  view.getPropertyView(primitive, "InvalidProperty", testCallback);
  view.getPropertyView(primitive, "NonexistentProperty", testCallback);

  REQUIRE(invokedCallbackCount == 2);
}

TEST_CASE("Test callback on invalid normalized PropertyAttributeProperty") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::FLOAT32;
  testClassProperty.normalized = true; // This is erroneous.

  PropertyAttribute& propertyAttribute =
      metadata.propertyAttributes.emplace_back();
  propertyAttribute.classProperty = "TestClass";

  PropertyAttributeProperty& propertyAttributeProperty =
      propertyAttribute.properties["TestClassProperty"];
  propertyAttributeProperty.attribute = "_INVALID";

  PropertyAttributeView view(model, propertyAttribute);
  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);

  uint32_t invokedCallbackCount = 0;
  auto testCallback = [&invokedCallbackCount](
                          const std::string& /*propertyId*/,
                          auto propertyValue) mutable {
    invokedCallbackCount++;
    REQUIRE(
        propertyValue.status() ==
        PropertyAttributePropertyViewStatus::ErrorInvalidNormalization);
  };

  view.getPropertyView(primitive, "TestClassProperty", testCallback);

  REQUIRE(invokedCallbackCount == 1);
}

TEST_CASE("Test callback for scalar PropertyAttributeProperty") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

  std::vector<int16_t> data{-1, 268, 542, -256};

  const std::string attributeName = "_ATTRIBUTE";
  addAttributeToModel(model, primitive, attributeName, data);

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::INT16;

  PropertyAttribute& propertyAttribute =
      metadata.propertyAttributes.emplace_back();
  propertyAttribute.classProperty = "TestClass";

  PropertyAttributeProperty& propertyAttributeProperty =
      propertyAttribute.properties["TestClassProperty"];
  propertyAttributeProperty.attribute = attributeName;

  PropertyAttributeView view(model, propertyAttribute);
  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT16);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(!classProperty->normalized);

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      primitive,
      "TestClassProperty",
      [&data, &invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) mutable {
        invokedCallbackCount++;
        if constexpr (std::is_same_v<
                          PropertyAttributePropertyView<int16_t>,
                          decltype(propertyValue)>) {
          REQUIRE(
              propertyValue.status() ==
              PropertyAttributePropertyViewStatus::Valid);

          for (size_t i = 0; i < data.size(); ++i) {
            REQUIRE(propertyValue.getRaw(static_cast<int64_t>(i)) == data[i]);
            REQUIRE(propertyValue.get(static_cast<int64_t>(i)) == data[i]);
          }
        } else {
          FAIL("getPropertyView returned PropertyAttributePropertyView of "
               "incorrect type for TestClassProperty.");
        }
      });

  REQUIRE(invokedCallbackCount == 1);
}

TEST_CASE("Test callback for scalar PropertyAttributeProperty (normalized)") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

  std::vector<int16_t> data{-1, 268, 542, -256};

  const std::string attributeName = "_ATTRIBUTE";
  addAttributeToModel<int16_t, true>(model, primitive, attributeName, data);

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::INT16;
  testClassProperty.normalized = true;

  PropertyAttribute& propertyAttribute =
      metadata.propertyAttributes.emplace_back();
  propertyAttribute.classProperty = "TestClass";

  PropertyAttributeProperty& propertyAttributeProperty =
      propertyAttribute.properties["TestClassProperty"];
  propertyAttributeProperty.attribute = attributeName;

  PropertyAttributeView view(model, propertyAttribute);
  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT16);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(classProperty->normalized);

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      primitive,
      "TestClassProperty",
      [&data, &invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) mutable {
        invokedCallbackCount++;
        if constexpr (std::is_same_v<
                          PropertyAttributePropertyView<int16_t, true>,
                          decltype(propertyValue)>) {
          REQUIRE(
              propertyValue.status() ==
              PropertyAttributePropertyViewStatus::Valid);

          for (size_t i = 0; i < data.size(); ++i) {
            REQUIRE(propertyValue.getRaw(static_cast<int64_t>(i)) == data[i]);
            REQUIRE(
                propertyValue.get(static_cast<int64_t>(i)) ==
                normalize(data[i]));
          }
        } else {
          FAIL("getPropertyView returned PropertyAttributePropertyView of "
               "incorrect type for TestClassProperty.");
        }
      });

  REQUIRE(invokedCallbackCount == 1);
}

TEST_CASE("Test callback for vecN PropertyAttributeProperty") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

  std::vector<glm::i8vec2> data{
      glm::i8vec2(-1, -1),
      glm::i8vec2(12, 1),
      glm::i8vec2(30, 2),
      glm::i8vec2(0, -1)};

  const std::string attributeName = "_ATTRIBUTE";
  addAttributeToModel(model, primitive, attributeName, data);

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::VEC2;
  testClassProperty.componentType = ClassProperty::ComponentType::INT8;

  PropertyAttribute& propertyAttribute =
      metadata.propertyAttributes.emplace_back();
  propertyAttribute.classProperty = "TestClass";

  PropertyAttributeProperty& propertyAttributeProperty =
      propertyAttribute.properties["TestClassProperty"];
  propertyAttributeProperty.attribute = attributeName;

  PropertyAttributeView view(model, propertyAttribute);
  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::VEC2);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT8);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(!classProperty->normalized);

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      primitive,
      "TestClassProperty",
      [&data, &invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) mutable {
        invokedCallbackCount++;
        if constexpr (std::is_same_v<
                          PropertyAttributePropertyView<glm::i8vec2>,
                          decltype(propertyValue)>) {
          REQUIRE(
              propertyValue.status() ==
              PropertyAttributePropertyViewStatus::Valid);

          for (size_t i = 0; i < data.size(); ++i) {
            REQUIRE(propertyValue.getRaw(static_cast<int64_t>(i)) == data[i]);
            REQUIRE(propertyValue.get(static_cast<int64_t>(i)) == data[i]);
          }
        } else {
          FAIL("getPropertyView returned PropertyAttributePropertyView of "
               "incorrect type for TestClassProperty.");
        }
      });

  REQUIRE(invokedCallbackCount == 1);
}

TEST_CASE("Test callback for vecN PropertyAttributeProperty (normalized)") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

  std::vector<glm::i8vec2> data{
      glm::i8vec2(-1, -1),
      glm::i8vec2(12, 1),
      glm::i8vec2(30, 2),
      glm::i8vec2(0, -1)};

  const std::string attributeName = "_ATTRIBUTE";
  addAttributeToModel<glm::i8vec2, true>(model, primitive, attributeName, data);

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::VEC2;
  testClassProperty.componentType = ClassProperty::ComponentType::INT8;
  testClassProperty.normalized = true;

  PropertyAttribute& propertyAttribute =
      metadata.propertyAttributes.emplace_back();
  propertyAttribute.classProperty = "TestClass";

  PropertyAttributeProperty& propertyAttributeProperty =
      propertyAttribute.properties["TestClassProperty"];
  propertyAttributeProperty.attribute = attributeName;

  PropertyAttributeView view(model, propertyAttribute);
  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::VEC2);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT8);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(classProperty->normalized);

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      primitive,
      "TestClassProperty",
      [&data, &invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) mutable {
        invokedCallbackCount++;
        if constexpr (std::is_same_v<
                          PropertyAttributePropertyView<glm::i8vec2, true>,
                          decltype(propertyValue)>) {
          REQUIRE(
              propertyValue.status() ==
              PropertyAttributePropertyViewStatus::Valid);

          for (size_t i = 0; i < data.size(); ++i) {
            REQUIRE(propertyValue.getRaw(static_cast<int64_t>(i)) == data[i]);
            REQUIRE(
                propertyValue.get(static_cast<int64_t>(i)) ==
                normalize(data[i]));
          }
        } else {
          FAIL("getPropertyView returned PropertyAttributePropertyView of "
               "incorrect type for TestClassProperty.");
        }
      });

  REQUIRE(invokedCallbackCount == 1);
}

TEST_CASE("Test callback for matN PropertyAttributeProperty") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

  // clang-format off
  std::vector<glm::u16mat2x2> data = {
      glm::u16mat2x2(
        12, 34,
        30, 1),
      glm::u16mat2x2(
        11, 8,
        73, 102),
      glm::u16mat2x2(
        1, 0,
        63, 2),
      glm::u16mat2x2(
        4, 8,
        3, 23)};
  // clang-format on

  const std::string attributeName = "_ATTRIBUTE";
  addAttributeToModel(model, primitive, attributeName, data);

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::MAT2;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT16;

  PropertyAttribute& propertyAttribute =
      metadata.propertyAttributes.emplace_back();
  propertyAttribute.classProperty = "TestClass";

  PropertyAttributeProperty& propertyAttributeProperty =
      propertyAttribute.properties["TestClassProperty"];
  propertyAttributeProperty.attribute = attributeName;

  PropertyAttributeView view(model, propertyAttribute);
  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::MAT2);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT16);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(!classProperty->normalized);

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      primitive,
      "TestClassProperty",
      [&data, &invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) mutable {
        invokedCallbackCount++;
        if constexpr (std::is_same_v<
                          PropertyAttributePropertyView<glm::u16mat2x2>,
                          decltype(propertyValue)>) {
          REQUIRE(
              propertyValue.status() ==
              PropertyAttributePropertyViewStatus::Valid);

          for (size_t i = 0; i < data.size(); ++i) {
            REQUIRE(propertyValue.getRaw(static_cast<int64_t>(i)) == data[i]);
            REQUIRE(propertyValue.get(static_cast<int64_t>(i)) == data[i]);
          }
        } else {
          FAIL("getPropertyView returned PropertyAttributePropertyView of "
               "incorrect type for TestClassProperty.");
        }
      });

  REQUIRE(invokedCallbackCount == 1);
}

TEST_CASE("Test callback for matN PropertyAttributeProperty (normalized)") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

  // clang-format off
  std::vector<glm::u16mat2x2> data = {
      glm::u16mat2x2(
        12, 34,
        30, 1),
      glm::u16mat2x2(
        11, 8,
        73, 102),
      glm::u16mat2x2(
        1, 0,
        63, 2),
      glm::u16mat2x2(
        4, 8,
        3, 23)};
  // clang-format on

  const std::string attributeName = "_ATTRIBUTE";
  addAttributeToModel<glm::u16mat2x2, true>(
      model,
      primitive,
      attributeName,
      data);

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::MAT2;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT16;
  testClassProperty.normalized = true;

  PropertyAttribute& propertyAttribute =
      metadata.propertyAttributes.emplace_back();
  propertyAttribute.classProperty = "TestClass";

  PropertyAttributeProperty& propertyAttributeProperty =
      propertyAttribute.properties["TestClassProperty"];
  propertyAttributeProperty.attribute = attributeName;

  PropertyAttributeView view(model, propertyAttribute);
  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::MAT2);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT16);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(classProperty->normalized);

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      primitive,
      "TestClassProperty",
      [&data, &invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) mutable {
        invokedCallbackCount++;
        if constexpr (std::is_same_v<
                          PropertyAttributePropertyView<glm::u16mat2x2, true>,
                          decltype(propertyValue)>) {
          REQUIRE(
              propertyValue.status() ==
              PropertyAttributePropertyViewStatus::Valid);

          for (size_t i = 0; i < data.size(); ++i) {
            REQUIRE(propertyValue.getRaw(static_cast<int64_t>(i)) == data[i]);
            REQUIRE(
                propertyValue.get(static_cast<int64_t>(i)) ==
                normalize(data[i]));
          }
        } else {
          FAIL("getPropertyView returned PropertyAttributePropertyView of "
               "incorrect type for TestClassProperty.");
        }
      });

  REQUIRE(invokedCallbackCount == 1);
}

TEST_CASE("Test callback on unsupported PropertyAttributeProperty") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];

  ClassProperty& doubleClassProperty =
      testClass.properties["DoubleClassProperty"];
  doubleClassProperty.type = ClassProperty::Type::SCALAR;
  doubleClassProperty.componentType = ClassProperty::ComponentType::FLOAT64;

  ClassProperty& arrayClassProperty =
      testClass.properties["ArrayClassProperty"];
  arrayClassProperty.type = ClassProperty::Type::SCALAR;
  arrayClassProperty.componentType = ClassProperty::ComponentType::UINT8;
  arrayClassProperty.array = true;
  arrayClassProperty.count = 2;

  PropertyAttribute& propertyAttribute =
      metadata.propertyAttributes.emplace_back();
  propertyAttribute.classProperty = "TestClass";

  PropertyAttributeProperty& doubleProperty =
      propertyAttribute.properties["DoubleClassProperty"];
  doubleProperty.attribute = "_DOUBLE";
  PropertyAttributeProperty& arrayProperty =
      propertyAttribute.properties["ArrayClassProperty"];
  arrayProperty.attribute = "_ARRAY";

  PropertyAttributeView view(model, propertyAttribute);
  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("DoubleClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(
      classProperty->componentType == ClassProperty::ComponentType::FLOAT64);
  REQUIRE(!classProperty->array);

  classProperty = view.getClassProperty("ArrayClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT8);
  REQUIRE(classProperty->array);
  REQUIRE(classProperty->count == 2);

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      primitive,
      "DoubleClassProperty",
      [&invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) mutable {
        invokedCallbackCount++;
        REQUIRE(
            propertyValue.status() ==
            PropertyAttributePropertyViewStatus::ErrorUnsupportedProperty);
      });
  REQUIRE(invokedCallbackCount == 1);

  view.getPropertyView(
      primitive,
      "ArrayClassProperty",
      [&invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) mutable {
        invokedCallbackCount++;
        REQUIRE(
            propertyValue.status() ==
            PropertyAttributePropertyViewStatus::ErrorUnsupportedProperty);
      });
  REQUIRE(invokedCallbackCount == 2);
}

TEST_CASE(
    "Test callback for empty PropertyAttributeProperty with default value") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

  // Add a position attribute so it can be used to substitute the property
  // accessor size.
  const std::string attributeName = "POSITION";
  std::vector<glm::vec3> data = {
      glm::vec3(0),
      glm::vec3(1, 2, 3),
      glm::vec3(0, 1, 0)};
  addAttributeToModel(model, primitive, attributeName, data);

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::INT16;

  const int16_t defaultValue = 10;
  testClassProperty.defaultProperty = defaultValue;

  PropertyAttribute& propertyAttribute =
      metadata.propertyAttributes.emplace_back();
  propertyAttribute.classProperty = "TestClass";

  PropertyAttributeView view(model, propertyAttribute);
  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT16);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(!classProperty->normalized);
  REQUIRE(classProperty->defaultProperty);

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      primitive,
      "TestClassProperty",
      [defaultValue, &data, &invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) mutable {
        invokedCallbackCount++;
        if constexpr (std::is_same_v<
                          PropertyAttributePropertyView<int16_t>,
                          decltype(propertyValue)>) {
          REQUIRE(
              propertyValue.status() ==
              PropertyAttributePropertyViewStatus::EmptyPropertyWithDefault);
          REQUIRE(propertyValue.size() == static_cast<int64_t>(data.size()));
          REQUIRE(propertyValue.defaultValue() == defaultValue);
          for (size_t i = 0; i < data.size(); ++i) {
            REQUIRE(propertyValue.get(static_cast<int64_t>(i)) == defaultValue);
          }
        } else {
          FAIL("getPropertyView returned PropertyAttributePropertyView of "
               "incorrect type for TestClassProperty.");
        }
      });

  REQUIRE(invokedCallbackCount == 1);
}
