#include "CesiumGltf/PropertyAttributeView.h"

#include <catch2/catch.hpp>
#include <gsl/span>

#include <cstddef>
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
  accessor.count = values.size();
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
    assert(false && "Input type is not supported as an accessor type");
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
  case PropertyComponentType::Uint32:
    accessor.componentType = Accessor::ComponentType::UNSIGNED_INT;
    break;
  case PropertyComponentType::Float32:
    accessor.componentType = Accessor::ComponentType::FLOAT;
    break;
  default:
    assert(
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

  SECTION("Access correct type") {
    PropertyAttributePropertyView<uint16_t> uint16Property =
        view.getPropertyView<uint16_t>("TestClassProperty", primitive);
    REQUIRE(
        uint16Property.status() == PropertyAttributePropertyViewStatus::Valid);
    for (size_t i = 0; i < data.size(); ++i) {
      REQUIRE(uint16Property.getRaw(static_cast<int64_t>(i)) == data[i]);
      REQUIRE(uint16Property.get(static_cast<int64_t>(i)) == data[i]);
    }
  }

  SECTION("Access wrong type") {
    PropertyAttributePropertyView<glm::u16vec2> u16vec2Invalid =
        view.getPropertyView<glm::u16vec2>("TestClassProperty", primitive);
    REQUIRE(
        u16vec2Invalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorTypeMismatch);
  }

  SECTION("Access wrong component type") {
    PropertyAttributePropertyView<uint8_t> uint8Invalid =
        view.getPropertyView<uint8_t>("TestClassProperty", primitive);
    REQUIRE(
        uint8Invalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorComponentTypeMismatch);

    PropertyAttributePropertyView<int32_t> int32Invalid =
        view.getPropertyView<int32_t>("TestClassProperty", primitive);
    REQUIRE(
        int32Invalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorComponentTypeMismatch);

    PropertyAttributePropertyView<float> uint64Invalid =
        view.getPropertyView<float>("TestClassProperty", primitive);
    REQUIRE(
        uint64Invalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SECTION("Access incorrectly as normalized") {
    PropertyAttributePropertyView<uint16_t, true> normalizedInvalid =
        view.getPropertyView<uint16_t, true>("TestClassProperty", primitive);
    REQUIRE(
        normalizedInvalid.status() ==
        PropertyAttributePropertyViewStatus::ErrorNormalizationMismatch);
  }

  SECTION("Buffer view points outside of the real buffer length") {
    model.buffers[bufferIndex].cesium.data.resize(4);
    PropertyAttributePropertyView<uint16_t> property =
        view.getPropertyView<uint16_t>("TestClassProperty", primitive);
    REQUIRE(
        property.status() ==
        PropertyAttributePropertyViewStatus::ErrorBufferViewOutOfBounds);
  }

  SECTION("Wrong buffer index") {
    model.bufferViews[bufferViewIndex].buffer = 2;
    PropertyAttributePropertyView<uint16_t> property =
        view.getPropertyView<uint16_t>("TestClassProperty", primitive);
    REQUIRE(
        property.status() ==
        PropertyAttributePropertyViewStatus::ErrorInvalidBuffer);
  }

  SECTION("Wrong buffer view index") {
    model.accessors[accessorIndex].bufferView = -1;
    PropertyAttributePropertyView<uint16_t> property =
        view.getPropertyView<uint16_t>("TestClassProperty", primitive);
    REQUIRE(
        property.status() ==
        PropertyAttributePropertyViewStatus::ErrorInvalidBufferView);
  }

  SECTION("Wrong accessor normalization") {
    model.accessors[accessorIndex].normalized = true;
    PropertyAttributePropertyView<uint16_t> property =
        view.getPropertyView<uint16_t>("TestClassProperty", primitive);
    REQUIRE(
        property.status() == PropertyAttributePropertyViewStatus::
                                 ErrorAccessorNormalizationMismatch);
  }

  SECTION("Wrong accessor component type") {
    model.accessors[accessorIndex].componentType =
        Accessor::ComponentType::SHORT;
    PropertyAttributePropertyView<uint16_t> property =
        view.getPropertyView<uint16_t>("TestClassProperty", primitive);
    REQUIRE(
        property.status() == PropertyAttributePropertyViewStatus::
                                 ErrorAccessorComponentTypeMismatch);
  }

  SECTION("Wrong accessor type") {
    model.accessors[accessorIndex].type = Accessor::Type::VEC2;
    PropertyAttributePropertyView<uint16_t> property =
        view.getPropertyView<uint16_t>("TestClassProperty", primitive);
    REQUIRE(
        property.status() ==
        PropertyAttributePropertyViewStatus::ErrorAccessorTypeMismatch);
  }

  SECTION("Wrong accessor index") {
    primitive.attributes[attributeName] = -1;
    PropertyAttributePropertyView<uint16_t> property =
        view.getPropertyView<uint16_t>("TestClassProperty", primitive);
    REQUIRE(
        property.status() ==
        PropertyAttributePropertyViewStatus::ErrorInvalidAccessor);
  }

  SECTION("Missing attribute") {
    primitive.attributes.clear();
    PropertyAttributePropertyView<uint16_t> property =
        view.getPropertyView<uint16_t>("TestClassProperty", primitive);
    REQUIRE(
        property.status() ==
        PropertyAttributePropertyViewStatus::ErrorMissingAttribute);
  }
}

// TEST_CASE("Test scalar PropertyAttributeProperty (normalized)") {
//  Model model;
//  std::vector<uint8_t> data = {12, 34, 30, 11};
//
//  addAttributeToModel(
//      model,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      2,
//      2,
//      1,
//      data);
//  size_t AttributeIndex = model.Attributes.size() - 1;
//  size_t imageIndex = model.images.size() - 1;
//
//  ExtensionModelExtStructuralMetadata& metadata =
//      model.addExtension<ExtensionModelExtStructuralMetadata>();
//
//  Schema& schema = metadata.schema.emplace();
//  Class& testClass = schema.classes["TestClass"];
//  ClassProperty& testClassProperty =
//  testClass.properties["TestClassProperty"]; testClassProperty.type =
//  ClassProperty::Type::SCALAR; testClassProperty.componentType =
//  ClassProperty::ComponentType::UINT8; testClassProperty.normalized = true;
//
//  PropertyAttribute& propertyAttribute =
//  metadata.propertyAttributes.emplace_back(); propertyAttribute.classProperty
//  = "TestClass";
//
//  PropertyAttributeProperty& propertyAttributeProperty =
//      propertyAttribute.properties["TestClassProperty"];
//  propertyAttributeProperty.index = static_cast<int32_t>(AttributeIndex);
//  propertyAttributeProperty.texCoord = 0;
//  propertyAttributeProperty.channels = {0};
//
//  PropertyAttributeView view(model, propertyAttribute);
//  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);
//
//  const ClassProperty* classProperty =
//      view.getClassProperty("TestClassProperty");
//  REQUIRE(classProperty);
//  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
//  REQUIRE(classProperty->componentType ==
//  ClassProperty::ComponentType::UINT8); REQUIRE(classProperty->count ==
//  std::nullopt); REQUIRE(!classProperty->array);
//  REQUIRE(classProperty->normalized);
//
//  SECTION("Access correct type") {
//    PropertyAttributePropertyView<uint8_t, true> uint8Property =
//        view.getPropertyView<uint8_t, true>("TestClassProperty");
//    REQUIRE(uint8Property.status() ==
//    PropertyAttributePropertyViewStatus::Valid);
//
//    std::vector<glm::dvec2> texCoords{
//        glm::dvec2(0, 0),
//        glm::dvec2(0.5, 0),
//        glm::dvec2(0, 0.5),
//        glm::dvec2(0.5, 0.5)};
//
//    for (size_t i = 0; i < texCoords.size(); ++i) {
//      glm::dvec2 uv = texCoords[i];
//      REQUIRE(uint8Property.getRaw(uv[0], uv[1]) == data[i]);
//      REQUIRE(uint8Property.get(uv[0], uv[1]) == normalize(data[i]));
//    }
//  }
//
//  SECTION("Access wrong type") {
//    PropertyAttributePropertyView<glm::u8vec2, true> u8vec2Invalid =
//        view.getPropertyView<glm::u8vec2, true>("TestClassProperty");
//    REQUIRE(
//        u8vec2Invalid.status() ==
//        PropertyAttributePropertyViewStatus::ErrorTypeMismatch);
//  }
//
//  SECTION("Access wrong component type") {
//    PropertyAttributePropertyView<uint16_t, true> uint16Invalid =
//        view.getPropertyView<uint16_t, true>("TestClassProperty");
//    REQUIRE(
//        uint16Invalid.status() ==
//        PropertyAttributePropertyViewStatus::ErrorComponentTypeMismatch);
//
//    PropertyAttributePropertyView<int32_t> int32Invalid =
//        view.getPropertyView<int32_t>("TestClassProperty");
//    REQUIRE(
//        int32Invalid.status() ==
//        PropertyAttributePropertyViewStatus::ErrorComponentTypeMismatch);
//  }
//
//  SECTION("Access incorrectly as array") {
//    PropertyAttributePropertyView<PropertyArrayView<uint8_t>, true>
//    arrayInvalid
//    =
//        view.getPropertyView<PropertyArrayView<uint8_t>, true>(
//            "TestClassProperty");
//    REQUIRE(
//        arrayInvalid.status() ==
//        PropertyAttributePropertyViewStatus::ErrorArrayTypeMismatch);
//  }
//
//  SECTION("Access incorrectly as non-normalized") {
//    PropertyAttributePropertyView<uint8_t> normalizedInvalid =
//        view.getPropertyView<uint8_t>("TestClassProperty");
//    REQUIRE(
//        normalizedInvalid.status() ==
//        PropertyAttributePropertyViewStatus::ErrorNormalizationMismatch);
//  }
//
//  SECTION("Access incorrectly as double") {
//    PropertyAttributePropertyView<double> doubleInvalid =
//        view.getPropertyView<double>("TestClassProperty");
//    REQUIRE(
//        doubleInvalid.status() ==
//        PropertyAttributePropertyViewStatus::ErrorComponentTypeMismatch);
//  }
//
//  SECTION("Channel and type mismatch") {
//    model.images[imageIndex].cesium.channels = 2;
//    propertyAttributeProperty.channels = {0, 1};
//    PropertyAttributePropertyView<uint8_t, true> uint8Property =
//        view.getPropertyView<uint8_t, true>("TestClassProperty");
//    REQUIRE(
//        uint8Property.status() ==
//        PropertyAttributePropertyViewStatus::ErrorChannelsAndTypeMismatch);
//  }
//}
//
// TEST_CASE("Test vecN PropertyAttributeProperty") {
//  Model model;
//  std::vector<uint8_t> data = {12, 34, 10, 3, 40, 0, 30, 11};
//
//  addAttributeToModel(
//      model,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      2,
//      2,
//      2,
//      data);
//  size_t AttributeIndex = model.Attributes.size() - 1;
//  size_t imageIndex = model.images.size() - 1;
//
//  ExtensionModelExtStructuralMetadata& metadata =
//      model.addExtension<ExtensionModelExtStructuralMetadata>();
//
//  Schema& schema = metadata.schema.emplace();
//  Class& testClass = schema.classes["TestClass"];
//  ClassProperty& testClassProperty =
//  testClass.properties["TestClassProperty"]; testClassProperty.type =
//  ClassProperty::Type::VEC2; testClassProperty.componentType =
//  ClassProperty::ComponentType::UINT8;
//
//  PropertyAttribute& propertyAttribute =
//  metadata.propertyAttributes.emplace_back(); propertyAttribute.classProperty
//  = "TestClass";
//
//  PropertyAttributeProperty& propertyAttributeProperty =
//      propertyAttribute.properties["TestClassProperty"];
//  propertyAttributeProperty.index = static_cast<int32_t>(AttributeIndex);
//  propertyAttributeProperty.texCoord = 0;
//  propertyAttributeProperty.channels = {0, 1};
//
//  PropertyAttributeView view(model, propertyAttribute);
//  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);
//
//  const ClassProperty* classProperty =
//      view.getClassProperty("TestClassProperty");
//  REQUIRE(classProperty);
//  REQUIRE(classProperty->type == ClassProperty::Type::VEC2);
//  REQUIRE(classProperty->componentType ==
//  ClassProperty::ComponentType::UINT8); REQUIRE(classProperty->count ==
//  std::nullopt); REQUIRE(!classProperty->array);
//  REQUIRE(!classProperty->normalized);
//
//  SECTION("Access correct type") {
//    PropertyAttributePropertyView<glm::u8vec2> u8vec2Property =
//        view.getPropertyView<glm::u8vec2>("TestClassProperty");
//    REQUIRE(
//        u8vec2Property.status() ==
//        PropertyAttributePropertyViewStatus::Valid);
//
//    std::vector<glm::dvec2> texCoords{
//        glm::dvec2(0, 0),
//        glm::dvec2(0.5, 0),
//        glm::dvec2(0, 0.5),
//        glm::dvec2(0.5, 0.5)};
//
//    std::vector<glm::u8vec2> expected{
//        glm::u8vec2(12, 34),
//        glm::u8vec2(10, 3),
//        glm::u8vec2(40, 0),
//        glm::u8vec2(30, 11)};
//    for (size_t i = 0; i < texCoords.size(); ++i) {
//      glm::dvec2 uv = texCoords[i];
//      REQUIRE(u8vec2Property.getRaw(uv[0], uv[1]) == expected[i]);
//      REQUIRE(u8vec2Property.get(uv[0], uv[1]) == expected[i]);
//    }
//  }
//
//  SECTION("Access wrong type") {
//    PropertyAttributePropertyView<uint8_t> uint8Invalid =
//        view.getPropertyView<uint8_t>("TestClassProperty");
//    REQUIRE(
//        uint8Invalid.status() ==
//        PropertyAttributePropertyViewStatus::ErrorTypeMismatch);
//
//    PropertyAttributePropertyView<glm::u8vec3> u8vec3Invalid =
//        view.getPropertyView<glm::u8vec3>("TestClassProperty");
//    REQUIRE(
//        u8vec3Invalid.status() ==
//        PropertyAttributePropertyViewStatus::ErrorTypeMismatch);
//  }
//
//  SECTION("Access wrong component type") {
//    PropertyAttributePropertyView<glm::u16vec2> u16vec2Invalid =
//        view.getPropertyView<glm::u16vec2>("TestClassProperty");
//    REQUIRE(
//        u16vec2Invalid.status() ==
//        PropertyAttributePropertyViewStatus::ErrorComponentTypeMismatch);
//
//    PropertyAttributePropertyView<glm::i8vec2> i8vec2Invalid =
//        view.getPropertyView<glm::i8vec2>("TestClassProperty");
//    REQUIRE(
//        i8vec2Invalid.status() ==
//        PropertyAttributePropertyViewStatus::ErrorComponentTypeMismatch);
//  }
//
//  SECTION("Access incorrectly as array") {
//    PropertyAttributePropertyView<PropertyArrayView<glm::u8vec2>> arrayInvalid
//    =
//        view.getPropertyView<PropertyArrayView<glm::u8vec2>>(
//            "TestClassProperty");
//    REQUIRE(
//        arrayInvalid.status() ==
//        PropertyAttributePropertyViewStatus::ErrorArrayTypeMismatch);
//  }
//
//  SECTION("Access incorrectly as normalized") {
//    PropertyAttributePropertyView<glm::u8vec2, true> normalizedInvalid =
//        view.getPropertyView<glm::u8vec2, true>("TestClassProperty");
//    REQUIRE(
//        normalizedInvalid.status() ==
//        PropertyAttributePropertyViewStatus::ErrorNormalizationMismatch);
//  }
//
//  SECTION("Channel and type mismatch") {
//    model.images[imageIndex].cesium.channels = 4;
//    propertyAttributeProperty.channels = {0, 1, 2, 3};
//    PropertyAttributePropertyView<glm::u8vec2> u8vec2Property =
//        view.getPropertyView<glm::u8vec2>("TestClassProperty");
//    REQUIRE(
//        u8vec2Property.status() ==
//        PropertyAttributePropertyViewStatus::ErrorChannelsAndTypeMismatch);
//  }
//
//  SECTION("Invalid channel values") {
//    propertyAttributeProperty.channels = {0, 4};
//    PropertyAttributePropertyView<glm::u8vec2> u8vec2Property =
//        view.getPropertyView<glm::u8vec2>("TestClassProperty");
//    REQUIRE(
//        u8vec2Property.status() ==
//        PropertyAttributePropertyViewStatus::ErrorInvalidChannels);
//  }
//
//  SECTION("Invalid bytes per channel") {
//    model.images[imageIndex].cesium.bytesPerChannel = 2;
//    PropertyAttributePropertyView<glm::u8vec2> u8vec2Property =
//        view.getPropertyView<glm::u8vec2>("TestClassProperty");
//    REQUIRE(
//        u8vec2Property.status() ==
//        PropertyAttributePropertyViewStatus::ErrorInvalidBytesPerChannel);
//  }
//}
//
// TEST_CASE("Test vecN PropertyAttributeProperty (normalized)") {
//  Model model;
//  std::vector<uint8_t> data = {12, 34, 10, 3, 40, 0, 30, 11};
//
//  addAttributeToModel(
//      model,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      2,
//      2,
//      2,
//      data);
//  size_t AttributeIndex = model.Attributes.size() - 1;
//  size_t imageIndex = model.images.size() - 1;
//
//  ExtensionModelExtStructuralMetadata& metadata =
//      model.addExtension<ExtensionModelExtStructuralMetadata>();
//
//  Schema& schema = metadata.schema.emplace();
//  Class& testClass = schema.classes["TestClass"];
//  ClassProperty& testClassProperty =
//  testClass.properties["TestClassProperty"]; testClassProperty.type =
//  ClassProperty::Type::VEC2; testClassProperty.componentType =
//  ClassProperty::ComponentType::UINT8; testClassProperty.normalized = true;
//
//  PropertyAttribute& propertyAttribute =
//  metadata.propertyAttributes.emplace_back(); propertyAttribute.classProperty
//  = "TestClass";
//
//  PropertyAttributeProperty& propertyAttributeProperty =
//      propertyAttribute.properties["TestClassProperty"];
//  propertyAttributeProperty.index = static_cast<int32_t>(AttributeIndex);
//  propertyAttributeProperty.texCoord = 0;
//  propertyAttributeProperty.channels = {0, 1};
//
//  PropertyAttributeView view(model, propertyAttribute);
//  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);
//
//  const ClassProperty* classProperty =
//      view.getClassProperty("TestClassProperty");
//  REQUIRE(classProperty);
//  REQUIRE(classProperty->type == ClassProperty::Type::VEC2);
//  REQUIRE(classProperty->componentType ==
//  ClassProperty::ComponentType::UINT8); REQUIRE(classProperty->count ==
//  std::nullopt); REQUIRE(!classProperty->array);
//  REQUIRE(classProperty->normalized);
//
//  SECTION("Access correct type") {
//    PropertyAttributePropertyView<glm::u8vec2, true> u8vec2Property =
//        view.getPropertyView<glm::u8vec2, true>("TestClassProperty");
//    REQUIRE(
//        u8vec2Property.status() ==
//        PropertyAttributePropertyViewStatus::Valid);
//
//    std::vector<glm::dvec2> texCoords{
//        glm::dvec2(0, 0),
//        glm::dvec2(0.5, 0),
//        glm::dvec2(0, 0.5),
//        glm::dvec2(0.5, 0.5)};
//
//    std::vector<glm::u8vec2> expected{
//        glm::u8vec2(12, 34),
//        glm::u8vec2(10, 3),
//        glm::u8vec2(40, 0),
//        glm::u8vec2(30, 11)};
//    for (size_t i = 0; i < texCoords.size(); ++i) {
//      glm::dvec2 uv = texCoords[i];
//      REQUIRE(u8vec2Property.getRaw(uv[0], uv[1]) == expected[i]);
//      REQUIRE(u8vec2Property.get(uv[0], uv[1]) == normalize(expected[i]));
//    }
//  }
//
//  SECTION("Access wrong type") {
//    PropertyAttributePropertyView<uint8_t, true> uint8Invalid =
//        view.getPropertyView<uint8_t, true>("TestClassProperty");
//    REQUIRE(
//        uint8Invalid.status() ==
//        PropertyAttributePropertyViewStatus::ErrorTypeMismatch);
//
//    PropertyAttributePropertyView<glm::u8vec3, true> u8vec3Invalid =
//        view.getPropertyView<glm::u8vec3, true>("TestClassProperty");
//    REQUIRE(
//        u8vec3Invalid.status() ==
//        PropertyAttributePropertyViewStatus::ErrorTypeMismatch);
//  }
//
//  SECTION("Access wrong component type") {
//    PropertyAttributePropertyView<glm::u16vec2, true> u16vec2Invalid =
//        view.getPropertyView<glm::u16vec2, true>("TestClassProperty");
//    REQUIRE(
//        u16vec2Invalid.status() ==
//        PropertyAttributePropertyViewStatus::ErrorComponentTypeMismatch);
//
//    PropertyAttributePropertyView<glm::i8vec2, true> i8vec2Invalid =
//        view.getPropertyView<glm::i8vec2, true>("TestClassProperty");
//    REQUIRE(
//        i8vec2Invalid.status() ==
//        PropertyAttributePropertyViewStatus::ErrorComponentTypeMismatch);
//  }
//
//  SECTION("Access incorrectly as array") {
//    PropertyAttributePropertyView<PropertyArrayView<glm::u8vec2>, true>
//        arrayInvalid =
//            view.getPropertyView<PropertyArrayView<glm::u8vec2>, true>(
//                "TestClassProperty");
//    REQUIRE(
//        arrayInvalid.status() ==
//        PropertyAttributePropertyViewStatus::ErrorArrayTypeMismatch);
//  }
//
//  SECTION("Access incorrectly as non-normalized") {
//    PropertyAttributePropertyView<glm::u8vec2> normalizedInvalid =
//        view.getPropertyView<glm::u8vec2>("TestClassProperty");
//    REQUIRE(
//        normalizedInvalid.status() ==
//        PropertyAttributePropertyViewStatus::ErrorNormalizationMismatch);
//  }
//
//  SECTION("Access incorrectly as dvec2") {
//    PropertyAttributePropertyView<glm::dvec2> normalizedInvalid =
//        view.getPropertyView<glm::dvec2>("TestClassProperty");
//    REQUIRE(
//        normalizedInvalid.status() ==
//        PropertyAttributePropertyViewStatus::ErrorComponentTypeMismatch);
//  }
//
//  SECTION("Channel and type mismatch") {
//    model.images[imageIndex].cesium.channels = 4;
//    propertyAttributeProperty.channels = {0, 1, 2, 3};
//    PropertyAttributePropertyView<glm::u8vec2, true> u8vec2Property =
//        view.getPropertyView<glm::u8vec2, true>("TestClassProperty");
//    REQUIRE(
//        u8vec2Property.status() ==
//        PropertyAttributePropertyViewStatus::ErrorChannelsAndTypeMismatch);
//  }
//}
//
// TEST_CASE("Test array PropertyAttributeProperty") {
//  Model model;
//  // clang-format off
//  std::vector<uint8_t> data = {
//    12, 34, 10,
//    40, 0, 30,
//    80, 4, 2,
//    6, 3, 4,
//  };
//  // clang-format on
//
//  addAttributeToModel(
//      model,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      2,
//      2,
//      3,
//      data);
//  size_t AttributeIndex = model.Attributes.size() - 1;
//  size_t imageIndex = model.images.size() - 1;
//
//  ExtensionModelExtStructuralMetadata& metadata =
//      model.addExtension<ExtensionModelExtStructuralMetadata>();
//
//  Schema& schema = metadata.schema.emplace();
//  Class& testClass = schema.classes["TestClass"];
//  ClassProperty& testClassProperty =
//  testClass.properties["TestClassProperty"]; testClassProperty.type =
//  ClassProperty::Type::SCALAR; testClassProperty.componentType =
//  ClassProperty::ComponentType::UINT8; testClassProperty.array = true;
//  testClassProperty.count = 3;
//
//  PropertyAttribute& propertyAttribute =
//  metadata.propertyAttributes.emplace_back(); propertyAttribute.classProperty
//  = "TestClass";
//
//  PropertyAttributeProperty& propertyAttributeProperty =
//      propertyAttribute.properties["TestClassProperty"];
//  propertyAttributeProperty.index = static_cast<int32_t>(AttributeIndex);
//  propertyAttributeProperty.texCoord = 0;
//  propertyAttributeProperty.channels = {0, 1, 2};
//
//  PropertyAttributeView view(model, propertyAttribute);
//  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);
//
//  const ClassProperty* classProperty =
//      view.getClassProperty("TestClassProperty");
//  REQUIRE(classProperty);
//  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
//  REQUIRE(classProperty->componentType ==
//  ClassProperty::ComponentType::UINT8); REQUIRE(classProperty->array);
//  REQUIRE(classProperty->count == 3);
//  REQUIRE(!classProperty->normalized);
//
//  SECTION("Access correct type") {
//    PropertyAttributePropertyView<PropertyArrayView<uint8_t>>
//    uint8ArrayProperty
//    =
//        view.getPropertyView<PropertyArrayView<uint8_t>>("TestClassProperty");
//    REQUIRE(
//        uint8ArrayProperty.status() ==
//        PropertyAttributePropertyViewStatus::Valid);
//
//    std::vector<glm::dvec2> texCoords{
//        glm::dvec2(0, 0),
//        glm::dvec2(0.5, 0),
//        glm::dvec2(0, 0.5),
//        glm::dvec2(0.5, 0.5)};
//
//    int64_t size = static_cast<int64_t>(texCoords.size());
//    for (int64_t i = 0; i < size; ++i) {
//      glm::dvec2 uv = texCoords[static_cast<size_t>(i)];
//
//      auto dataStart = data.begin() + i * 3;
//      std::vector<uint8_t> expected(dataStart, dataStart + 3);
//
//      const PropertyArrayView<uint8_t>& value =
//          uint8ArrayProperty.getRaw(uv[0], uv[1]);
//      REQUIRE(static_cast<size_t>(value.size()) == expected.size());
//      for (int64_t j = 0; j < value.size(); j++) {
//        REQUIRE(value[j] == expected[static_cast<size_t>(j)]);
//      }
//
//      auto maybeValue = uint8ArrayProperty.get(uv[0], uv[1]);
//      REQUIRE(maybeValue);
//      for (int64_t j = 0; j < maybeValue->size(); j++) {
//        REQUIRE((*maybeValue)[j] == value[j]);
//      }
//    }
//  }
//
//  SECTION("Access wrong component type") {
//    PropertyAttributePropertyView<PropertyArrayView<int8_t>> int8ArrayInvalid
//    =
//        view.getPropertyView<PropertyArrayView<int8_t>>("TestClassProperty");
//    REQUIRE(
//        int8ArrayInvalid.status() ==
//        PropertyAttributePropertyViewStatus::ErrorComponentTypeMismatch);
//
//    PropertyAttributePropertyView<PropertyArrayView<uint16_t>>
//        uint16ArrayInvalid =
//        view.getPropertyView<PropertyArrayView<uint16_t>>(
//            "TestClassProperty");
//    REQUIRE(
//        uint16ArrayInvalid.status() ==
//        PropertyAttributePropertyViewStatus::ErrorComponentTypeMismatch);
//  }
//
//  SECTION("Access incorrectly as non-array") {
//    PropertyAttributePropertyView<uint8_t> uint8Invalid =
//        view.getPropertyView<uint8_t>("TestClassProperty");
//    REQUIRE(
//        uint8Invalid.status() ==
//        PropertyAttributePropertyViewStatus::ErrorArrayTypeMismatch);
//
//    PropertyAttributePropertyView<glm::u8vec3> u8vec3Invalid =
//        view.getPropertyView<glm::u8vec3>("TestClassProperty");
//    REQUIRE(
//        u8vec3Invalid.status() ==
//        PropertyAttributePropertyViewStatus::ErrorArrayTypeMismatch);
//  }
//
//  SECTION("Access incorrectly as normalized") {
//    PropertyAttributePropertyView<PropertyArrayView<uint8_t>, true>
//        normalizedInvalid =
//            view.getPropertyView<PropertyArrayView<uint8_t>, true>(
//                "TestClassProperty");
//    REQUIRE(
//        normalizedInvalid.status() ==
//        PropertyAttributePropertyViewStatus::ErrorNormalizationMismatch);
//  }
//
//  SECTION("Channel and type mismatch") {
//    model.images[imageIndex].cesium.channels = 4;
//    propertyAttributeProperty.channels = {0, 1, 2, 3};
//    PropertyAttributePropertyView<PropertyArrayView<uint8_t>>
//    uint8ArrayProperty
//    =
//        view.getPropertyView<PropertyArrayView<uint8_t>>("TestClassProperty");
//    REQUIRE(
//        uint8ArrayProperty.status() ==
//        PropertyAttributePropertyViewStatus::ErrorChannelsAndTypeMismatch);
//  }
//
//  SECTION("Invalid channel values") {
//    propertyAttributeProperty.channels = {0, 4, 1};
//    PropertyAttributePropertyView<PropertyArrayView<uint8_t>>
//    uint8ArrayProperty
//    =
//        view.getPropertyView<PropertyArrayView<uint8_t>>("TestClassProperty");
//    REQUIRE(
//        uint8ArrayProperty.status() ==
//        PropertyAttributePropertyViewStatus::ErrorInvalidChannels);
//  }
//
//  SECTION("Invalid bytes per channel") {
//    model.images[imageIndex].cesium.bytesPerChannel = 2;
//    PropertyAttributePropertyView<PropertyArrayView<uint8_t>>
//    uint8ArrayProperty
//    =
//        view.getPropertyView<PropertyArrayView<uint8_t>>("TestClassProperty");
//    REQUIRE(
//        uint8ArrayProperty.status() ==
//        PropertyAttributePropertyViewStatus::ErrorInvalidBytesPerChannel);
//  }
//}
//
// TEST_CASE("Test array PropertyAttributeProperty (normalized)") {
//  Model model;
//  // clang-format off
//  std::vector<uint8_t> data = {
//    12, 34, 10,
//    40, 0, 30,
//    80, 4, 2,
//    6, 3, 4,
//  };
//  // clang-format on
//
//  addAttributeToModel(
//      model,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      2,
//      2,
//      3,
//      data);
//  size_t AttributeIndex = model.Attributes.size() - 1;
//  size_t imageIndex = model.images.size() - 1;
//
//  ExtensionModelExtStructuralMetadata& metadata =
//      model.addExtension<ExtensionModelExtStructuralMetadata>();
//
//  Schema& schema = metadata.schema.emplace();
//  Class& testClass = schema.classes["TestClass"];
//  ClassProperty& testClassProperty =
//  testClass.properties["TestClassProperty"]; testClassProperty.type =
//  ClassProperty::Type::SCALAR; testClassProperty.componentType =
//  ClassProperty::ComponentType::UINT8; testClassProperty.array = true;
//  testClassProperty.count = 3;
//  testClassProperty.normalized = true;
//
//  PropertyAttribute& propertyAttribute =
//  metadata.propertyAttributes.emplace_back(); propertyAttribute.classProperty
//  = "TestClass";
//
//  PropertyAttributeProperty& propertyAttributeProperty =
//      propertyAttribute.properties["TestClassProperty"];
//  propertyAttributeProperty.index = static_cast<int32_t>(AttributeIndex);
//  propertyAttributeProperty.texCoord = 0;
//  propertyAttributeProperty.channels = {0, 1, 2};
//
//  PropertyAttributeView view(model, propertyAttribute);
//  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);
//
//  const ClassProperty* classProperty =
//      view.getClassProperty("TestClassProperty");
//  REQUIRE(classProperty);
//  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
//  REQUIRE(classProperty->componentType ==
//  ClassProperty::ComponentType::UINT8); REQUIRE(classProperty->array);
//  REQUIRE(classProperty->count == 3);
//  REQUIRE(classProperty->normalized);
//
//  SECTION("Access correct type") {
//    PropertyAttributePropertyView<PropertyArrayView<uint8_t>, true>
//        uint8ArrayProperty =
//            view.getPropertyView<PropertyArrayView<uint8_t>, true>(
//                "TestClassProperty");
//    REQUIRE(
//        uint8ArrayProperty.status() ==
//        PropertyAttributePropertyViewStatus::Valid);
//
//    std::vector<glm::dvec2> texCoords{
//        glm::dvec2(0, 0),
//        glm::dvec2(0.5, 0),
//        glm::dvec2(0, 0.5),
//        glm::dvec2(0.5, 0.5)};
//
//    int64_t size = static_cast<int64_t>(texCoords.size());
//    for (int64_t i = 0; i < size; ++i) {
//      glm::dvec2 uv = texCoords[static_cast<size_t>(i)];
//
//      auto dataStart = data.begin() + i * 3;
//      std::vector<uint8_t> expected(dataStart, dataStart + 3);
//
//      const PropertyArrayView<uint8_t>& value =
//          uint8ArrayProperty.getRaw(uv[0], uv[1]);
//      REQUIRE(static_cast<size_t>(value.size()) == expected.size());
//      for (int64_t j = 0; j < value.size(); j++) {
//        REQUIRE(value[j] == expected[static_cast<size_t>(j)]);
//      }
//
//      auto maybeValue = uint8ArrayProperty.get(uv[0], uv[1]);
//      REQUIRE(maybeValue);
//      for (int64_t j = 0; j < maybeValue->size(); j++) {
//        REQUIRE((*maybeValue)[j] == normalize(value[j]));
//      }
//    }
//  }
//
//  SECTION("Access wrong component type") {
//    PropertyAttributePropertyView<PropertyArrayView<int8_t>, true>
//        int8ArrayInvalid =
//            view.getPropertyView<PropertyArrayView<int8_t>, true>(
//                "TestClassProperty");
//    REQUIRE(
//        int8ArrayInvalid.status() ==
//        PropertyAttributePropertyViewStatus::ErrorComponentTypeMismatch);
//
//    PropertyAttributePropertyView<PropertyArrayView<uint16_t>, true>
//        uint16ArrayInvalid =
//            view.getPropertyView<PropertyArrayView<uint16_t>, true>(
//                "TestClassProperty");
//    REQUIRE(
//        uint16ArrayInvalid.status() ==
//        PropertyAttributePropertyViewStatus::ErrorComponentTypeMismatch);
//  }
//
//  SECTION("Access incorrectly as non-array") {
//    PropertyAttributePropertyView<uint8_t, true> uint8Invalid =
//        view.getPropertyView<uint8_t, true>("TestClassProperty");
//    REQUIRE(
//        uint8Invalid.status() ==
//        PropertyAttributePropertyViewStatus::ErrorArrayTypeMismatch);
//
//    PropertyAttributePropertyView<glm::u8vec3, true> u8vec3Invalid =
//        view.getPropertyView<glm::u8vec3, true>("TestClassProperty");
//    REQUIRE(
//        u8vec3Invalid.status() ==
//        PropertyAttributePropertyViewStatus::ErrorArrayTypeMismatch);
//  }
//
//  SECTION("Access incorrectly as normalized") {
//    PropertyAttributePropertyView<PropertyArrayView<uint8_t>>
//    normalizedInvalid
//    =
//        view.getPropertyView<PropertyArrayView<uint8_t>>("TestClassProperty");
//    REQUIRE(
//        normalizedInvalid.status() ==
//        PropertyAttributePropertyViewStatus::ErrorNormalizationMismatch);
//  }
//
//  SECTION("Channel and type mismatch") {
//    model.images[imageIndex].cesium.channels = 4;
//    propertyAttributeProperty.channels = {0, 1, 2, 3};
//    PropertyAttributePropertyView<PropertyArrayView<uint8_t>, true>
//        uint8ArrayProperty =
//            view.getPropertyView<PropertyArrayView<uint8_t>, true>(
//                "TestClassProperty");
//    REQUIRE(
//        uint8ArrayProperty.status() ==
//        PropertyAttributePropertyViewStatus::ErrorChannelsAndTypeMismatch);
//  }
//}
//
// TEST_CASE("Test with PropertyAttributeProperty offset, scale, min, max") {
//  Model model;
//  // clang-format off
//    std::vector<uint8_t> data{
//      0, 0, 0, 1,
//      9, 0, 1, 0,
//      20, 2, 2, 0,
//      8, 1, 0, 1};
//  // clang-format on
//
//  std::vector<uint32_t> expectedUint{16777216, 65545, 131604, 16777480};
//
//  const float offset = 1.0f;
//  const float scale = 2.0f;
//  const float min = -10.0f;
//  const float max = 10.0f;
//
//  addAttributeToModel(
//      model,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      2,
//      2,
//      4,
//      data);
//  size_t AttributeIndex = model.Attributes.size() - 1;
//
//  ExtensionModelExtStructuralMetadata& metadata =
//      model.addExtension<ExtensionModelExtStructuralMetadata>();
//
//  Schema& schema = metadata.schema.emplace();
//  Class& testClass = schema.classes["TestClass"];
//  ClassProperty& testClassProperty =
//  testClass.properties["TestClassProperty"]; testClassProperty.type =
//  ClassProperty::Type::SCALAR; testClassProperty.componentType =
//  ClassProperty::ComponentType::FLOAT32; testClassProperty.offset = offset;
//  testClassProperty.scale = scale;
//  testClassProperty.min = min;
//  testClassProperty.max = max;
//
//  PropertyAttribute& propertyAttribute =
//  metadata.propertyAttributes.emplace_back(); propertyAttribute.classProperty
//  = "TestClass";
//
//  PropertyAttributeProperty& propertyAttributeProperty =
//      propertyAttribute.properties["TestClassProperty"];
//  propertyAttributeProperty.index = static_cast<int32_t>(AttributeIndex);
//  propertyAttributeProperty.texCoord = 0;
//  propertyAttributeProperty.channels = {0, 1, 2, 3};
//
//  PropertyAttributeView view(model, propertyAttribute);
//  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);
//
//  const ClassProperty* classProperty =
//      view.getClassProperty("TestClassProperty");
//  REQUIRE(classProperty);
//  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
//  REQUIRE(
//      classProperty->componentType == ClassProperty::ComponentType::FLOAT32);
//  REQUIRE(classProperty->count == std::nullopt);
//  REQUIRE(!classProperty->array);
//  REQUIRE(!classProperty->normalized);
//  REQUIRE(classProperty->offset);
//  REQUIRE(classProperty->scale);
//  REQUIRE(classProperty->min);
//  REQUIRE(classProperty->max);
//
//  std::vector<glm::dvec2> texCoords{
//      glm::dvec2(0, 0),
//      glm::dvec2(0.5, 0),
//      glm::dvec2(0, 0.5),
//      glm::dvec2(0.5, 0.5)};
//
//  SECTION("Use class property values") {
//    PropertyAttributePropertyView<float> property =
//        view.getPropertyView<float>("TestClassProperty");
//    REQUIRE(property.status() == PropertyAttributePropertyViewStatus::Valid);
//    REQUIRE(property.offset() == offset);
//    REQUIRE(property.scale() == scale);
//    REQUIRE(property.min() == min);
//    REQUIRE(property.max() == max);
//
//    std::vector<float> expectedRaw(expectedUint.size());
//    std::vector<float> expectedTransformed(expectedUint.size());
//    for (size_t i = 0; i < expectedUint.size(); i++) {
//      float value = *reinterpret_cast<float*>(&expectedUint[i]);
//      expectedRaw[i] = value;
//      expectedTransformed[i] = value * scale + offset;
//    }
//
//    for (size_t i = 0; i < texCoords.size(); ++i) {
//      glm::dvec2 uv = texCoords[i];
//      REQUIRE(property.getRaw(uv[0], uv[1]) == expectedRaw[i]);
//      REQUIRE(property.get(uv[0], uv[1]) == expectedTransformed[i]);
//    }
//  }
//
//  SECTION("Use own property values") {
//    const float newOffset = 1.0f;
//    const float newScale = -1.0f;
//    const float newMin = -3.0f;
//    const float newMax = 0.0f;
//    propertyAttributeProperty.offset = newOffset;
//    propertyAttributeProperty.scale = newScale;
//    propertyAttributeProperty.min = newMin;
//    propertyAttributeProperty.max = newMax;
//
//    PropertyAttributePropertyView<float> property =
//        view.getPropertyView<float>("TestClassProperty");
//    REQUIRE(property.status() == PropertyAttributePropertyViewStatus::Valid);
//    REQUIRE(property.offset() == newOffset);
//    REQUIRE(property.scale() == newScale);
//    REQUIRE(property.min() == newMin);
//    REQUIRE(property.max() == newMax);
//
//    std::vector<float> expectedRaw(expectedUint.size());
//    std::vector<float> expectedTransformed(expectedUint.size());
//    for (size_t i = 0; i < expectedUint.size(); i++) {
//      float value = *reinterpret_cast<float*>(&expectedUint[i]);
//      expectedRaw[i] = value;
//      expectedTransformed[i] = value * newScale + newOffset;
//    }
//
//    for (size_t i = 0; i < texCoords.size(); ++i) {
//      glm::dvec2 uv = texCoords[i];
//      REQUIRE(property.getRaw(uv[0], uv[1]) == expectedRaw[i]);
//      REQUIRE(property.get(uv[0], uv[1]) == expectedTransformed[i]);
//    }
//  }
//}
//
// TEST_CASE(
//    "Test with PropertyAttributeProperty offset, scale, min, max
//    (normalized)")
//    {
//  Model model;
//  std::vector<uint8_t> data = {12, 34, 30, 11};
//
//  const double offset = 1.0;
//  const double scale = 2.0;
//  const double min = 1.0;
//  const double max = 3.0;
//
//  addAttributeToModel(
//      model,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      2,
//      2,
//      1,
//      data);
//  size_t AttributeIndex = model.Attributes.size() - 1;
//
//  ExtensionModelExtStructuralMetadata& metadata =
//      model.addExtension<ExtensionModelExtStructuralMetadata>();
//
//  Schema& schema = metadata.schema.emplace();
//  Class& testClass = schema.classes["TestClass"];
//  ClassProperty& testClassProperty =
//  testClass.properties["TestClassProperty"]; testClassProperty.type =
//  ClassProperty::Type::SCALAR; testClassProperty.componentType =
//  ClassProperty::ComponentType::UINT8; testClassProperty.normalized = true;
//  testClassProperty.offset = offset;
//  testClassProperty.scale = scale;
//  testClassProperty.min = min;
//  testClassProperty.max = max;
//
//  PropertyAttribute& propertyAttribute =
//  metadata.propertyAttributes.emplace_back(); propertyAttribute.classProperty
//  = "TestClass";
//
//  PropertyAttributeProperty& propertyAttributeProperty =
//      propertyAttribute.properties["TestClassProperty"];
//  propertyAttributeProperty.index = static_cast<int32_t>(AttributeIndex);
//  propertyAttributeProperty.texCoord = 0;
//  propertyAttributeProperty.channels = {0};
//
//  PropertyAttributeView view(model, propertyAttribute);
//  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);
//
//  const ClassProperty* classProperty =
//      view.getClassProperty("TestClassProperty");
//  REQUIRE(classProperty);
//  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
//  REQUIRE(classProperty->componentType ==
//  ClassProperty::ComponentType::UINT8); REQUIRE(classProperty->count ==
//  std::nullopt); REQUIRE(!classProperty->array);
//  REQUIRE(classProperty->normalized);
//
//  std::vector<glm::dvec2> texCoords{
//      glm::dvec2(0, 0),
//      glm::dvec2(0.5, 0),
//      glm::dvec2(0, 0.5),
//      glm::dvec2(0.5, 0.5)};
//
//  SECTION("Use class property values") {
//    PropertyAttributePropertyView<uint8_t, true> property =
//        view.getPropertyView<uint8_t, true>("TestClassProperty");
//    REQUIRE(property.status() == PropertyAttributePropertyViewStatus::Valid);
//    REQUIRE(property.offset() == offset);
//    REQUIRE(property.scale() == scale);
//    REQUIRE(property.min() == min);
//    REQUIRE(property.max() == max);
//
//    for (size_t i = 0; i < texCoords.size(); ++i) {
//      glm::dvec2 uv = texCoords[i];
//      REQUIRE(property.getRaw(uv[0], uv[1]) == data[i]);
//      REQUIRE(
//          property.get(uv[0], uv[1]) == normalize(data[i]) * scale + offset);
//    }
//  }
//
//  SECTION("Use own property values") {
//    const double newOffset = 2.0;
//    const double newScale = 5.0;
//    const double newMin = 10.0;
//    const double newMax = 11.0;
//    propertyAttributeProperty.offset = newOffset;
//    propertyAttributeProperty.scale = newScale;
//    propertyAttributeProperty.min = newMin;
//    propertyAttributeProperty.max = newMax;
//
//    PropertyAttributePropertyView<uint8_t, true> property =
//        view.getPropertyView<uint8_t, true>("TestClassProperty");
//    REQUIRE(property.status() == PropertyAttributePropertyViewStatus::Valid);
//    REQUIRE(property.offset() == newOffset);
//    REQUIRE(property.scale() == newScale);
//    REQUIRE(property.min() == newMin);
//    REQUIRE(property.max() == newMax);
//
//    for (size_t i = 0; i < texCoords.size(); ++i) {
//      glm::dvec2 uv = texCoords[i];
//      REQUIRE(property.getRaw(uv[0], uv[1]) == data[i]);
//      REQUIRE(
//          property.get(uv[0], uv[1]) ==
//          normalize(data[i]) * newScale + newOffset);
//    }
//  }
//}
//
// TEST_CASE("Test with PropertyAttributeProperty noData") {
//  Model model;
//  std::vector<uint8_t> data = {12, 34, 30, 11};
//  const uint8_t noData = 34;
//
//  addAttributeToModel(
//      model,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      2,
//      2,
//      1,
//      data);
//  size_t AttributeIndex = model.Attributes.size() - 1;
//
//  ExtensionModelExtStructuralMetadata& metadata =
//      model.addExtension<ExtensionModelExtStructuralMetadata>();
//
//  Schema& schema = metadata.schema.emplace();
//  Class& testClass = schema.classes["TestClass"];
//  ClassProperty& testClassProperty =
//  testClass.properties["TestClassProperty"]; testClassProperty.type =
//  ClassProperty::Type::SCALAR; testClassProperty.componentType =
//  ClassProperty::ComponentType::UINT8; testClassProperty.noData = noData;
//
//  PropertyAttribute& propertyAttribute =
//  metadata.propertyAttributes.emplace_back(); propertyAttribute.classProperty
//  = "TestClass";
//
//  PropertyAttributeProperty& propertyAttributeProperty =
//      propertyAttribute.properties["TestClassProperty"];
//  propertyAttributeProperty.index = static_cast<int32_t>(AttributeIndex);
//  propertyAttributeProperty.texCoord = 0;
//  propertyAttributeProperty.channels = {0};
//
//  PropertyAttributeView view(model, propertyAttribute);
//  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);
//
//  const ClassProperty* classProperty =
//      view.getClassProperty("TestClassProperty");
//  REQUIRE(classProperty);
//  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
//  REQUIRE(classProperty->componentType ==
//  ClassProperty::ComponentType::UINT8); REQUIRE(classProperty->count ==
//  std::nullopt); REQUIRE(!classProperty->array);
//  REQUIRE(!classProperty->normalized);
//
//  std::vector<glm::dvec2> texCoords{
//      glm::dvec2(0, 0),
//      glm::dvec2(0.5, 0),
//      glm::dvec2(0, 0.5),
//      glm::dvec2(0.5, 0.5)};
//
//  SECTION("Without default value") {
//    PropertyAttributePropertyView<uint8_t> property =
//        view.getPropertyView<uint8_t>("TestClassProperty");
//    REQUIRE(property.status() == PropertyAttributePropertyViewStatus::Valid);
//
//    for (size_t i = 0; i < texCoords.size(); ++i) {
//      glm::dvec2 uv = texCoords[i];
//      auto value = property.getRaw(uv[0], uv[1]);
//      REQUIRE(value == data[i]);
//
//      auto maybeValue = property.get(uv[0], uv[1]);
//      if (value == noData) {
//        REQUIRE(!maybeValue);
//      } else {
//        REQUIRE(maybeValue == data[i]);
//      }
//    }
//  }
//
//  SECTION("With default value") {
//    const uint8_t defaultValue = 255;
//    testClassProperty.defaultProperty = defaultValue;
//
//    PropertyAttributePropertyView<uint8_t> property =
//        view.getPropertyView<uint8_t>("TestClassProperty");
//    REQUIRE(property.status() == PropertyAttributePropertyViewStatus::Valid);
//
//    for (size_t i = 0; i < texCoords.size(); ++i) {
//      glm::dvec2 uv = texCoords[i];
//      auto value = property.getRaw(uv[0], uv[1]);
//      REQUIRE(value == data[i]);
//
//      auto maybeValue = property.get(uv[0], uv[1]);
//      if (value == noData) {
//        REQUIRE(maybeValue == defaultValue);
//      } else {
//        REQUIRE(maybeValue == data[i]);
//      }
//    }
//  }
//}
//
// TEST_CASE("Test with PropertyAttributeProperty noData (normalized)") {
//  Model model;
//  std::vector<uint8_t> data = {12, 34, 30, 11};
//  const uint8_t noData = 34;
//
//  addAttributeToModel(
//      model,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      2,
//      2,
//      1,
//      data);
//  size_t AttributeIndex = model.Attributes.size() - 1;
//
//  ExtensionModelExtStructuralMetadata& metadata =
//      model.addExtension<ExtensionModelExtStructuralMetadata>();
//
//  Schema& schema = metadata.schema.emplace();
//  Class& testClass = schema.classes["TestClass"];
//  ClassProperty& testClassProperty =
//  testClass.properties["TestClassProperty"]; testClassProperty.type =
//  ClassProperty::Type::SCALAR; testClassProperty.componentType =
//  ClassProperty::ComponentType::UINT8; testClassProperty.normalized = true;
//  testClassProperty.noData = noData;
//
//  PropertyAttribute& propertyAttribute =
//  metadata.propertyAttributes.emplace_back(); propertyAttribute.classProperty
//  = "TestClass";
//
//  PropertyAttributeProperty& propertyAttributeProperty =
//      propertyAttribute.properties["TestClassProperty"];
//  propertyAttributeProperty.index = static_cast<int32_t>(AttributeIndex);
//  propertyAttributeProperty.texCoord = 0;
//  propertyAttributeProperty.channels = {0};
//
//  PropertyAttributeView view(model, propertyAttribute);
//  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);
//
//  const ClassProperty* classProperty =
//      view.getClassProperty("TestClassProperty");
//  REQUIRE(classProperty);
//  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
//  REQUIRE(classProperty->componentType ==
//  ClassProperty::ComponentType::UINT8); REQUIRE(classProperty->count ==
//  std::nullopt); REQUIRE(!classProperty->array);
//  REQUIRE(classProperty->normalized);
//
//  std::vector<glm::dvec2> texCoords{
//      glm::dvec2(0, 0),
//      glm::dvec2(0.5, 0),
//      glm::dvec2(0, 0.5),
//      glm::dvec2(0.5, 0.5)};
//
//  SECTION("Without default value") {
//    PropertyAttributePropertyView<uint8_t, true> property =
//        view.getPropertyView<uint8_t, true>("TestClassProperty");
//    REQUIRE(property.status() == PropertyAttributePropertyViewStatus::Valid);
//
//    for (size_t i = 0; i < texCoords.size(); ++i) {
//      glm::dvec2 uv = texCoords[i];
//      auto value = property.getRaw(uv[0], uv[1]);
//      REQUIRE(value == data[i]);
//
//      auto maybeValue = property.get(uv[0], uv[1]);
//      if (value == noData) {
//        REQUIRE(!maybeValue);
//      } else {
//        REQUIRE(maybeValue == normalize(data[i]));
//      }
//    }
//  }
//
//  SECTION("With default value") {
//    const double defaultValue = -1.0;
//    testClassProperty.defaultProperty = defaultValue;
//
//    PropertyAttributePropertyView<uint8_t, true> property =
//        view.getPropertyView<uint8_t, true>("TestClassProperty");
//    REQUIRE(property.status() == PropertyAttributePropertyViewStatus::Valid);
//
//    for (size_t i = 0; i < texCoords.size(); ++i) {
//      glm::dvec2 uv = texCoords[i];
//      auto value = property.getRaw(uv[0], uv[1]);
//      REQUIRE(value == data[i]);
//
//      auto maybeValue = property.get(uv[0], uv[1]);
//      if (value == noData) {
//        REQUIRE(maybeValue == defaultValue);
//      } else {
//        REQUIRE(maybeValue == normalize(data[i]));
//      }
//    }
//  }
//}
//
// TEST_CASE("Test callback on invalid property Attribute view") {
//  Model model;
//  ExtensionModelExtStructuralMetadata& metadata =
//      model.addExtension<ExtensionModelExtStructuralMetadata>();
//  metadata.schema.emplace();
//
//  // Property Attribute has a nonexistent class.
//  PropertyAttribute& propertyAttribute =
//  metadata.propertyAttributes.emplace_back(); propertyAttribute.classProperty
//  = "TestClass";
//
//  PropertyAttributeProperty& propertyAttributeProperty =
//      propertyAttribute.properties["TestClassProperty"];
//  propertyAttributeProperty.index = -1;
//
//  PropertyAttributeView view(model, propertyAttribute);
//  REQUIRE(view.status() == PropertyAttributeViewStatus::ErrorClassNotFound);
//
//  const ClassProperty* classProperty =
//      view.getClassProperty("TestClassProperty");
//  REQUIRE(!classProperty);
//
//  uint32_t invokedCallbackCount = 0;
//  view.getPropertyView(
//      "TestClassProperty",
//      [&invokedCallbackCount](
//          const std::string& /*propertyName*/,
//          auto propertyValue) mutable {
//        invokedCallbackCount++;
//        REQUIRE(
//            propertyValue.status() ==
//            PropertyAttributePropertyViewStatus::ErrorInvalidPropertyAttribute);
//      });
//
//  REQUIRE(invokedCallbackCount == 1);
//}
//
// TEST_CASE("Test callback on invalid PropertyAttributeProperty") {
//  Model model;
//  ExtensionModelExtStructuralMetadata& metadata =
//      model.addExtension<ExtensionModelExtStructuralMetadata>();
//
//  Schema& schema = metadata.schema.emplace();
//  Class& testClass = schema.classes["TestClass"];
//  ClassProperty& testClassProperty = testClass.properties["InvalidProperty"];
//  testClassProperty.type = ClassProperty::Type::SCALAR;
//  testClassProperty.componentType = ClassProperty::ComponentType::UINT8;
//
//  PropertyAttribute& propertyAttribute =
//  metadata.propertyAttributes.emplace_back(); propertyAttribute.classProperty
//  = "TestClass";
//
//  PropertyAttributeProperty& propertyAttributeProperty =
//      propertyAttribute.properties["InvalidProperty"];
//  propertyAttributeProperty.index = -1;
//
//  PropertyAttributeView view(model, propertyAttribute);
//  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);
//
//  const ClassProperty* classProperty =
//  view.getClassProperty("InvalidProperty"); REQUIRE(classProperty);
//
//  classProperty = view.getClassProperty("NonexistentProperty");
//  REQUIRE(!classProperty);
//
//  uint32_t invokedCallbackCount = 0;
//  auto testCallback = [&invokedCallbackCount](
//                          const std::string& /*propertyName*/,
//                          auto propertyValue) mutable {
//    invokedCallbackCount++;
//    REQUIRE(propertyValue.status() !=
//    PropertyAttributePropertyViewStatus::Valid);
//  };
//
//  view.getPropertyView("InvalidProperty", testCallback);
//  view.getPropertyView("NonexistentProperty", testCallback);
//
//  REQUIRE(invokedCallbackCount == 2);
//}
//
// TEST_CASE("Test callback on invalid normalized PropertyAttributeProperty") {
//  Model model;
//  ExtensionModelExtStructuralMetadata& metadata =
//      model.addExtension<ExtensionModelExtStructuralMetadata>();
//
//  Schema& schema = metadata.schema.emplace();
//  Class& testClass = schema.classes["TestClass"];
//  ClassProperty& testClassProperty =
//  testClass.properties["TestClassProperty"]; testClassProperty.type =
//  ClassProperty::Type::SCALAR; testClassProperty.componentType =
//  ClassProperty::ComponentType::FLOAT32; testClassProperty.normalized = true;
//  // This is erroneous.
//
//  PropertyAttribute& propertyAttribute =
//  metadata.propertyAttributes.emplace_back(); propertyAttribute.classProperty
//  = "TestClass";
//
//  PropertyAttributeProperty& propertyAttributeProperty =
//      propertyAttribute.properties["TestClassProperty"];
//  propertyAttributeProperty.index = static_cast<int32_t>(0);
//  propertyAttributeProperty.texCoord = 0;
//  propertyAttributeProperty.channels = {0, 1};
//
//  PropertyAttributeView view(model, propertyAttribute);
//  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);
//
//  const ClassProperty* classProperty =
//      view.getClassProperty("TestClassProperty");
//  REQUIRE(classProperty);
//
//  uint32_t invokedCallbackCount = 0;
//  auto testCallback = [&invokedCallbackCount](
//                          const std::string& /*propertyName*/,
//                          auto propertyValue) mutable {
//    invokedCallbackCount++;
//    REQUIRE(
//        propertyValue.status() ==
//        PropertyAttributePropertyViewStatus::ErrorInvalidNormalization);
//  };
//
//  view.getPropertyView("TestClassProperty", testCallback);
//
//  REQUIRE(invokedCallbackCount == 1);
//}
//
// TEST_CASE("Test callback for scalar PropertyAttributeProperty") {
//  Model model;
//  std::vector<uint8_t> data = {255, 255, 12, 1, 30, 2, 0, 255};
//
//  addAttributeToModel(
//      model,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      2,
//      2,
//      2,
//      data);
//  size_t AttributeIndex = model.Attributes.size() - 1;
//
//  ExtensionModelExtStructuralMetadata& metadata =
//      model.addExtension<ExtensionModelExtStructuralMetadata>();
//
//  Schema& schema = metadata.schema.emplace();
//  Class& testClass = schema.classes["TestClass"];
//  ClassProperty& testClassProperty =
//  testClass.properties["TestClassProperty"]; testClassProperty.type =
//  ClassProperty::Type::SCALAR; testClassProperty.componentType =
//  ClassProperty::ComponentType::INT16;
//
//  PropertyAttribute& propertyAttribute =
//  metadata.propertyAttributes.emplace_back(); propertyAttribute.classProperty
//  = "TestClass";
//
//  PropertyAttributeProperty& propertyAttributeProperty =
//      propertyAttribute.properties["TestClassProperty"];
//  propertyAttributeProperty.index = static_cast<int32_t>(AttributeIndex);
//  propertyAttributeProperty.texCoord = 0;
//  propertyAttributeProperty.channels = {0, 1};
//
//  PropertyAttributeView view(model, propertyAttribute);
//  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);
//
//  const ClassProperty* classProperty =
//      view.getClassProperty("TestClassProperty");
//  REQUIRE(classProperty);
//  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
//  REQUIRE(classProperty->componentType ==
//  ClassProperty::ComponentType::INT16); REQUIRE(classProperty->count ==
//  std::nullopt); REQUIRE(!classProperty->array);
//  REQUIRE(!classProperty->normalized);
//
//  std::vector<glm::dvec2> texCoords{
//      glm::dvec2(0, 0),
//      glm::dvec2(0.5, 0),
//      glm::dvec2(0, 0.5),
//      glm::dvec2(0.5, 0.5)};
//
//  std::vector<int16_t> expected{-1, 268, 542, -256};
//  uint32_t invokedCallbackCount = 0;
//  view.getPropertyView(
//      "TestClassProperty",
//      [&expected, &texCoords, &invokedCallbackCount](
//          const std::string& /*propertyName*/,
//          auto propertyValue) mutable {
//        invokedCallbackCount++;
//        if constexpr (std::is_same_v<
//                          PropertyAttributePropertyView<int16_t>,
//                          decltype(propertyValue)>) {
//          REQUIRE(
//              propertyValue.status() ==
//              PropertyAttributePropertyViewStatus::Valid);
//
//          for (size_t i = 0; i < expected.size(); ++i) {
//            glm::dvec2& uv = texCoords[i];
//            REQUIRE(propertyValue.getRaw(uv[0], uv[1]) == expected[i]);
//            REQUIRE(propertyValue.get(uv[0], uv[1]) == expected[i]);
//          }
//        } else {
//          FAIL("getPropertyView returned PropertyAttributePropertyView of "
//               "incorrect type for TestClassProperty.");
//        }
//      });
//
//  REQUIRE(invokedCallbackCount == 1);
//}
//
// TEST_CASE("Test callback for scalar PropertyAttributeProperty (normalized)")
// {
//  Model model;
//  std::vector<uint8_t> data = {255, 255, 12, 1, 30, 2, 0, 255};
//
//  addAttributeToModel(
//      model,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      2,
//      2,
//      2,
//      data);
//  size_t AttributeIndex = model.Attributes.size() - 1;
//
//  ExtensionModelExtStructuralMetadata& metadata =
//      model.addExtension<ExtensionModelExtStructuralMetadata>();
//
//  Schema& schema = metadata.schema.emplace();
//  Class& testClass = schema.classes["TestClass"];
//  ClassProperty& testClassProperty =
//  testClass.properties["TestClassProperty"]; testClassProperty.type =
//  ClassProperty::Type::SCALAR; testClassProperty.componentType =
//  ClassProperty::ComponentType::INT16; testClassProperty.normalized = true;
//
//  PropertyAttribute& propertyAttribute =
//  metadata.propertyAttributes.emplace_back(); propertyAttribute.classProperty
//  = "TestClass";
//
//  PropertyAttributeProperty& propertyAttributeProperty =
//      propertyAttribute.properties["TestClassProperty"];
//  propertyAttributeProperty.index = static_cast<int32_t>(AttributeIndex);
//  propertyAttributeProperty.texCoord = 0;
//  propertyAttributeProperty.channels = {0, 1};
//
//  PropertyAttributeView view(model, propertyAttribute);
//  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);
//
//  const ClassProperty* classProperty =
//      view.getClassProperty("TestClassProperty");
//  REQUIRE(classProperty);
//  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
//  REQUIRE(classProperty->componentType ==
//  ClassProperty::ComponentType::INT16); REQUIRE(classProperty->count ==
//  std::nullopt); REQUIRE(!classProperty->array);
//  REQUIRE(classProperty->normalized);
//
//  std::vector<glm::dvec2> texCoords{
//      glm::dvec2(0, 0),
//      glm::dvec2(0.5, 0),
//      glm::dvec2(0, 0.5),
//      glm::dvec2(0.5, 0.5)};
//
//  std::vector<int16_t> expected{-1, 268, 542, -256};
//  uint32_t invokedCallbackCount = 0;
//  view.getPropertyView(
//      "TestClassProperty",
//      [&expected, &texCoords, &invokedCallbackCount](
//          const std::string& /*propertyName*/,
//          auto propertyValue) mutable {
//        invokedCallbackCount++;
//        if constexpr (std::is_same_v<
//                          PropertyAttributePropertyView<int16_t, true>,
//                          decltype(propertyValue)>) {
//          REQUIRE(
//              propertyValue.status() ==
//              PropertyAttributePropertyViewStatus::Valid);
//
//          for (size_t i = 0; i < expected.size(); ++i) {
//            glm::dvec2& uv = texCoords[i];
//            REQUIRE(propertyValue.getRaw(uv[0], uv[1]) == expected[i]);
//            REQUIRE(propertyValue.get(uv[0], uv[1]) ==
//            normalize(expected[i]));
//          }
//        } else {
//          FAIL("getPropertyView returned PropertyAttributePropertyView of "
//               "incorrect type for TestClassProperty.");
//        }
//      });
//
//  REQUIRE(invokedCallbackCount == 1);
//}
//
// TEST_CASE("Test callback for vecN PropertyAttributeProperty") {
//  Model model;
//  // clang-format off
//  std::vector<uint8_t> data = {
//    255, 255,
//    12, 1,
//    30, 2,
//    0, 255};
//  // clang-format on
//
//  addAttributeToModel(
//      model,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      2,
//      2,
//      2,
//      data);
//  size_t AttributeIndex = model.Attributes.size() - 1;
//
//  ExtensionModelExtStructuralMetadata& metadata =
//      model.addExtension<ExtensionModelExtStructuralMetadata>();
//
//  Schema& schema = metadata.schema.emplace();
//  Class& testClass = schema.classes["TestClass"];
//  ClassProperty& testClassProperty =
//  testClass.properties["TestClassProperty"]; testClassProperty.type =
//  ClassProperty::Type::VEC2; testClassProperty.componentType =
//  ClassProperty::ComponentType::INT8;
//
//  PropertyAttribute& propertyAttribute =
//  metadata.propertyAttributes.emplace_back(); propertyAttribute.classProperty
//  = "TestClass";
//
//  PropertyAttributeProperty& propertyAttributeProperty =
//      propertyAttribute.properties["TestClassProperty"];
//  propertyAttributeProperty.index = static_cast<int32_t>(AttributeIndex);
//  propertyAttributeProperty.texCoord = 0;
//  propertyAttributeProperty.channels = {0, 1};
//
//  PropertyAttributeView view(model, propertyAttribute);
//  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);
//
//  const ClassProperty* classProperty =
//      view.getClassProperty("TestClassProperty");
//  REQUIRE(classProperty);
//  REQUIRE(classProperty->type == ClassProperty::Type::VEC2);
//  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT8);
//  REQUIRE(classProperty->count == std::nullopt);
//  REQUIRE(!classProperty->array);
//  REQUIRE(!classProperty->normalized);
//
//  std::vector<glm::i8vec2> expected{
//      glm::i8vec2(-1, -1),
//      glm::i8vec2(12, 1),
//      glm::i8vec2(30, 2),
//      glm::i8vec2(0, -1)};
//
//  std::vector<glm::dvec2> texCoords{
//      glm::dvec2(0, 0),
//      glm::dvec2(0.5, 0),
//      glm::dvec2(0, 0.5),
//      glm::dvec2(0.5, 0.5)};
//
//  uint32_t invokedCallbackCount = 0;
//  view.getPropertyView(
//      "TestClassProperty",
//      [&expected, &texCoords, &invokedCallbackCount](
//          const std::string& /*propertyName*/,
//          auto propertyValue) mutable {
//        invokedCallbackCount++;
//        if constexpr (std::is_same_v<
//                          PropertyAttributePropertyView<glm::i8vec2>,
//                          decltype(propertyValue)>) {
//          REQUIRE(
//              propertyValue.status() ==
//              PropertyAttributePropertyViewStatus::Valid);
//
//          for (size_t i = 0; i < expected.size(); ++i) {
//            glm::dvec2& uv = texCoords[i];
//            REQUIRE(propertyValue.getRaw(uv[0], uv[1]) == expected[i]);
//            REQUIRE(propertyValue.get(uv[0], uv[1]) == expected[i]);
//          }
//        } else {
//          FAIL("getPropertyView returned PropertyAttributePropertyView of "
//               "incorrect type for TestClassProperty.");
//        }
//      });
//
//  REQUIRE(invokedCallbackCount == 1);
//}
//
// TEST_CASE("Test callback for vecN PropertyAttributeProperty (normalized)") {
//  Model model;
//  // clang-format off
//  std::vector<uint8_t> data = {
//    255, 255,
//    12, 1,
//    30, 2,
//    0, 255};
//  // clang-format on
//
//  addAttributeToModel(
//      model,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      2,
//      2,
//      2,
//      data);
//  size_t AttributeIndex = model.Attributes.size() - 1;
//
//  ExtensionModelExtStructuralMetadata& metadata =
//      model.addExtension<ExtensionModelExtStructuralMetadata>();
//
//  Schema& schema = metadata.schema.emplace();
//  Class& testClass = schema.classes["TestClass"];
//  ClassProperty& testClassProperty =
//  testClass.properties["TestClassProperty"]; testClassProperty.type =
//  ClassProperty::Type::VEC2; testClassProperty.componentType =
//  ClassProperty::ComponentType::INT8; testClassProperty.normalized = true;
//
//  PropertyAttribute& propertyAttribute =
//  metadata.propertyAttributes.emplace_back(); propertyAttribute.classProperty
//  = "TestClass";
//
//  PropertyAttributeProperty& propertyAttributeProperty =
//      propertyAttribute.properties["TestClassProperty"];
//  propertyAttributeProperty.index = static_cast<int32_t>(AttributeIndex);
//  propertyAttributeProperty.texCoord = 0;
//  propertyAttributeProperty.channels = {0, 1};
//
//  PropertyAttributeView view(model, propertyAttribute);
//  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);
//
//  const ClassProperty* classProperty =
//      view.getClassProperty("TestClassProperty");
//  REQUIRE(classProperty);
//  REQUIRE(classProperty->type == ClassProperty::Type::VEC2);
//  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT8);
//  REQUIRE(classProperty->count == std::nullopt);
//  REQUIRE(!classProperty->array);
//  REQUIRE(classProperty->normalized);
//
//  std::vector<glm::i8vec2> expected{
//      glm::i8vec2(-1, -1),
//      glm::i8vec2(12, 1),
//      glm::i8vec2(30, 2),
//      glm::i8vec2(0, -1)};
//
//  std::vector<glm::dvec2> texCoords{
//      glm::dvec2(0, 0),
//      glm::dvec2(0.5, 0),
//      glm::dvec2(0, 0.5),
//      glm::dvec2(0.5, 0.5)};
//
//  uint32_t invokedCallbackCount = 0;
//  view.getPropertyView(
//      "TestClassProperty",
//      [&expected, &texCoords, &invokedCallbackCount](
//          const std::string& /*propertyName*/,
//          auto propertyValue) mutable {
//        invokedCallbackCount++;
//        if constexpr (std::is_same_v<
//                          PropertyAttributePropertyView<glm::i8vec2, true>,
//                          decltype(propertyValue)>) {
//          REQUIRE(
//              propertyValue.status() ==
//              PropertyAttributePropertyViewStatus::Valid);
//
//          for (size_t i = 0; i < expected.size(); ++i) {
//            glm::dvec2& uv = texCoords[i];
//            REQUIRE(propertyValue.getRaw(uv[0], uv[1]) == expected[i]);
//            REQUIRE(propertyValue.get(uv[0], uv[1]) ==
//            normalize(expected[i]));
//          }
//        } else {
//          FAIL("getPropertyView returned PropertyAttributePropertyView of "
//               "incorrect type for TestClassProperty.");
//        }
//      });
//
//  REQUIRE(invokedCallbackCount == 1);
//}
//
// TEST_CASE("Test callback for array PropertyAttributeProperty") {
//  Model model;
//  // clang-format off
//  std::vector<uint8_t> data = {
//    254, 0, 253, 1,
//    10, 2, 40, 3,
//    30, 0, 0, 2,
//    10, 2, 255, 4};
//  // clang-format on
//
//  addAttributeToModel(
//      model,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      2,
//      2,
//      4,
//      data);
//  size_t AttributeIndex = model.Attributes.size() - 1;
//
//  ExtensionModelExtStructuralMetadata& metadata =
//      model.addExtension<ExtensionModelExtStructuralMetadata>();
//
//  Schema& schema = metadata.schema.emplace();
//  Class& testClass = schema.classes["TestClass"];
//  ClassProperty& testClassProperty =
//  testClass.properties["TestClassProperty"]; testClassProperty.type =
//  ClassProperty::Type::SCALAR; testClassProperty.componentType =
//  ClassProperty::ComponentType::UINT16; testClassProperty.array = true;
//  testClassProperty.count = 2;
//
//  PropertyAttribute& propertyAttribute =
//  metadata.propertyAttributes.emplace_back(); propertyAttribute.classProperty
//  = "TestClass";
//
//  PropertyAttributeProperty& propertyAttributeProperty =
//      propertyAttribute.properties["TestClassProperty"];
//  propertyAttributeProperty.index = static_cast<int32_t>(AttributeIndex);
//  propertyAttributeProperty.texCoord = 0;
//  propertyAttributeProperty.channels = {0, 1, 2, 3};
//
//  PropertyAttributeView view(model, propertyAttribute);
//  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);
//
//  const ClassProperty* classProperty =
//      view.getClassProperty("TestClassProperty");
//  REQUIRE(classProperty);
//  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
//  REQUIRE(classProperty->componentType ==
//  ClassProperty::ComponentType::UINT16); REQUIRE(classProperty->array);
//  REQUIRE(classProperty->count == 2);
//  REQUIRE(!classProperty->normalized);
//
//  std::vector<std::vector<uint16_t>> expected{
//      {254, 509},
//      {522, 808},
//      {30, 512},
//      {522, 1279}};
//
//  std::vector<glm::dvec2> texCoords{
//      glm::dvec2(0, 0),
//      glm::dvec2(0.5, 0),
//      glm::dvec2(0, 0.5),
//      glm::dvec2(0.5, 0.5)};
//
//  uint32_t invokedCallbackCount = 0;
//  view.getPropertyView(
//      "TestClassProperty",
//      [&expected, &texCoords, &invokedCallbackCount](
//          const std::string& /*propertyName*/,
//          auto propertyValue) mutable {
//        invokedCallbackCount++;
//        if constexpr (std::is_same_v<
//                          PropertyAttributePropertyView<
//                              PropertyArrayView<uint16_t>>,
//                          decltype(propertyValue)>) {
//          REQUIRE(
//              propertyValue.status() ==
//              PropertyAttributePropertyViewStatus::Valid);
//
//          for (size_t i = 0; i < expected.size(); ++i) {
//            std::vector<uint16_t>& expectedArray = expected[i];
//            glm::dvec2& uv = texCoords[i];
//            PropertyArrayView<uint16_t> array =
//                propertyValue.getRaw(uv[0], uv[1]);
//
//            REQUIRE(static_cast<size_t>(array.size()) ==
//            expectedArray.size()); for (int64_t j = 0; j < array.size(); j++)
//            {
//              REQUIRE(array[j] == expectedArray[static_cast<size_t>(j)]);
//            }
//
//            auto maybeArray = propertyValue.get(uv[0], uv[1]);
//            REQUIRE(maybeArray);
//            REQUIRE(
//                static_cast<size_t>(maybeArray->size()) ==
//                expectedArray.size());
//            for (int64_t j = 0; j < array.size(); j++) {
//              REQUIRE(
//                  (*maybeArray)[j] == expectedArray[static_cast<size_t>(j)]);
//            }
//          }
//        } else {
//          FAIL("getPropertyView returned PropertyAttributePropertyView of "
//               "incorrect type for TestClassProperty.");
//        }
//      });
//
//  REQUIRE(invokedCallbackCount == 1);
//}
//
// TEST_CASE("Test callback for array PropertyAttributeProperty (normalized)") {
//  Model model;
//  // clang-format off
//  std::vector<uint8_t> data = {
//    254, 0, 253, 1,
//    10, 2, 40, 3,
//    30, 0, 0, 2,
//    10, 2, 255, 4};
//  // clang-format on
//
//  addAttributeToModel(
//      model,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      2,
//      2,
//      4,
//      data);
//  size_t AttributeIndex = model.Attributes.size() - 1;
//
//  ExtensionModelExtStructuralMetadata& metadata =
//      model.addExtension<ExtensionModelExtStructuralMetadata>();
//
//  Schema& schema = metadata.schema.emplace();
//  Class& testClass = schema.classes["TestClass"];
//  ClassProperty& testClassProperty =
//  testClass.properties["TestClassProperty"]; testClassProperty.type =
//  ClassProperty::Type::SCALAR; testClassProperty.componentType =
//  ClassProperty::ComponentType::UINT16; testClassProperty.array = true;
//  testClassProperty.count = 2;
//  testClassProperty.normalized = true;
//
//  PropertyAttribute& propertyAttribute =
//  metadata.propertyAttributes.emplace_back(); propertyAttribute.classProperty
//  = "TestClass";
//
//  PropertyAttributeProperty& propertyAttributeProperty =
//      propertyAttribute.properties["TestClassProperty"];
//  propertyAttributeProperty.index = static_cast<int32_t>(AttributeIndex);
//  propertyAttributeProperty.texCoord = 0;
//  propertyAttributeProperty.channels = {0, 1, 2, 3};
//
//  PropertyAttributeView view(model, propertyAttribute);
//  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);
//
//  const ClassProperty* classProperty =
//      view.getClassProperty("TestClassProperty");
//  REQUIRE(classProperty);
//  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
//  REQUIRE(classProperty->componentType ==
//  ClassProperty::ComponentType::UINT16); REQUIRE(classProperty->array);
//  REQUIRE(classProperty->count == 2);
//  REQUIRE(classProperty->normalized);
//
//  std::vector<std::vector<uint16_t>> expected{
//      {254, 509},
//      {522, 808},
//      {30, 512},
//      {522, 1279}};
//
//  std::vector<glm::dvec2> texCoords{
//      glm::dvec2(0, 0),
//      glm::dvec2(0.5, 0),
//      glm::dvec2(0, 0.5),
//      glm::dvec2(0.5, 0.5)};
//
//  uint32_t invokedCallbackCount = 0;
//  view.getPropertyView(
//      "TestClassProperty",
//      [&expected, &texCoords, &invokedCallbackCount](
//          const std::string& /*propertyName*/,
//          auto propertyValue) mutable {
//        invokedCallbackCount++;
//        if constexpr (
//            std::is_same_v<
//                PropertyAttributePropertyView<PropertyArrayView<uint16_t>,
//                true>, decltype(propertyValue)>) {
//          REQUIRE(
//              propertyValue.status() ==
//              PropertyAttributePropertyViewStatus::Valid);
//
//          for (size_t i = 0; i < expected.size(); ++i) {
//            std::vector<uint16_t>& expectedArray = expected[i];
//            glm::dvec2& uv = texCoords[i];
//            PropertyArrayView<uint16_t> array =
//                propertyValue.getRaw(uv[0], uv[1]);
//
//            REQUIRE(static_cast<size_t>(array.size()) ==
//            expectedArray.size()); for (int64_t j = 0; j < array.size(); j++)
//            {
//              REQUIRE(array[j] == expectedArray[static_cast<size_t>(j)]);
//            }
//
//            auto maybeArray = propertyValue.get(uv[0], uv[1]);
//            REQUIRE(maybeArray);
//            REQUIRE(
//                static_cast<size_t>(maybeArray->size()) ==
//                expectedArray.size());
//            for (int64_t j = 0; j < array.size(); j++) {
//              auto rawValue = expectedArray[static_cast<size_t>(j)];
//              REQUIRE((*maybeArray)[j] == normalize(rawValue));
//            }
//          }
//        } else {
//          FAIL("getPropertyView returned PropertyAttributePropertyView of "
//               "incorrect type for TestClassProperty.");
//        }
//      });
//
//  REQUIRE(invokedCallbackCount == 1);
//}
//
// TEST_CASE("Test callback on unsupported PropertyAttributeProperty") {
//  Model model;
//  // clang-format off
//  std::vector<uint8_t> data = {
//    254, 0, 253, 1,
//    10, 2, 40, 3,
//    30, 0, 0, 2,
//    10, 2, 255, 4};
//  // clang-format on
//
//  addAttributeToModel(
//      model,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      Sampler::WrapS::CLAMP_TO_EDGE,
//      2,
//      1,
//      8,
//      data);
//  size_t AttributeIndex = model.Attributes.size() - 1;
//
//  ExtensionModelExtStructuralMetadata& metadata =
//      model.addExtension<ExtensionModelExtStructuralMetadata>();
//
//  Schema& schema = metadata.schema.emplace();
//  Class& testClass = schema.classes["TestClass"];
//
//  ClassProperty& doubleClassProperty =
//      testClass.properties["DoubleClassProperty"];
//  doubleClassProperty.type = ClassProperty::Type::SCALAR;
//  doubleClassProperty.componentType = ClassProperty::ComponentType::FLOAT64;
//
//  ClassProperty& arrayClassProperty =
//      testClass.properties["ArrayClassProperty"];
//  arrayClassProperty.type = ClassProperty::Type::VEC4;
//  arrayClassProperty.componentType = ClassProperty::ComponentType::UINT8;
//  arrayClassProperty.array = true;
//  arrayClassProperty.count = 2;
//
//  PropertyAttribute& propertyAttribute =
//  metadata.propertyAttributes.emplace_back(); propertyAttribute.classProperty
//  = "TestClass";
//
//  PropertyAttributeProperty& doubleProperty =
//      propertyAttribute.properties["DoubleClassProperty"];
//  doubleProperty.index = static_cast<int32_t>(AttributeIndex);
//  doubleProperty.texCoord = 0;
//  doubleProperty.channels = {0, 1, 2, 3, 4, 5, 6, 7};
//
//  PropertyAttributeProperty& arrayProperty =
//      propertyAttribute.properties["ArrayClassProperty"];
//  arrayProperty.index = static_cast<int32_t>(AttributeIndex);
//  arrayProperty.texCoord = 0;
//  arrayProperty.channels = {0, 1, 2, 3, 4, 5, 6, 7};
//
//  PropertyAttributeView view(model, propertyAttribute);
//  REQUIRE(view.status() == PropertyAttributeViewStatus::Valid);
//
//  const ClassProperty* classProperty =
//      view.getClassProperty("DoubleClassProperty");
//  REQUIRE(classProperty);
//  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
//  REQUIRE(
//      classProperty->componentType == ClassProperty::ComponentType::FLOAT64);
//  REQUIRE(!classProperty->array);
//
//  classProperty = view.getClassProperty("ArrayClassProperty");
//  REQUIRE(classProperty);
//  REQUIRE(classProperty->type == ClassProperty::Type::VEC4);
//  REQUIRE(classProperty->componentType ==
//  ClassProperty::ComponentType::UINT8); REQUIRE(classProperty->array);
//  REQUIRE(classProperty->count == 2);
//
//  uint32_t invokedCallbackCount = 0;
//  view.getPropertyView(
//      "DoubleClassProperty",
//      [&invokedCallbackCount](
//          const std::string& /*propertyName*/,
//          auto propertyValue) mutable {
//        invokedCallbackCount++;
//        REQUIRE(
//            propertyValue.status() ==
//            PropertyAttributePropertyViewStatus::ErrorUnsupportedProperty);
//      });
//  REQUIRE(invokedCallbackCount == 1);
//
//  view.getPropertyView(
//      "ArrayClassProperty",
//      [&invokedCallbackCount](
//          const std::string& /*propertyName*/,
//          auto propertyValue) mutable {
//        invokedCallbackCount++;
//        REQUIRE(
//            propertyValue.status() ==
//            PropertyAttributePropertyViewStatus::ErrorUnsupportedProperty);
//      });
//  REQUIRE(invokedCallbackCount == 2);
//}
