#include <CesiumGltf/Class.h>
#include <CesiumGltf/ClassProperty.h>
#include <CesiumGltf/ExtensionKhrTextureTransform.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltf/Image.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/PropertyArrayView.h>
#include <CesiumGltf/PropertyTexture.h>
#include <CesiumGltf/PropertyTextureProperty.h>
#include <CesiumGltf/PropertyTexturePropertyView.h>
#include <CesiumGltf/PropertyTextureView.h>
#include <CesiumGltf/PropertyTransformations.h>
#include <CesiumGltf/Sampler.h>
#include <CesiumGltf/Schema.h>
#include <CesiumGltf/Texture.h>
#include <CesiumGltf/TextureView.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>
#include <glm/ext/vector_int2_sized.hpp>
#include <glm/ext/vector_uint2_sized.hpp>
#include <glm/ext/vector_uint3_sized.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <vector>

using namespace CesiumGltf;

namespace {
void addTextureToModel(
    Model& model,
    int32_t wrapS,
    int32_t wrapT,
    int32_t width,
    int32_t height,
    int32_t channels,
    const std::vector<uint8_t>& data) {
  Image& image = model.images.emplace_back();
  image.pAsset.emplace();
  image.pAsset->width = width;
  image.pAsset->height = height;
  image.pAsset->channels = channels;
  image.pAsset->bytesPerChannel = 1;

  std::vector<std::byte>& imageData = image.pAsset->pixelData;
  imageData.resize(data.size());
  std::memcpy(imageData.data(), data.data(), data.size());

  Sampler& sampler = model.samplers.emplace_back();
  sampler.wrapS = wrapS;
  sampler.wrapT = wrapT;

  Texture& texture = model.textures.emplace_back();
  texture.sampler = static_cast<int32_t>(model.samplers.size() - 1);
  texture.source = static_cast<int32_t>(model.images.size() - 1);
}

template <typename T, bool Normalized>
void verifyTextureTransformConstruction(
    const PropertyTexturePropertyView<T, Normalized>& propertyView,
    const ExtensionKhrTextureTransform& extension) {
  auto textureTransform = propertyView.getTextureTransform();
  REQUIRE(textureTransform != std::nullopt);
  REQUIRE(
      textureTransform->offset() ==
      glm::dvec2{extension.offset[0], extension.offset[1]});
  REQUIRE(textureTransform->rotation() == extension.rotation);
  REQUIRE(
      textureTransform->scale() ==
      glm::dvec2{extension.scale[0], extension.scale[1]});

  // Texcoord is overridden by value in KHR_texture_transform.
  if (extension.texCoord) {
    REQUIRE(
        propertyView.getTexCoordSetIndex() ==
        textureTransform->getTexCoordSetIndex());
    REQUIRE(textureTransform->getTexCoordSetIndex() == *extension.texCoord);
  }
}
} // namespace

TEST_CASE("Test PropertyTextureView on model without EXT_structural_metadata "
          "extension") {
  Model model;

  // Create an erroneously isolated property texture.
  PropertyTexture propertyTexture;
  propertyTexture.classProperty = "TestClass";

  PropertyTextureProperty& propertyTextureProperty =
      propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = 0;
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0};

  PropertyTextureView view(model, propertyTexture);
  REQUIRE(
      view.status() ==
      PropertyTextureViewStatus::ErrorMissingMetadataExtension);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(!classProperty);
}

TEST_CASE("Test PropertyTextureView on model without metadata schema") {
  Model model;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  PropertyTexture& propertyTexture = metadata.propertyTextures.emplace_back();
  propertyTexture.classProperty = "TestClass";

  PropertyTextureProperty& propertyTextureProperty =
      propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = 0;
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0};

  PropertyTextureView view(model, propertyTexture);
  REQUIRE(view.status() == PropertyTextureViewStatus::ErrorMissingSchema);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(!classProperty);
}

TEST_CASE("Test property texture with nonexistent class") {
  Model model;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT8;

  PropertyTexture& propertyTexture = metadata.propertyTextures.emplace_back();
  propertyTexture.classProperty = "I Don't Exist";

  PropertyTextureProperty& propertyTextureProperty =
      propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = 0;
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0};

  PropertyTextureView view(model, propertyTexture);
  REQUIRE(view.status() == PropertyTextureViewStatus::ErrorClassNotFound);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(!classProperty);
}

TEST_CASE("Test scalar PropertyTextureProperty") {
  Model model;
  std::vector<uint8_t> data = {12, 34, 30, 11};

  addTextureToModel(
      model,
      Sampler::WrapS::REPEAT,
      Sampler::WrapT::REPEAT,
      2,
      2,
      1,
      data);
  size_t textureIndex = model.textures.size() - 1;
  size_t imageIndex = model.images.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT8;

  PropertyTexture& propertyTexture = metadata.propertyTextures.emplace_back();
  propertyTexture.classProperty = "TestClass";

  PropertyTextureProperty& propertyTextureProperty =
      propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = static_cast<int32_t>(textureIndex);
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0};

  PropertyTextureView view(model, propertyTexture);
  REQUIRE(view.status() == PropertyTextureViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT8);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(!classProperty->normalized);

  SUBCASE("Access correct type") {
    PropertyTexturePropertyView<uint8_t> uint8Property =
        view.getPropertyView<uint8_t>("TestClassProperty");
    REQUIRE(uint8Property.status() == PropertyTexturePropertyViewStatus::Valid);

    std::vector<glm::dvec2> texCoords{
        glm::dvec2(0, 0),
        glm::dvec2(0.5, 0),
        glm::dvec2(0, 0.5),
        glm::dvec2(0.5, 0.5)};

    for (size_t i = 0; i < texCoords.size(); ++i) {
      glm::dvec2 uv = texCoords[i];
      REQUIRE(uint8Property.getRaw(uv[0], uv[1]) == data[i]);
      REQUIRE(uint8Property.get(uv[0], uv[1]) == data[i]);
    }
  }

  SUBCASE("Access with KHR_texture_transform") {
    TextureViewOptions options;
    options.applyKhrTextureTransformExtension = true;

    ExtensionKhrTextureTransform& extension =
        propertyTextureProperty.addExtension<ExtensionKhrTextureTransform>();
    extension.offset = {0.5, -0.5};
    extension.rotation = CesiumUtility::Math::PiOverTwo;
    extension.scale = {0.5, 0.5};
    extension.texCoord = 10;

    PropertyTexturePropertyView<uint8_t> uint8Property =
        view.getPropertyView<uint8_t>("TestClassProperty", options);
    REQUIRE(uint8Property.status() == PropertyTexturePropertyViewStatus::Valid);

    verifyTextureTransformConstruction(uint8Property, extension);

    // This transforms to the following UV values:
    // (0, 0) -> (0.5, -0.5) -> wraps to (0.5, 0.5)
    // (1, 0) -> (0.5, -1) -> wraps to (0.5, 0)
    // (0, 1) -> (1, -0.5) -> wraps to (0, 0.5)
    // (1, 1) -> (1, -1) -> wraps to (0.0, 0.0)
    std::vector<glm::dvec2> texCoords{
        glm::dvec2(0, 0),
        glm::dvec2(1, 0),
        glm::dvec2(0, 1),
        glm::dvec2(1, 1)};

    std::vector<uint8_t> expected{data[3], data[1], data[2], data[0]};
    for (size_t i = 0; i < texCoords.size(); ++i) {
      glm::dvec2 uv = texCoords[i];
      REQUIRE(uint8Property.getRaw(uv[0], uv[1]) == expected[i]);
      REQUIRE(uint8Property.get(uv[0], uv[1]) == expected[i]);
    }

    propertyTextureProperty.extensions.clear();
  }

  SUBCASE("Access with image copy") {
    TextureViewOptions options;
    options.makeImageCopy = true;

    PropertyTexturePropertyView<uint8_t> uint8Property =
        view.getPropertyView<uint8_t>("TestClassProperty", options);
    REQUIRE(uint8Property.status() == PropertyTexturePropertyViewStatus::Valid);

    // Clear the original image data.
    std::vector<std::byte> emptyData;
    model.images[model.images.size() - 1].pAsset->pixelData.swap(emptyData);

    std::vector<glm::dvec2> texCoords{
        glm::dvec2(0, 0),
        glm::dvec2(0.5, 0),
        glm::dvec2(0, 0.5),
        glm::dvec2(0.5, 0.5)};

    for (size_t i = 0; i < texCoords.size(); ++i) {
      glm::dvec2 uv = texCoords[i];
      REQUIRE(uint8Property.getRaw(uv[0], uv[1]) == data[i]);
      REQUIRE(uint8Property.get(uv[0], uv[1]) == data[i]);
    }
  }

  SUBCASE("Access wrong type") {
    PropertyTexturePropertyView<glm::u8vec2> u8vec2Invalid =
        view.getPropertyView<glm::u8vec2>("TestClassProperty");
    REQUIRE(
        u8vec2Invalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Access wrong component type") {
    PropertyTexturePropertyView<uint16_t> uint16Invalid =
        view.getPropertyView<uint16_t>("TestClassProperty");
    REQUIRE(
        uint16Invalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorComponentTypeMismatch);

    PropertyTexturePropertyView<int32_t> int32Invalid =
        view.getPropertyView<int32_t>("TestClassProperty");
    REQUIRE(
        int32Invalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorComponentTypeMismatch);

    PropertyTexturePropertyView<float> uint64Invalid =
        view.getPropertyView<float>("TestClassProperty");
    REQUIRE(
        uint64Invalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Access incorrectly as array") {
    PropertyTexturePropertyView<PropertyArrayView<uint8_t>> arrayInvalid =
        view.getPropertyView<PropertyArrayView<uint8_t>>("TestClassProperty");
    REQUIRE(
        arrayInvalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Access incorrectly as normalized") {
    PropertyTexturePropertyView<uint8_t, true> normalizedInvalid =
        view.getPropertyView<uint8_t, true>("TestClassProperty");
    REQUIRE(
        normalizedInvalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Channel and type mismatch") {
    model.images[imageIndex].pAsset->channels = 2;
    propertyTextureProperty.channels = {0, 1};
    PropertyTexturePropertyView<uint8_t> uint8Property =
        view.getPropertyView<uint8_t>("TestClassProperty");
    REQUIRE(
        uint8Property.status() ==
        PropertyTexturePropertyViewStatus::ErrorChannelsAndTypeMismatch);
  }

  SUBCASE("Invalid channel values") {
    propertyTextureProperty.channels = {5};
    PropertyTexturePropertyView<uint8_t> uint8Property =
        view.getPropertyView<uint8_t>("TestClassProperty");
    REQUIRE(
        uint8Property.status() ==
        PropertyTexturePropertyViewStatus::ErrorInvalidChannels);
  }

  SUBCASE("Zero channel values") {
    propertyTextureProperty.channels.clear();
    PropertyTexturePropertyView<uint8_t> uint8Property =
        view.getPropertyView<uint8_t>("TestClassProperty");
    REQUIRE(
        uint8Property.status() ==
        PropertyTexturePropertyViewStatus::ErrorInvalidChannels);
  }

  SUBCASE("Invalid bytes per channel") {
    model.images[imageIndex].pAsset->bytesPerChannel = 2;
    PropertyTexturePropertyView<uint8_t> uint8Property =
        view.getPropertyView<uint8_t>("TestClassProperty");
    REQUIRE(
        uint8Property.status() ==
        PropertyTexturePropertyViewStatus::ErrorInvalidBytesPerChannel);
  }

  SUBCASE("Empty image") {
    model.images[imageIndex].pAsset->width = 0;
    PropertyTexturePropertyView<uint8_t> uint8Property =
        view.getPropertyView<uint8_t>("TestClassProperty");
    REQUIRE(
        uint8Property.status() ==
        PropertyTexturePropertyViewStatus::ErrorEmptyImage);
  }

  SUBCASE("Wrong image index") {
    model.textures[textureIndex].source = 1;
    PropertyTexturePropertyView<uint8_t> uint8Property =
        view.getPropertyView<uint8_t>("TestClassProperty");
    REQUIRE(
        uint8Property.status() ==
        PropertyTexturePropertyViewStatus::ErrorInvalidImage);
  }

  SUBCASE("Wrong sampler index") {
    model.textures[textureIndex].sampler = 1;
    PropertyTexturePropertyView<uint8_t> uint8Property =
        view.getPropertyView<uint8_t>("TestClassProperty");
    REQUIRE(
        uint8Property.status() ==
        PropertyTexturePropertyViewStatus::ErrorInvalidSampler);
  }

  SUBCASE("Wrong texture index") {
    propertyTextureProperty.index = 1;
    PropertyTexturePropertyView<uint8_t> uint8Property =
        view.getPropertyView<uint8_t>("TestClassProperty");
    REQUIRE(
        uint8Property.status() ==
        PropertyTexturePropertyViewStatus::ErrorInvalidTexture);
  }
}

TEST_CASE("Test scalar PropertyTextureProperty (normalized)") {
  Model model;
  std::vector<uint8_t> data = {12, 34, 30, 11};

  addTextureToModel(
      model,
      Sampler::WrapS::REPEAT,
      Sampler::WrapT::REPEAT,
      2,
      2,
      1,
      data);
  size_t textureIndex = model.textures.size() - 1;
  size_t imageIndex = model.images.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT8;
  testClassProperty.normalized = true;

  PropertyTexture& propertyTexture = metadata.propertyTextures.emplace_back();
  propertyTexture.classProperty = "TestClass";

  PropertyTextureProperty& propertyTextureProperty =
      propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = static_cast<int32_t>(textureIndex);
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0};

  PropertyTextureView view(model, propertyTexture);
  REQUIRE(view.status() == PropertyTextureViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT8);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(classProperty->normalized);

  SUBCASE("Access correct type") {
    PropertyTexturePropertyView<uint8_t, true> uint8Property =
        view.getPropertyView<uint8_t, true>("TestClassProperty");
    REQUIRE(uint8Property.status() == PropertyTexturePropertyViewStatus::Valid);

    std::vector<glm::dvec2> texCoords{
        glm::dvec2(0, 0),
        glm::dvec2(0.5, 0),
        glm::dvec2(0, 0.5),
        glm::dvec2(0.5, 0.5)};

    for (size_t i = 0; i < texCoords.size(); ++i) {
      glm::dvec2 uv = texCoords[i];
      REQUIRE(uint8Property.getRaw(uv[0], uv[1]) == data[i]);
      REQUIRE(uint8Property.get(uv[0], uv[1]) == normalize(data[i]));
    }
  }

  SUBCASE("Access with KHR_texture_transform") {
    TextureViewOptions options;
    options.applyKhrTextureTransformExtension = true;

    ExtensionKhrTextureTransform& extension =
        propertyTextureProperty.addExtension<ExtensionKhrTextureTransform>();
    extension.offset = {0.5, -0.5};
    extension.rotation = CesiumUtility::Math::PiOverTwo;
    extension.scale = {0.5, 0.5};
    extension.texCoord = 10;

    PropertyTexturePropertyView<uint8_t, true> uint8Property =
        view.getPropertyView<uint8_t, true>("TestClassProperty", options);
    REQUIRE(uint8Property.status() == PropertyTexturePropertyViewStatus::Valid);

    verifyTextureTransformConstruction(uint8Property, extension);

    // This transforms to the following UV values:
    // (0, 0) -> (0.5, -0.5) -> wraps to (0.5, 0.5)
    // (1, 0) -> (0.5, -1) -> wraps to (0.5, 0)
    // (0, 1) -> (1, -0.5) -> wraps to (0, 0.5)
    // (1, 1) -> (1, -1) -> wraps to (0.0, 0.0)
    std::vector<glm::dvec2> texCoords{
        glm::dvec2(0, 0),
        glm::dvec2(1, 0),
        glm::dvec2(0, 1),
        glm::dvec2(1, 1)};

    std::vector<uint8_t> expected{data[3], data[1], data[2], data[0]};
    for (size_t i = 0; i < texCoords.size(); ++i) {
      glm::dvec2 uv = texCoords[i];
      REQUIRE(uint8Property.getRaw(uv[0], uv[1]) == expected[i]);
      REQUIRE(uint8Property.get(uv[0], uv[1]) == normalize(expected[i]));
    }

    propertyTextureProperty.extensions.clear();
  }

  SUBCASE("Access with image copy") {
    TextureViewOptions options;
    options.makeImageCopy = true;

    PropertyTexturePropertyView<uint8_t, true> uint8Property =
        view.getPropertyView<uint8_t, true>("TestClassProperty", options);
    REQUIRE(uint8Property.status() == PropertyTexturePropertyViewStatus::Valid);

    // Clear the original image data.
    std::vector<std::byte> emptyData;
    model.images[model.images.size() - 1].pAsset->pixelData.swap(emptyData);

    std::vector<glm::dvec2> texCoords{
        glm::dvec2(0, 0),
        glm::dvec2(0.5, 0),
        glm::dvec2(0, 0.5),
        glm::dvec2(0.5, 0.5)};

    for (size_t i = 0; i < texCoords.size(); ++i) {
      glm::dvec2 uv = texCoords[i];
      REQUIRE(uint8Property.getRaw(uv[0], uv[1]) == data[i]);
      REQUIRE(uint8Property.get(uv[0], uv[1]) == normalize(data[i]));
    }
  }

  SUBCASE("Access wrong type") {
    PropertyTexturePropertyView<glm::u8vec2, true> u8vec2Invalid =
        view.getPropertyView<glm::u8vec2, true>("TestClassProperty");
    REQUIRE(
        u8vec2Invalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Access wrong component type") {
    PropertyTexturePropertyView<uint16_t, true> uint16Invalid =
        view.getPropertyView<uint16_t, true>("TestClassProperty");
    REQUIRE(
        uint16Invalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorComponentTypeMismatch);

    PropertyTexturePropertyView<int32_t> int32Invalid =
        view.getPropertyView<int32_t>("TestClassProperty");
    REQUIRE(
        int32Invalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Access incorrectly as array") {
    PropertyTexturePropertyView<PropertyArrayView<uint8_t>, true> arrayInvalid =
        view.getPropertyView<PropertyArrayView<uint8_t>, true>(
            "TestClassProperty");
    REQUIRE(
        arrayInvalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Access incorrectly as non-normalized") {
    PropertyTexturePropertyView<uint8_t> normalizedInvalid =
        view.getPropertyView<uint8_t>("TestClassProperty");
    REQUIRE(
        normalizedInvalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Access incorrectly as double") {
    PropertyTexturePropertyView<double> doubleInvalid =
        view.getPropertyView<double>("TestClassProperty");
    REQUIRE(
        doubleInvalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Channel and type mismatch") {
    model.images[imageIndex].pAsset->channels = 2;
    propertyTextureProperty.channels = {0, 1};
    PropertyTexturePropertyView<uint8_t, true> uint8Property =
        view.getPropertyView<uint8_t, true>("TestClassProperty");
    REQUIRE(
        uint8Property.status() ==
        PropertyTexturePropertyViewStatus::ErrorChannelsAndTypeMismatch);
  }
}

TEST_CASE("Test vecN PropertyTextureProperty") {
  Model model;
  std::vector<uint8_t> data = {12, 34, 10, 3, 40, 0, 30, 11};
  std::vector<glm::u8vec2> expected{
      glm::u8vec2(data[0], data[1]),
      glm::u8vec2(data[2], data[3]),
      glm::u8vec2(data[4], data[5]),
      glm::u8vec2(data[6], data[7])};

  addTextureToModel(
      model,
      Sampler::WrapS::REPEAT,
      Sampler::WrapT::REPEAT,
      2,
      2,
      2,
      data);
  size_t textureIndex = model.textures.size() - 1;
  size_t imageIndex = model.images.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::VEC2;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT8;

  PropertyTexture& propertyTexture = metadata.propertyTextures.emplace_back();
  propertyTexture.classProperty = "TestClass";

  PropertyTextureProperty& propertyTextureProperty =
      propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = static_cast<int32_t>(textureIndex);
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0, 1};

  PropertyTextureView view(model, propertyTexture);
  REQUIRE(view.status() == PropertyTextureViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::VEC2);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT8);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(!classProperty->normalized);

  SUBCASE("Access correct type") {
    PropertyTexturePropertyView<glm::u8vec2> u8vec2Property =
        view.getPropertyView<glm::u8vec2>("TestClassProperty");
    REQUIRE(
        u8vec2Property.status() == PropertyTexturePropertyViewStatus::Valid);

    std::vector<glm::dvec2> texCoords{
        glm::dvec2(0, 0),
        glm::dvec2(0.5, 0),
        glm::dvec2(0, 0.5),
        glm::dvec2(0.5, 0.5)};

    for (size_t i = 0; i < texCoords.size(); ++i) {
      glm::dvec2 uv = texCoords[i];
      REQUIRE(u8vec2Property.getRaw(uv[0], uv[1]) == expected[i]);
      REQUIRE(u8vec2Property.get(uv[0], uv[1]) == expected[i]);
    }
  }

  SUBCASE("Access with KHR_texture_transform") {
    TextureViewOptions options;
    options.applyKhrTextureTransformExtension = true;

    ExtensionKhrTextureTransform& extension =
        propertyTextureProperty.addExtension<ExtensionKhrTextureTransform>();
    extension.offset = {0.5, -0.5};
    extension.rotation = CesiumUtility::Math::PiOverTwo;
    extension.scale = {0.5, 0.5};
    extension.texCoord = 10;

    PropertyTexturePropertyView<glm::u8vec2> u8vec2Property =
        view.getPropertyView<glm::u8vec2>("TestClassProperty", options);
    REQUIRE(
        u8vec2Property.status() == PropertyTexturePropertyViewStatus::Valid);

    verifyTextureTransformConstruction(u8vec2Property, extension);

    // This transforms to the following UV values:
    // (0, 0) -> (0.5, -0.5) -> wraps to (0.5, 0.5)
    // (1, 0) -> (0.5, -1) -> wraps to (0.5, 0)
    // (0, 1) -> (1, -0.5) -> wraps to (0, 0.5)
    // (1, 1) -> (1, -1) -> wraps to (0.0, 0.0)
    std::vector<glm::dvec2> texCoords{
        glm::dvec2(0, 0),
        glm::dvec2(1, 0),
        glm::dvec2(0, 1),
        glm::dvec2(1, 1)};

    std::vector<glm::u8vec2> expectedTransformed{
        expected[3],
        expected[1],
        expected[2],
        expected[0]};
    for (size_t i = 0; i < texCoords.size(); ++i) {
      glm::dvec2 uv = texCoords[i];
      REQUIRE(u8vec2Property.getRaw(uv[0], uv[1]) == expectedTransformed[i]);
      REQUIRE(u8vec2Property.get(uv[0], uv[1]) == expectedTransformed[i]);
    }

    propertyTextureProperty.extensions.clear();
  }

  SUBCASE("Access with image copy") {
    TextureViewOptions options;
    options.makeImageCopy = true;

    PropertyTexturePropertyView<glm::u8vec2> u8vec2Property =
        view.getPropertyView<glm::u8vec2>("TestClassProperty", options);
    REQUIRE(
        u8vec2Property.status() == PropertyTexturePropertyViewStatus::Valid);

    // Clear the original image data.
    std::vector<std::byte> emptyData;
    model.images[model.images.size() - 1].pAsset->pixelData.swap(emptyData);

    std::vector<glm::dvec2> texCoords{
        glm::dvec2(0, 0),
        glm::dvec2(0.5, 0),
        glm::dvec2(0, 0.5),
        glm::dvec2(0.5, 0.5)};

    for (size_t i = 0; i < texCoords.size(); ++i) {
      glm::dvec2 uv = texCoords[i];
      REQUIRE(u8vec2Property.getRaw(uv[0], uv[1]) == expected[i]);
      REQUIRE(u8vec2Property.get(uv[0], uv[1]) == expected[i]);
    }
  }

  SUBCASE("Access wrong type") {
    PropertyTexturePropertyView<uint8_t> uint8Invalid =
        view.getPropertyView<uint8_t>("TestClassProperty");
    REQUIRE(
        uint8Invalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorTypeMismatch);

    PropertyTexturePropertyView<glm::u8vec3> u8vec3Invalid =
        view.getPropertyView<glm::u8vec3>("TestClassProperty");
    REQUIRE(
        u8vec3Invalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Access wrong component type") {
    PropertyTexturePropertyView<glm::u16vec2> u16vec2Invalid =
        view.getPropertyView<glm::u16vec2>("TestClassProperty");
    REQUIRE(
        u16vec2Invalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorComponentTypeMismatch);

    PropertyTexturePropertyView<glm::i8vec2> i8vec2Invalid =
        view.getPropertyView<glm::i8vec2>("TestClassProperty");
    REQUIRE(
        i8vec2Invalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Access incorrectly as array") {
    PropertyTexturePropertyView<PropertyArrayView<glm::u8vec2>> arrayInvalid =
        view.getPropertyView<PropertyArrayView<glm::u8vec2>>(
            "TestClassProperty");
    REQUIRE(
        arrayInvalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Access incorrectly as normalized") {
    PropertyTexturePropertyView<glm::u8vec2, true> normalizedInvalid =
        view.getPropertyView<glm::u8vec2, true>("TestClassProperty");
    REQUIRE(
        normalizedInvalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Channel and type mismatch") {
    model.images[imageIndex].pAsset->channels = 4;
    propertyTextureProperty.channels = {0, 1, 2, 3};
    PropertyTexturePropertyView<glm::u8vec2> u8vec2Property =
        view.getPropertyView<glm::u8vec2>("TestClassProperty");
    REQUIRE(
        u8vec2Property.status() ==
        PropertyTexturePropertyViewStatus::ErrorChannelsAndTypeMismatch);
  }

  SUBCASE("Invalid channel values") {
    propertyTextureProperty.channels = {0, 4};
    PropertyTexturePropertyView<glm::u8vec2> u8vec2Property =
        view.getPropertyView<glm::u8vec2>("TestClassProperty");
    REQUIRE(
        u8vec2Property.status() ==
        PropertyTexturePropertyViewStatus::ErrorInvalidChannels);
  }

  SUBCASE("Invalid bytes per channel") {
    model.images[imageIndex].pAsset->bytesPerChannel = 2;
    PropertyTexturePropertyView<glm::u8vec2> u8vec2Property =
        view.getPropertyView<glm::u8vec2>("TestClassProperty");
    REQUIRE(
        u8vec2Property.status() ==
        PropertyTexturePropertyViewStatus::ErrorInvalidBytesPerChannel);
  }
}

TEST_CASE("Test vecN PropertyTextureProperty (normalized)") {
  Model model;
  std::vector<uint8_t> data = {12, 34, 10, 3, 40, 0, 30, 11};
  std::vector<glm::u8vec2> expected{
      glm::u8vec2(data[0], data[1]),
      glm::u8vec2(data[2], data[3]),
      glm::u8vec2(data[4], data[5]),
      glm::u8vec2(data[6], data[7])};

  addTextureToModel(
      model,
      Sampler::WrapS::REPEAT,
      Sampler::WrapT::REPEAT,
      2,
      2,
      2,
      data);
  size_t textureIndex = model.textures.size() - 1;
  size_t imageIndex = model.images.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::VEC2;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT8;
  testClassProperty.normalized = true;

  PropertyTexture& propertyTexture = metadata.propertyTextures.emplace_back();
  propertyTexture.classProperty = "TestClass";

  PropertyTextureProperty& propertyTextureProperty =
      propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = static_cast<int32_t>(textureIndex);
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0, 1};

  PropertyTextureView view(model, propertyTexture);
  REQUIRE(view.status() == PropertyTextureViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::VEC2);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT8);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(classProperty->normalized);

  SUBCASE("Access correct type") {
    PropertyTexturePropertyView<glm::u8vec2, true> u8vec2Property =
        view.getPropertyView<glm::u8vec2, true>("TestClassProperty");
    REQUIRE(
        u8vec2Property.status() == PropertyTexturePropertyViewStatus::Valid);

    std::vector<glm::dvec2> texCoords{
        glm::dvec2(0, 0),
        glm::dvec2(0.5, 0),
        glm::dvec2(0, 0.5),
        glm::dvec2(0.5, 0.5)};
    for (size_t i = 0; i < texCoords.size(); ++i) {
      glm::dvec2 uv = texCoords[i];
      REQUIRE(u8vec2Property.getRaw(uv[0], uv[1]) == expected[i]);
      REQUIRE(u8vec2Property.get(uv[0], uv[1]) == normalize(expected[i]));
    }
  }

  SUBCASE("Access with KHR_texture_transform") {
    TextureViewOptions options;
    options.applyKhrTextureTransformExtension = true;

    ExtensionKhrTextureTransform& extension =
        propertyTextureProperty.addExtension<ExtensionKhrTextureTransform>();
    extension.offset = {0.5, -0.5};
    extension.rotation = CesiumUtility::Math::PiOverTwo;
    extension.scale = {0.5, 0.5};
    extension.texCoord = 10;

    PropertyTexturePropertyView<glm::u8vec2, true> u8vec2Property =
        view.getPropertyView<glm::u8vec2, true>("TestClassProperty", options);
    REQUIRE(
        u8vec2Property.status() == PropertyTexturePropertyViewStatus::Valid);

    verifyTextureTransformConstruction(u8vec2Property, extension);

    // This transforms to the following UV values:
    // (0, 0) -> (0.5, -0.5) -> wraps to (0.5, 0.5)
    // (1, 0) -> (0.5, -1) -> wraps to (0.5, 0)
    // (0, 1) -> (1, -0.5) -> wraps to (0, 0.5)
    // (1, 1) -> (1, -1) -> wraps to (0.0, 0.0)
    std::vector<glm::dvec2> texCoords{
        glm::dvec2(0, 0),
        glm::dvec2(1, 0),
        glm::dvec2(0, 1),
        glm::dvec2(1, 1)};

    std::vector<glm::u8vec2> expectedTransformed{
        expected[3],
        expected[1],
        expected[2],
        expected[0]};
    for (size_t i = 0; i < texCoords.size(); ++i) {
      glm::dvec2 uv = texCoords[i];
      REQUIRE(u8vec2Property.getRaw(uv[0], uv[1]) == expectedTransformed[i]);
      REQUIRE(
          u8vec2Property.get(uv[0], uv[1]) ==
          normalize(expectedTransformed[i]));
    }

    propertyTextureProperty.extensions.clear();
  }

  SUBCASE("Access with image copy") {
    TextureViewOptions options;
    options.makeImageCopy = true;

    PropertyTexturePropertyView<glm::u8vec2, true> u8vec2Property =
        view.getPropertyView<glm::u8vec2, true>("TestClassProperty", options);
    REQUIRE(
        u8vec2Property.status() == PropertyTexturePropertyViewStatus::Valid);

    // Clear the original image data.
    std::vector<std::byte> emptyData;
    model.images[model.images.size() - 1].pAsset->pixelData.swap(emptyData);

    std::vector<glm::dvec2> texCoords{
        glm::dvec2(0, 0),
        glm::dvec2(0.5, 0),
        glm::dvec2(0, 0.5),
        glm::dvec2(0.5, 0.5)};

    for (size_t i = 0; i < texCoords.size(); ++i) {
      glm::dvec2 uv = texCoords[i];
      REQUIRE(u8vec2Property.getRaw(uv[0], uv[1]) == expected[i]);
      REQUIRE(u8vec2Property.get(uv[0], uv[1]) == normalize(expected[i]));
    }
  }

  SUBCASE("Access wrong type") {
    PropertyTexturePropertyView<uint8_t, true> uint8Invalid =
        view.getPropertyView<uint8_t, true>("TestClassProperty");
    REQUIRE(
        uint8Invalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorTypeMismatch);

    PropertyTexturePropertyView<glm::u8vec3, true> u8vec3Invalid =
        view.getPropertyView<glm::u8vec3, true>("TestClassProperty");
    REQUIRE(
        u8vec3Invalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Access wrong component type") {
    PropertyTexturePropertyView<glm::u16vec2, true> u16vec2Invalid =
        view.getPropertyView<glm::u16vec2, true>("TestClassProperty");
    REQUIRE(
        u16vec2Invalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorComponentTypeMismatch);

    PropertyTexturePropertyView<glm::i8vec2, true> i8vec2Invalid =
        view.getPropertyView<glm::i8vec2, true>("TestClassProperty");
    REQUIRE(
        i8vec2Invalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Access incorrectly as array") {
    PropertyTexturePropertyView<PropertyArrayView<glm::u8vec2>, true>
        arrayInvalid =
            view.getPropertyView<PropertyArrayView<glm::u8vec2>, true>(
                "TestClassProperty");
    REQUIRE(
        arrayInvalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Access incorrectly as non-normalized") {
    PropertyTexturePropertyView<glm::u8vec2> normalizedInvalid =
        view.getPropertyView<glm::u8vec2>("TestClassProperty");
    REQUIRE(
        normalizedInvalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Access incorrectly as dvec2") {
    PropertyTexturePropertyView<glm::dvec2> dvec2Invalid =
        view.getPropertyView<glm::dvec2>("TestClassProperty");
    REQUIRE(
        dvec2Invalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Channel and type mismatch") {
    model.images[imageIndex].pAsset->channels = 4;
    propertyTextureProperty.channels = {0, 1, 2, 3};
    PropertyTexturePropertyView<glm::u8vec2, true> u8vec2Property =
        view.getPropertyView<glm::u8vec2, true>("TestClassProperty");
    REQUIRE(
        u8vec2Property.status() ==
        PropertyTexturePropertyViewStatus::ErrorChannelsAndTypeMismatch);
  }
}

TEST_CASE("Test array PropertyTextureProperty") {
  Model model;
  // clang-format off
  std::vector<uint8_t> data = {
    12, 34, 10,
    40, 0, 30,
    80, 4, 2,
    6, 3, 4,
  };
  // clang-format on
  std::vector<std::array<uint8_t, 3>> expected = {
      {data[0], data[1], data[2]},
      {data[3], data[4], data[5]},
      {data[6], data[7], data[8]},
      {data[9], data[10], data[11]},
  };

  addTextureToModel(
      model,
      Sampler::WrapS::REPEAT,
      Sampler::WrapT::REPEAT,
      2,
      2,
      3,
      data);
  size_t textureIndex = model.textures.size() - 1;
  size_t imageIndex = model.images.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT8;
  testClassProperty.array = true;
  testClassProperty.count = 3;

  PropertyTexture& propertyTexture = metadata.propertyTextures.emplace_back();
  propertyTexture.classProperty = "TestClass";

  PropertyTextureProperty& propertyTextureProperty =
      propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = static_cast<int32_t>(textureIndex);
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0, 1, 2};

  PropertyTextureView view(model, propertyTexture);
  REQUIRE(view.status() == PropertyTextureViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT8);
  REQUIRE(classProperty->array);
  REQUIRE(classProperty->count == 3);
  REQUIRE(!classProperty->normalized);

  SUBCASE("Access correct type") {
    PropertyTexturePropertyView<PropertyArrayView<uint8_t>> uint8ArrayProperty =
        view.getPropertyView<PropertyArrayView<uint8_t>>("TestClassProperty");
    REQUIRE(
        uint8ArrayProperty.status() ==
        PropertyTexturePropertyViewStatus::Valid);

    std::vector<glm::dvec2> texCoords{
        glm::dvec2(0, 0),
        glm::dvec2(0.5, 0),
        glm::dvec2(0, 0.5),
        glm::dvec2(0.5, 0.5)};

    for (size_t i = 0; i < texCoords.size(); ++i) {
      glm::dvec2 uv = texCoords[i];
      const std::array<uint8_t, 3>& expectedArray = expected[i];

      PropertyArrayCopy<uint8_t> value =
          uint8ArrayProperty.getRaw(uv[0], uv[1]);
      REQUIRE(static_cast<size_t>(value.size()) == expectedArray.size());

      for (int64_t j = 0; j < value.size(); j++) {
        REQUIRE(value[j] == expectedArray[static_cast<size_t>(j)]);
      }

      auto maybeValue = uint8ArrayProperty.get(uv[0], uv[1]);
      REQUIRE(maybeValue);
      for (int64_t j = 0; j < maybeValue->size(); j++) {
        REQUIRE((*maybeValue)[j] == value[j]);
      }
    }
  }

  SUBCASE("Access with KHR_texture_transform") {
    TextureViewOptions options;
    options.applyKhrTextureTransformExtension = true;

    ExtensionKhrTextureTransform& extension =
        propertyTextureProperty.addExtension<ExtensionKhrTextureTransform>();
    extension.offset = {0.5, -0.5};
    extension.rotation = CesiumUtility::Math::PiOverTwo;
    extension.scale = {0.5, 0.5};
    extension.texCoord = 10;

    PropertyTexturePropertyView<PropertyArrayView<uint8_t>> uint8ArrayProperty =
        view.getPropertyView<PropertyArrayView<uint8_t>>(
            "TestClassProperty",
            options);
    REQUIRE(
        uint8ArrayProperty.status() ==
        PropertyTexturePropertyViewStatus::Valid);

    verifyTextureTransformConstruction(uint8ArrayProperty, extension);

    // This transforms to the following UV values:
    // (0, 0) -> (0.5, -0.5) -> wraps to (0.5, 0.5)
    // (1, 0) -> (0.5, -1) -> wraps to (0.5, 0)
    // (0, 1) -> (1, -0.5) -> wraps to (0, 0.5)
    // (1, 1) -> (1, -1) -> wraps to (0.0, 0.0)
    std::vector<glm::dvec2> texCoords{
        glm::dvec2(0, 0),
        glm::dvec2(1, 0),
        glm::dvec2(0, 1),
        glm::dvec2(1, 1)};

    std::vector<std::array<uint8_t, 3>> expectedTransformed{
        expected[3],
        expected[1],
        expected[2],
        expected[0]};

    for (size_t i = 0; i < texCoords.size(); ++i) {
      glm::dvec2 uv = texCoords[i];
      const std::array<uint8_t, 3>& expectedArray = expectedTransformed[i];

      PropertyArrayCopy<uint8_t> value =
          uint8ArrayProperty.getRaw(uv[0], uv[1]);
      REQUIRE(static_cast<size_t>(value.size()) == expectedArray.size());

      for (int64_t j = 0; j < value.size(); j++) {
        REQUIRE(value[j] == expectedArray[static_cast<size_t>(j)]);
      }

      std::optional<PropertyArrayCopy<uint8_t>> maybeValue =
          uint8ArrayProperty.get(uv[0], uv[1]);
      REQUIRE(maybeValue);
      for (int64_t j = 0; j < maybeValue->size(); j++) {
        REQUIRE((*maybeValue)[j] == value[j]);
      }
    }

    propertyTextureProperty.extensions.clear();
  }

  SUBCASE("Access with image copy") {
    TextureViewOptions options;
    options.makeImageCopy = true;

    PropertyTexturePropertyView<PropertyArrayView<uint8_t>> uint8ArrayProperty =
        view.getPropertyView<PropertyArrayView<uint8_t>>(
            "TestClassProperty",
            options);
    REQUIRE(
        uint8ArrayProperty.status() ==
        PropertyTexturePropertyViewStatus::Valid);

    // Clear the original image data.
    std::vector<std::byte> emptyData;
    model.images[model.images.size() - 1].pAsset->pixelData.swap(emptyData);

    std::vector<glm::dvec2> texCoords{
        glm::dvec2(0, 0),
        glm::dvec2(0.5, 0),
        glm::dvec2(0, 0.5),
        glm::dvec2(0.5, 0.5)};

    for (size_t i = 0; i < texCoords.size(); ++i) {
      glm::dvec2 uv = texCoords[i];
      const std::array<uint8_t, 3>& expectedArray = expected[i];

      PropertyArrayCopy<uint8_t> value =
          uint8ArrayProperty.getRaw(uv[0], uv[1]);
      REQUIRE(static_cast<size_t>(value.size()) == expectedArray.size());

      for (int64_t j = 0; j < value.size(); j++) {
        REQUIRE(value[j] == expectedArray[static_cast<size_t>(j)]);
      }

      std::optional<PropertyArrayCopy<uint8_t>> maybeValue =
          uint8ArrayProperty.get(uv[0], uv[1]);
      REQUIRE(maybeValue);
      for (int64_t j = 0; j < maybeValue->size(); j++) {
        REQUIRE((*maybeValue)[j] == value[j]);
      }
    }
  }

  SUBCASE("Access wrong component type") {
    PropertyTexturePropertyView<PropertyArrayView<int8_t>> int8ArrayInvalid =
        view.getPropertyView<PropertyArrayView<int8_t>>("TestClassProperty");
    REQUIRE(
        int8ArrayInvalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorComponentTypeMismatch);

    PropertyTexturePropertyView<PropertyArrayView<uint16_t>>
        uint16ArrayInvalid = view.getPropertyView<PropertyArrayView<uint16_t>>(
            "TestClassProperty");
    REQUIRE(
        uint16ArrayInvalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Access incorrectly as non-array") {
    PropertyTexturePropertyView<uint8_t> uint8Invalid =
        view.getPropertyView<uint8_t>("TestClassProperty");
    REQUIRE(
        uint8Invalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorArrayTypeMismatch);

    PropertyTexturePropertyView<glm::u8vec3> u8vec3Invalid =
        view.getPropertyView<glm::u8vec3>("TestClassProperty");
    REQUIRE(
        u8vec3Invalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Access incorrectly as normalized") {
    PropertyTexturePropertyView<PropertyArrayView<uint8_t>, true>
        normalizedInvalid =
            view.getPropertyView<PropertyArrayView<uint8_t>, true>(
                "TestClassProperty");
    REQUIRE(
        normalizedInvalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Channel and type mismatch") {
    model.images[imageIndex].pAsset->channels = 4;
    propertyTextureProperty.channels = {0, 1, 2, 3};
    PropertyTexturePropertyView<PropertyArrayView<uint8_t>> uint8ArrayProperty =
        view.getPropertyView<PropertyArrayView<uint8_t>>("TestClassProperty");
    REQUIRE(
        uint8ArrayProperty.status() ==
        PropertyTexturePropertyViewStatus::ErrorChannelsAndTypeMismatch);
  }

  SUBCASE("Invalid channel values") {
    propertyTextureProperty.channels = {0, 4, 1};
    PropertyTexturePropertyView<PropertyArrayView<uint8_t>> uint8ArrayProperty =
        view.getPropertyView<PropertyArrayView<uint8_t>>("TestClassProperty");
    REQUIRE(
        uint8ArrayProperty.status() ==
        PropertyTexturePropertyViewStatus::ErrorInvalidChannels);
  }

  SUBCASE("Invalid bytes per channel") {
    model.images[imageIndex].pAsset->bytesPerChannel = 2;
    PropertyTexturePropertyView<PropertyArrayView<uint8_t>> uint8ArrayProperty =
        view.getPropertyView<PropertyArrayView<uint8_t>>("TestClassProperty");
    REQUIRE(
        uint8ArrayProperty.status() ==
        PropertyTexturePropertyViewStatus::ErrorInvalidBytesPerChannel);
  }
}

TEST_CASE("Test array PropertyTextureProperty (normalized)") {
  Model model;
  // clang-format off
  std::vector<uint8_t> data = {
    12, 34, 10,
    40, 0, 30,
    80, 4, 2,
    6, 3, 4,
  };
  // clang-format on

  std::vector<std::array<uint8_t, 3>> expected = {
      {data[0], data[1], data[2]},
      {data[3], data[4], data[5]},
      {data[6], data[7], data[8]},
      {data[9], data[10], data[11]},
  };

  addTextureToModel(
      model,
      Sampler::WrapS::REPEAT,
      Sampler::WrapT::REPEAT,
      2,
      2,
      3,
      data);
  size_t textureIndex = model.textures.size() - 1;
  size_t imageIndex = model.images.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT8;
  testClassProperty.array = true;
  testClassProperty.count = 3;
  testClassProperty.normalized = true;

  PropertyTexture& propertyTexture = metadata.propertyTextures.emplace_back();
  propertyTexture.classProperty = "TestClass";

  PropertyTextureProperty& propertyTextureProperty =
      propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = static_cast<int32_t>(textureIndex);
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0, 1, 2};

  PropertyTextureView view(model, propertyTexture);
  REQUIRE(view.status() == PropertyTextureViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT8);
  REQUIRE(classProperty->array);
  REQUIRE(classProperty->count == 3);
  REQUIRE(classProperty->normalized);

  SUBCASE("Access correct type") {
    PropertyTexturePropertyView<PropertyArrayView<uint8_t>, true>
        uint8ArrayProperty =
            view.getPropertyView<PropertyArrayView<uint8_t>, true>(
                "TestClassProperty");
    REQUIRE(
        uint8ArrayProperty.status() ==
        PropertyTexturePropertyViewStatus::Valid);

    std::vector<glm::dvec2> texCoords{
        glm::dvec2(0, 0),
        glm::dvec2(0.5, 0),
        glm::dvec2(0, 0.5),
        glm::dvec2(0.5, 0.5)};

    for (size_t i = 0; i < texCoords.size(); ++i) {
      glm::dvec2 uv = texCoords[i];
      const std::array<uint8_t, 3>& expectedArray = expected[i];

      PropertyArrayCopy<uint8_t> value =
          uint8ArrayProperty.getRaw(uv[0], uv[1]);
      REQUIRE(static_cast<size_t>(value.size()) == expectedArray.size());
      for (int64_t j = 0; j < value.size(); j++) {
        REQUIRE(value[j] == expectedArray[static_cast<size_t>(j)]);
      }

      std::optional<PropertyArrayCopy<double>> maybeValue =
          uint8ArrayProperty.get(uv[0], uv[1]);
      REQUIRE(maybeValue);
      for (int64_t j = 0; j < maybeValue->size(); j++) {
        REQUIRE((*maybeValue)[j] == normalize(value[j]));
      }
    }
  }

  SUBCASE("Access with KHR_texture_transform") {
    TextureViewOptions options;
    options.applyKhrTextureTransformExtension = true;

    ExtensionKhrTextureTransform& extension =
        propertyTextureProperty.addExtension<ExtensionKhrTextureTransform>();
    extension.offset = {0.5, -0.5};
    extension.rotation = CesiumUtility::Math::PiOverTwo;
    extension.scale = {0.5, 0.5};
    extension.texCoord = 10;

    PropertyTexturePropertyView<PropertyArrayView<uint8_t>, true>
        uint8ArrayProperty =
            view.getPropertyView<PropertyArrayView<uint8_t>, true>(
                "TestClassProperty",
                options);
    REQUIRE(
        uint8ArrayProperty.status() ==
        PropertyTexturePropertyViewStatus::Valid);

    verifyTextureTransformConstruction(uint8ArrayProperty, extension);

    // This transforms to the following UV values:
    // (0, 0) -> (0.5, -0.5) -> wraps to (0.5, 0.5)
    // (1, 0) -> (0.5, -1) -> wraps to (0.5, 0)
    // (0, 1) -> (1, -0.5) -> wraps to (0, 0.5)
    // (1, 1) -> (1, -1) -> wraps to (0.0, 0.0)
    std::vector<glm::dvec2> texCoords{
        glm::dvec2(0, 0),
        glm::dvec2(1, 0),
        glm::dvec2(0, 1),
        glm::dvec2(1, 1)};

    std::vector<std::array<uint8_t, 3>> expectedTransformed{
        expected[3],
        expected[1],
        expected[2],
        expected[0]};

    for (size_t i = 0; i < texCoords.size(); ++i) {
      glm::dvec2 uv = texCoords[i];
      const std::array<uint8_t, 3>& expectedArray = expectedTransformed[i];

      PropertyArrayCopy<uint8_t> value =
          uint8ArrayProperty.getRaw(uv[0], uv[1]);
      REQUIRE(static_cast<size_t>(value.size()) == expectedArray.size());

      for (int64_t j = 0; j < value.size(); j++) {
        REQUIRE(value[j] == expectedArray[static_cast<size_t>(j)]);
      }

      std::optional<PropertyArrayCopy<double>> maybeValue =
          uint8ArrayProperty.get(uv[0], uv[1]);
      REQUIRE(maybeValue);
      for (int64_t j = 0; j < maybeValue->size(); j++) {
        REQUIRE((*maybeValue)[j] == normalize(value[j]));
      }
    }

    propertyTextureProperty.extensions.clear();
  }

  SUBCASE("Access with image copy") {
    TextureViewOptions options;
    options.makeImageCopy = true;

    PropertyTexturePropertyView<PropertyArrayView<uint8_t>, true>
        uint8ArrayProperty =
            view.getPropertyView<PropertyArrayView<uint8_t>, true>(
                "TestClassProperty",
                options);
    REQUIRE(
        uint8ArrayProperty.status() ==
        PropertyTexturePropertyViewStatus::Valid);

    // Clear the original image data.
    std::vector<std::byte> emptyData;
    model.images[model.images.size() - 1].pAsset->pixelData.swap(emptyData);

    std::vector<glm::dvec2> texCoords{
        glm::dvec2(0, 0),
        glm::dvec2(0.5, 0),
        glm::dvec2(0, 0.5),
        glm::dvec2(0.5, 0.5)};

    for (size_t i = 0; i < texCoords.size(); ++i) {
      glm::dvec2 uv = texCoords[i];
      const std::array<uint8_t, 3>& expectedArray = expected[i];

      PropertyArrayCopy<uint8_t> value =
          uint8ArrayProperty.getRaw(uv[0], uv[1]);
      REQUIRE(static_cast<size_t>(value.size()) == expectedArray.size());

      for (int64_t j = 0; j < value.size(); j++) {
        REQUIRE(value[j] == expectedArray[static_cast<size_t>(j)]);
      }

      std::optional<PropertyArrayCopy<double>> maybeValue =
          uint8ArrayProperty.get(uv[0], uv[1]);
      REQUIRE(maybeValue);
      for (int64_t j = 0; j < maybeValue->size(); j++) {
        REQUIRE((*maybeValue)[j] == normalize(value[j]));
      }
    }
  }

  SUBCASE("Access wrong component type") {
    PropertyTexturePropertyView<PropertyArrayView<int8_t>, true>
        int8ArrayInvalid =
            view.getPropertyView<PropertyArrayView<int8_t>, true>(
                "TestClassProperty");
    REQUIRE(
        int8ArrayInvalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorComponentTypeMismatch);

    PropertyTexturePropertyView<PropertyArrayView<uint16_t>, true>
        uint16ArrayInvalid =
            view.getPropertyView<PropertyArrayView<uint16_t>, true>(
                "TestClassProperty");
    REQUIRE(
        uint16ArrayInvalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Access incorrectly as non-array") {
    PropertyTexturePropertyView<uint8_t, true> uint8Invalid =
        view.getPropertyView<uint8_t, true>("TestClassProperty");
    REQUIRE(
        uint8Invalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorArrayTypeMismatch);

    PropertyTexturePropertyView<glm::u8vec3, true> u8vec3Invalid =
        view.getPropertyView<glm::u8vec3, true>("TestClassProperty");
    REQUIRE(
        u8vec3Invalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorArrayTypeMismatch);
  }

  SUBCASE("Access incorrectly as normalized") {
    PropertyTexturePropertyView<PropertyArrayView<uint8_t>> normalizedInvalid =
        view.getPropertyView<PropertyArrayView<uint8_t>>("TestClassProperty");
    REQUIRE(
        normalizedInvalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Channel and type mismatch") {
    model.images[imageIndex].pAsset->channels = 4;
    propertyTextureProperty.channels = {0, 1, 2, 3};
    PropertyTexturePropertyView<PropertyArrayView<uint8_t>, true>
        uint8ArrayProperty =
            view.getPropertyView<PropertyArrayView<uint8_t>, true>(
                "TestClassProperty");
    REQUIRE(
        uint8ArrayProperty.status() ==
        PropertyTexturePropertyViewStatus::ErrorChannelsAndTypeMismatch);
  }
}

TEST_CASE("Test with PropertyTextureProperty offset, scale, min, max") {
  Model model;
  // clang-format off
    std::vector<uint8_t> data{
      0, 0, 0, 1,
      9, 0, 1, 0,
      20, 2, 2, 0,
      8, 1, 0, 1};
  // clang-format on

  std::vector<uint32_t> expectedUint{16777216, 65545, 131604, 16777480};

  const float offset = 1.0f;
  const float scale = 2.0f;
  const float min = -10.0f;
  const float max = 10.0f;

  addTextureToModel(
      model,
      Sampler::WrapS::CLAMP_TO_EDGE,
      Sampler::WrapS::CLAMP_TO_EDGE,
      2,
      2,
      4,
      data);
  size_t textureIndex = model.textures.size() - 1;

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

  PropertyTexture& propertyTexture = metadata.propertyTextures.emplace_back();
  propertyTexture.classProperty = "TestClass";

  PropertyTextureProperty& propertyTextureProperty =
      propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = static_cast<int32_t>(textureIndex);
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0, 1, 2, 3};

  PropertyTextureView view(model, propertyTexture);
  REQUIRE(view.status() == PropertyTextureViewStatus::Valid);

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

  std::vector<glm::dvec2> texCoords{
      glm::dvec2(0, 0),
      glm::dvec2(0.5, 0),
      glm::dvec2(0, 0.5),
      glm::dvec2(0.5, 0.5)};

  SUBCASE("Use class property values") {
    PropertyTexturePropertyView<float> property =
        view.getPropertyView<float>("TestClassProperty");
    REQUIRE(property.status() == PropertyTexturePropertyViewStatus::Valid);
    REQUIRE(property.offset() == offset);
    REQUIRE(property.scale() == scale);
    REQUIRE(property.min() == min);
    REQUIRE(property.max() == max);

    std::vector<float> expectedRaw(expectedUint.size());
    std::vector<float> expectedTransformed(expectedUint.size());
    for (size_t i = 0; i < expectedUint.size(); i++) {
      float value = *reinterpret_cast<float*>(&expectedUint[i]);
      expectedRaw[i] = value;
      expectedTransformed[i] = value * scale + offset;
    }

    for (size_t i = 0; i < texCoords.size(); ++i) {
      glm::dvec2 uv = texCoords[i];
      REQUIRE(property.getRaw(uv[0], uv[1]) == expectedRaw[i]);
      REQUIRE(property.get(uv[0], uv[1]) == expectedTransformed[i]);
    }
  }

  SUBCASE("Use own property values") {
    const float newOffset = 1.0f;
    const float newScale = -1.0f;
    const float newMin = -3.0f;
    const float newMax = 0.0f;
    propertyTextureProperty.offset = newOffset;
    propertyTextureProperty.scale = newScale;
    propertyTextureProperty.min = newMin;
    propertyTextureProperty.max = newMax;

    PropertyTexturePropertyView<float> property =
        view.getPropertyView<float>("TestClassProperty");
    REQUIRE(property.status() == PropertyTexturePropertyViewStatus::Valid);
    REQUIRE(property.offset() == newOffset);
    REQUIRE(property.scale() == newScale);
    REQUIRE(property.min() == newMin);
    REQUIRE(property.max() == newMax);

    std::vector<float> expectedRaw(expectedUint.size());
    std::vector<float> expectedTransformed(expectedUint.size());
    for (size_t i = 0; i < expectedUint.size(); i++) {
      float value = *reinterpret_cast<float*>(&expectedUint[i]);
      expectedRaw[i] = value;
      expectedTransformed[i] = value * newScale + newOffset;
    }

    for (size_t i = 0; i < texCoords.size(); ++i) {
      glm::dvec2 uv = texCoords[i];
      REQUIRE(property.getRaw(uv[0], uv[1]) == expectedRaw[i]);
      REQUIRE(property.get(uv[0], uv[1]) == expectedTransformed[i]);
    }
  }
}

TEST_CASE(
    "Test with PropertyTextureProperty offset, scale, min, max (normalized)") {
  Model model;
  std::vector<uint8_t> data = {12, 34, 30, 11};

  const double offset = 1.0;
  const double scale = 2.0;
  const double min = 1.0;
  const double max = 3.0;

  addTextureToModel(
      model,
      Sampler::WrapS::CLAMP_TO_EDGE,
      Sampler::WrapS::CLAMP_TO_EDGE,
      2,
      2,
      1,
      data);
  size_t textureIndex = model.textures.size() - 1;

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

  PropertyTexture& propertyTexture = metadata.propertyTextures.emplace_back();
  propertyTexture.classProperty = "TestClass";

  PropertyTextureProperty& propertyTextureProperty =
      propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = static_cast<int32_t>(textureIndex);
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0};

  PropertyTextureView view(model, propertyTexture);
  REQUIRE(view.status() == PropertyTextureViewStatus::Valid);

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
    PropertyTexturePropertyView<uint8_t, true> property =
        view.getPropertyView<uint8_t, true>("TestClassProperty");
    REQUIRE(property.status() == PropertyTexturePropertyViewStatus::Valid);
    REQUIRE(property.offset() == offset);
    REQUIRE(property.scale() == scale);
    REQUIRE(property.min() == min);
    REQUIRE(property.max() == max);

    for (size_t i = 0; i < texCoords.size(); ++i) {
      glm::dvec2 uv = texCoords[i];
      REQUIRE(property.getRaw(uv[0], uv[1]) == data[i]);
      REQUIRE(
          property.get(uv[0], uv[1]) == normalize(data[i]) * scale + offset);
    }
  }

  SUBCASE("Use own property values") {
    const double newOffset = 2.0;
    const double newScale = 5.0;
    const double newMin = 10.0;
    const double newMax = 11.0;
    propertyTextureProperty.offset = newOffset;
    propertyTextureProperty.scale = newScale;
    propertyTextureProperty.min = newMin;
    propertyTextureProperty.max = newMax;

    PropertyTexturePropertyView<uint8_t, true> property =
        view.getPropertyView<uint8_t, true>("TestClassProperty");
    REQUIRE(property.status() == PropertyTexturePropertyViewStatus::Valid);
    REQUIRE(property.offset() == newOffset);
    REQUIRE(property.scale() == newScale);
    REQUIRE(property.min() == newMin);
    REQUIRE(property.max() == newMax);

    for (size_t i = 0; i < texCoords.size(); ++i) {
      glm::dvec2 uv = texCoords[i];
      REQUIRE(property.getRaw(uv[0], uv[1]) == data[i]);
      REQUIRE(
          property.get(uv[0], uv[1]) ==
          normalize(data[i]) * newScale + newOffset);
    }
  }
}

TEST_CASE("Test with PropertyTextureProperty noData") {
  Model model;
  std::vector<uint8_t> data = {12, 34, 30, 11};
  const uint8_t noData = 34;

  addTextureToModel(
      model,
      Sampler::WrapS::CLAMP_TO_EDGE,
      Sampler::WrapS::CLAMP_TO_EDGE,
      2,
      2,
      1,
      data);
  size_t textureIndex = model.textures.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT8;
  testClassProperty.noData = noData;

  PropertyTexture& propertyTexture = metadata.propertyTextures.emplace_back();
  propertyTexture.classProperty = "TestClass";

  PropertyTextureProperty& propertyTextureProperty =
      propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = static_cast<int32_t>(textureIndex);
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0};

  PropertyTextureView view(model, propertyTexture);
  REQUIRE(view.status() == PropertyTextureViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT8);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(!classProperty->normalized);

  std::vector<glm::dvec2> texCoords{
      glm::dvec2(0, 0),
      glm::dvec2(0.5, 0),
      glm::dvec2(0, 0.5),
      glm::dvec2(0.5, 0.5)};

  SUBCASE("Without default value") {
    PropertyTexturePropertyView<uint8_t> property =
        view.getPropertyView<uint8_t>("TestClassProperty");
    REQUIRE(property.status() == PropertyTexturePropertyViewStatus::Valid);

    for (size_t i = 0; i < texCoords.size(); ++i) {
      glm::dvec2 uv = texCoords[i];
      auto value = property.getRaw(uv[0], uv[1]);
      REQUIRE(value == data[i]);

      auto maybeValue = property.get(uv[0], uv[1]);
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

    PropertyTexturePropertyView<uint8_t> property =
        view.getPropertyView<uint8_t>("TestClassProperty");
    REQUIRE(property.status() == PropertyTexturePropertyViewStatus::Valid);

    for (size_t i = 0; i < texCoords.size(); ++i) {
      glm::dvec2 uv = texCoords[i];
      auto value = property.getRaw(uv[0], uv[1]);
      REQUIRE(value == data[i]);

      auto maybeValue = property.get(uv[0], uv[1]);
      if (value == noData) {
        REQUIRE(maybeValue == defaultValue);
      } else {
        REQUIRE(maybeValue == data[i]);
      }
    }
  }
}

TEST_CASE("Test with PropertyTextureProperty noData (normalized)") {
  Model model;
  std::vector<uint8_t> data = {12, 34, 30, 11};
  const uint8_t noData = 34;

  addTextureToModel(
      model,
      Sampler::WrapS::CLAMP_TO_EDGE,
      Sampler::WrapS::CLAMP_TO_EDGE,
      2,
      2,
      1,
      data);
  size_t textureIndex = model.textures.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT8;
  testClassProperty.normalized = true;
  testClassProperty.noData = noData;

  PropertyTexture& propertyTexture = metadata.propertyTextures.emplace_back();
  propertyTexture.classProperty = "TestClass";

  PropertyTextureProperty& propertyTextureProperty =
      propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = static_cast<int32_t>(textureIndex);
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0};

  PropertyTextureView view(model, propertyTexture);
  REQUIRE(view.status() == PropertyTextureViewStatus::Valid);

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

  SUBCASE("Without default value") {
    PropertyTexturePropertyView<uint8_t, true> property =
        view.getPropertyView<uint8_t, true>("TestClassProperty");
    REQUIRE(property.status() == PropertyTexturePropertyViewStatus::Valid);

    for (size_t i = 0; i < texCoords.size(); ++i) {
      glm::dvec2 uv = texCoords[i];
      auto value = property.getRaw(uv[0], uv[1]);
      REQUIRE(value == data[i]);

      auto maybeValue = property.get(uv[0], uv[1]);
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

    PropertyTexturePropertyView<uint8_t, true> property =
        view.getPropertyView<uint8_t, true>("TestClassProperty");
    REQUIRE(property.status() == PropertyTexturePropertyViewStatus::Valid);

    for (size_t i = 0; i < texCoords.size(); ++i) {
      glm::dvec2 uv = texCoords[i];
      auto value = property.getRaw(uv[0], uv[1]);
      REQUIRE(value == data[i]);

      auto maybeValue = property.get(uv[0], uv[1]);
      if (value == noData) {
        REQUIRE(maybeValue == defaultValue);
      } else {
        REQUIRE(maybeValue == normalize(data[i]));
      }
    }
  }
}

TEST_CASE(
    "Test nonexistent PropertyTextureProperty with class property default") {
  Model model;
  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT8;

  const uint8_t defaultValue = 10;
  testClassProperty.defaultProperty = defaultValue;

  PropertyTexture& propertyTexture = metadata.propertyTextures.emplace_back();
  propertyTexture.classProperty = "TestClass";

  PropertyTextureView view(model, propertyTexture);
  REQUIRE(view.status() == PropertyTextureViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT8);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(!classProperty->normalized);
  REQUIRE(classProperty->defaultProperty);

  SUBCASE("Access correct type") {
    PropertyTexturePropertyView<uint8_t> uint8Property =
        view.getPropertyView<uint8_t>("TestClassProperty");
    REQUIRE(
        uint8Property.status() ==
        PropertyTexturePropertyViewStatus::EmptyPropertyWithDefault);
    REQUIRE(uint8Property.defaultValue() == defaultValue);

    std::vector<glm::dvec2> texCoords{
        glm::dvec2(0, 0),
        glm::dvec2(0.5, 0),
        glm::dvec2(0, 0.5),
        glm::dvec2(0.5, 0.5)};

    for (size_t i = 0; i < texCoords.size(); ++i) {
      glm::dvec2 uv = texCoords[i];
      REQUIRE(uint8Property.get(uv[0], uv[1]) == defaultValue);
    }
  }

  SUBCASE("Access wrong type") {
    PropertyTexturePropertyView<glm::u8vec2> u8vec2Invalid =
        view.getPropertyView<glm::u8vec2>("TestClassProperty");
    REQUIRE(
        u8vec2Invalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorTypeMismatch);
  }

  SUBCASE("Access wrong component type") {
    PropertyTexturePropertyView<uint16_t> uint16Invalid =
        view.getPropertyView<uint16_t>("TestClassProperty");
    REQUIRE(
        uint16Invalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorComponentTypeMismatch);
  }

  SUBCASE("Access incorrectly as normalized") {
    PropertyTexturePropertyView<uint8_t, true> normalizedInvalid =
        view.getPropertyView<uint8_t, true>("TestClassProperty");
    REQUIRE(
        normalizedInvalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorNormalizationMismatch);
  }

  SUBCASE("Invalid default value") {
    testClassProperty.defaultProperty = "not a number";
    PropertyTexturePropertyView<uint8_t> uint8Property =
        view.getPropertyView<uint8_t>("TestClassProperty");
    REQUIRE(
        uint8Property.status() ==
        PropertyTexturePropertyViewStatus::ErrorInvalidDefaultValue);
  }

  SUBCASE("No default value") {
    testClassProperty.defaultProperty.reset();
    PropertyTexturePropertyView<uint8_t> uint8Property =
        view.getPropertyView<uint8_t>("TestClassProperty");
    REQUIRE(
        uint8Property.status() ==
        PropertyTexturePropertyViewStatus::ErrorNonexistentProperty);
  }
}

TEST_CASE("Test callback on invalid property texture view") {
  Model model;
  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();
  metadata.schema.emplace();

  // Property texture has a nonexistent class.
  PropertyTexture& propertyTexture = metadata.propertyTextures.emplace_back();
  propertyTexture.classProperty = "TestClass";

  PropertyTextureProperty& propertyTextureProperty =
      propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = -1;

  PropertyTextureView view(model, propertyTexture);
  REQUIRE(view.status() == PropertyTextureViewStatus::ErrorClassNotFound);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(!classProperty);

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      "TestClassProperty",
      [&invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) mutable {
        invokedCallbackCount++;
        REQUIRE(
            propertyValue.status() ==
            PropertyTexturePropertyViewStatus::ErrorInvalidPropertyTexture);
      });

  REQUIRE(invokedCallbackCount == 1);
}

TEST_CASE("Test callback on invalid PropertyTextureProperty") {
  Model model;
  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["InvalidProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT8;

  PropertyTexture& propertyTexture = metadata.propertyTextures.emplace_back();
  propertyTexture.classProperty = "TestClass";

  PropertyTextureProperty& propertyTextureProperty =
      propertyTexture.properties["InvalidProperty"];
  propertyTextureProperty.index = -1;

  PropertyTextureView view(model, propertyTexture);
  REQUIRE(view.status() == PropertyTextureViewStatus::Valid);

  const ClassProperty* classProperty = view.getClassProperty("InvalidProperty");
  REQUIRE(classProperty);

  classProperty = view.getClassProperty("NonexistentProperty");
  REQUIRE(!classProperty);

  uint32_t invokedCallbackCount = 0;
  auto testCallback = [&invokedCallbackCount](
                          const std::string& /*propertyId*/,
                          auto propertyValue) mutable {
    invokedCallbackCount++;
    REQUIRE(propertyValue.status() != PropertyTexturePropertyViewStatus::Valid);
  };

  view.getPropertyView("InvalidProperty", testCallback);
  view.getPropertyView("NonexistentProperty", testCallback);

  REQUIRE(invokedCallbackCount == 2);
}

TEST_CASE("Test callback on invalid normalized PropertyTextureProperty") {
  Model model;
  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::FLOAT32;
  testClassProperty.normalized = true; // This is erroneous.

  PropertyTexture& propertyTexture = metadata.propertyTextures.emplace_back();
  propertyTexture.classProperty = "TestClass";

  PropertyTextureProperty& propertyTextureProperty =
      propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = static_cast<int32_t>(0);
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0, 1};

  PropertyTextureView view(model, propertyTexture);
  REQUIRE(view.status() == PropertyTextureViewStatus::Valid);

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
        PropertyTexturePropertyViewStatus::ErrorInvalidNormalization);
  };

  view.getPropertyView("TestClassProperty", testCallback);

  REQUIRE(invokedCallbackCount == 1);
}

TEST_CASE("Test callback for scalar PropertyTextureProperty") {
  Model model;
  std::vector<uint8_t> data = {255, 255, 12, 1, 30, 2, 0, 255};

  addTextureToModel(
      model,
      Sampler::WrapS::CLAMP_TO_EDGE,
      Sampler::WrapS::CLAMP_TO_EDGE,
      2,
      2,
      2,
      data);
  size_t textureIndex = model.textures.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::INT16;

  PropertyTexture& propertyTexture = metadata.propertyTextures.emplace_back();
  propertyTexture.classProperty = "TestClass";

  PropertyTextureProperty& propertyTextureProperty =
      propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = static_cast<int32_t>(textureIndex);
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0, 1};

  PropertyTextureView view(model, propertyTexture);
  REQUIRE(view.status() == PropertyTextureViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT16);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(!classProperty->normalized);

  std::vector<glm::dvec2> texCoords{
      glm::dvec2(0, 0),
      glm::dvec2(0.5, 0),
      glm::dvec2(0, 0.5),
      glm::dvec2(0.5, 0.5)};

  std::vector<int16_t> expected{-1, 268, 542, -256};

  SUBCASE("Works") {
    uint32_t invokedCallbackCount = 0;
    view.getPropertyView(
        "TestClassProperty",
        [&expected, &texCoords, &invokedCallbackCount](
            const std::string& /*propertyId*/,
            auto propertyValue) mutable {
          invokedCallbackCount++;
          if constexpr (std::is_same_v<
                            PropertyTexturePropertyView<int16_t>,
                            decltype(propertyValue)>) {
            REQUIRE(
                propertyValue.status() ==
                PropertyTexturePropertyViewStatus::Valid);

            for (size_t i = 0; i < expected.size(); ++i) {
              glm::dvec2& uv = texCoords[i];
              REQUIRE(propertyValue.getRaw(uv[0], uv[1]) == expected[i]);
              REQUIRE(propertyValue.get(uv[0], uv[1]) == expected[i]);
            }
          } else {
            FAIL("getPropertyView returned PropertyTexturePropertyView of "
                 "incorrect type for TestClassProperty.");
          }
        });

    REQUIRE(invokedCallbackCount == 1);
  }

  SUBCASE("Works with options") {
    TextureViewOptions options;
    options.makeImageCopy = true;

    uint32_t invokedCallbackCount = 0;
    view.getPropertyView(
        "TestClassProperty",
        [&expected, &texCoords, &invokedCallbackCount, &model](
            const std::string& /*propertyId*/,
            auto propertyValue) mutable {
          invokedCallbackCount++;
          if constexpr (std::is_same_v<
                            PropertyTexturePropertyView<int16_t>,
                            decltype(propertyValue)>) {
            REQUIRE(
                propertyValue.status() ==
                PropertyTexturePropertyViewStatus::Valid);

            // Clear the original image data.
            std::vector<std::byte> emptyData;
            model.images[model.images.size() - 1].pAsset->pixelData.swap(
                emptyData);

            for (size_t i = 0; i < expected.size(); ++i) {
              glm::dvec2& uv = texCoords[i];
              REQUIRE(propertyValue.getRaw(uv[0], uv[1]) == expected[i]);
              REQUIRE(propertyValue.get(uv[0], uv[1]) == expected[i]);
            }
          } else {
            FAIL("getPropertyView returned PropertyTexturePropertyView of "
                 "incorrect type for TestClassProperty.");
          }
        },
        options);

    REQUIRE(invokedCallbackCount == 1);
  }
}

TEST_CASE("Test callback for scalar PropertyTextureProperty (normalized)") {
  Model model;
  std::vector<uint8_t> data = {255, 255, 12, 1, 30, 2, 0, 255};

  addTextureToModel(
      model,
      Sampler::WrapS::CLAMP_TO_EDGE,
      Sampler::WrapS::CLAMP_TO_EDGE,
      2,
      2,
      2,
      data);
  size_t textureIndex = model.textures.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::INT16;
  testClassProperty.normalized = true;

  PropertyTexture& propertyTexture = metadata.propertyTextures.emplace_back();
  propertyTexture.classProperty = "TestClass";

  PropertyTextureProperty& propertyTextureProperty =
      propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = static_cast<int32_t>(textureIndex);
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0, 1};

  PropertyTextureView view(model, propertyTexture);
  REQUIRE(view.status() == PropertyTextureViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT16);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(classProperty->normalized);

  std::vector<glm::dvec2> texCoords{
      glm::dvec2(0, 0),
      glm::dvec2(0.5, 0),
      glm::dvec2(0, 0.5),
      glm::dvec2(0.5, 0.5)};

  std::vector<int16_t> expected{-1, 268, 542, -256};

  SUBCASE("Works") {
    uint32_t invokedCallbackCount = 0;
    view.getPropertyView(
        "TestClassProperty",
        [&expected, &texCoords, &invokedCallbackCount](
            const std::string& /*propertyId*/,
            auto propertyValue) mutable {
          invokedCallbackCount++;
          if constexpr (std::is_same_v<
                            PropertyTexturePropertyView<int16_t, true>,
                            decltype(propertyValue)>) {
            REQUIRE(
                propertyValue.status() ==
                PropertyTexturePropertyViewStatus::Valid);

            for (size_t i = 0; i < expected.size(); ++i) {
              glm::dvec2& uv = texCoords[i];
              REQUIRE(propertyValue.getRaw(uv[0], uv[1]) == expected[i]);
              REQUIRE(
                  propertyValue.get(uv[0], uv[1]) == normalize(expected[i]));
            }
          } else {
            FAIL("getPropertyView returned PropertyTexturePropertyView of "
                 "incorrect type for TestClassProperty.");
          }
        });

    REQUIRE(invokedCallbackCount == 1);
  }

  SUBCASE("Works with options") {
    TextureViewOptions options;
    options.makeImageCopy = true;

    uint32_t invokedCallbackCount = 0;
    view.getPropertyView(
        "TestClassProperty",
        [&expected, &texCoords, &invokedCallbackCount, &model](
            const std::string& /*propertyId*/,
            auto propertyValue) mutable {
          invokedCallbackCount++;
          if constexpr (std::is_same_v<
                            PropertyTexturePropertyView<int16_t, true>,
                            decltype(propertyValue)>) {
            REQUIRE(
                propertyValue.status() ==
                PropertyTexturePropertyViewStatus::Valid);

            // Clear the original image data.
            std::vector<std::byte> emptyData;
            model.images[model.images.size() - 1].pAsset->pixelData.swap(
                emptyData);

            for (size_t i = 0; i < expected.size(); ++i) {
              glm::dvec2& uv = texCoords[i];
              REQUIRE(propertyValue.getRaw(uv[0], uv[1]) == expected[i]);
              REQUIRE(
                  propertyValue.get(uv[0], uv[1]) == normalize(expected[i]));
            }
          } else {
            FAIL("getPropertyView returned PropertyTexturePropertyView of "
                 "incorrect type for TestClassProperty.");
          }
        },
        options);

    REQUIRE(invokedCallbackCount == 1);
  }
}

TEST_CASE("Test callback for vecN PropertyTextureProperty") {
  Model model;
  // clang-format off
  std::vector<uint8_t> data = {
    255, 255,
    12, 1,
    30, 2,
    0, 255};
  // clang-format on

  addTextureToModel(
      model,
      Sampler::WrapS::CLAMP_TO_EDGE,
      Sampler::WrapS::CLAMP_TO_EDGE,
      2,
      2,
      2,
      data);
  size_t textureIndex = model.textures.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::VEC2;
  testClassProperty.componentType = ClassProperty::ComponentType::INT8;

  PropertyTexture& propertyTexture = metadata.propertyTextures.emplace_back();
  propertyTexture.classProperty = "TestClass";

  PropertyTextureProperty& propertyTextureProperty =
      propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = static_cast<int32_t>(textureIndex);
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0, 1};

  PropertyTextureView view(model, propertyTexture);
  REQUIRE(view.status() == PropertyTextureViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::VEC2);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT8);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(!classProperty->normalized);

  std::vector<glm::i8vec2> expected{
      glm::i8vec2(-1, -1),
      glm::i8vec2(12, 1),
      glm::i8vec2(30, 2),
      glm::i8vec2(0, -1)};

  std::vector<glm::dvec2> texCoords{
      glm::dvec2(0, 0),
      glm::dvec2(0.5, 0),
      glm::dvec2(0, 0.5),
      glm::dvec2(0.5, 0.5)};

  SUBCASE("Works") {
    uint32_t invokedCallbackCount = 0;
    view.getPropertyView(
        "TestClassProperty",
        [&expected, &texCoords, &invokedCallbackCount](
            const std::string& /*propertyId*/,
            auto propertyValue) mutable {
          invokedCallbackCount++;
          if constexpr (std::is_same_v<
                            PropertyTexturePropertyView<glm::i8vec2>,
                            decltype(propertyValue)>) {
            REQUIRE(
                propertyValue.status() ==
                PropertyTexturePropertyViewStatus::Valid);

            for (size_t i = 0; i < expected.size(); ++i) {
              glm::dvec2& uv = texCoords[i];
              REQUIRE(propertyValue.getRaw(uv[0], uv[1]) == expected[i]);
              REQUIRE(propertyValue.get(uv[0], uv[1]) == expected[i]);
            }
          } else {
            FAIL("getPropertyView returned PropertyTexturePropertyView of "
                 "incorrect type for TestClassProperty.");
          }
        });

    REQUIRE(invokedCallbackCount == 1);
  }

  SUBCASE("Works with options") {
    TextureViewOptions options;
    options.makeImageCopy = true;

    uint32_t invokedCallbackCount = 0;
    view.getPropertyView(
        "TestClassProperty",
        [&expected, &texCoords, &invokedCallbackCount, &model](
            const std::string& /*propertyId*/,
            auto propertyValue) mutable {
          invokedCallbackCount++;
          if constexpr (std::is_same_v<
                            PropertyTexturePropertyView<glm::i8vec2>,
                            decltype(propertyValue)>) {
            REQUIRE(
                propertyValue.status() ==
                PropertyTexturePropertyViewStatus::Valid);

            // Clear the original image data.
            std::vector<std::byte> emptyData;
            model.images[model.images.size() - 1].pAsset->pixelData.swap(
                emptyData);

            for (size_t i = 0; i < expected.size(); ++i) {
              glm::dvec2& uv = texCoords[i];
              REQUIRE(propertyValue.getRaw(uv[0], uv[1]) == expected[i]);
              REQUIRE(propertyValue.get(uv[0], uv[1]) == expected[i]);
            }
          } else {
            FAIL("getPropertyView returned PropertyTexturePropertyView of "
                 "incorrect type for TestClassProperty.");
          }
        },
        options);

    REQUIRE(invokedCallbackCount == 1);
  }
}

TEST_CASE("Test callback for vecN PropertyTextureProperty (normalized)") {
  Model model;
  // clang-format off
  std::vector<uint8_t> data = {
    255, 255,
    12, 1,
    30, 2,
    0, 255};
  // clang-format on

  addTextureToModel(
      model,
      Sampler::WrapS::CLAMP_TO_EDGE,
      Sampler::WrapS::CLAMP_TO_EDGE,
      2,
      2,
      2,
      data);
  size_t textureIndex = model.textures.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::VEC2;
  testClassProperty.componentType = ClassProperty::ComponentType::INT8;
  testClassProperty.normalized = true;

  PropertyTexture& propertyTexture = metadata.propertyTextures.emplace_back();
  propertyTexture.classProperty = "TestClass";

  PropertyTextureProperty& propertyTextureProperty =
      propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = static_cast<int32_t>(textureIndex);
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0, 1};

  PropertyTextureView view(model, propertyTexture);
  REQUIRE(view.status() == PropertyTextureViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::VEC2);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT8);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(classProperty->normalized);

  std::vector<glm::i8vec2> expected{
      glm::i8vec2(-1, -1),
      glm::i8vec2(12, 1),
      glm::i8vec2(30, 2),
      glm::i8vec2(0, -1)};

  std::vector<glm::dvec2> texCoords{
      glm::dvec2(0, 0),
      glm::dvec2(0.5, 0),
      glm::dvec2(0, 0.5),
      glm::dvec2(0.5, 0.5)};

  SUBCASE("Works") {
    uint32_t invokedCallbackCount = 0;
    view.getPropertyView(
        "TestClassProperty",
        [&expected, &texCoords, &invokedCallbackCount](
            const std::string& /*propertyId*/,
            auto propertyValue) mutable {
          invokedCallbackCount++;
          if constexpr (std::is_same_v<
                            PropertyTexturePropertyView<glm::i8vec2, true>,
                            decltype(propertyValue)>) {
            REQUIRE(
                propertyValue.status() ==
                PropertyTexturePropertyViewStatus::Valid);

            for (size_t i = 0; i < expected.size(); ++i) {
              glm::dvec2& uv = texCoords[i];
              REQUIRE(propertyValue.getRaw(uv[0], uv[1]) == expected[i]);
              REQUIRE(
                  propertyValue.get(uv[0], uv[1]) == normalize(expected[i]));
            }
          } else {
            FAIL("getPropertyView returned PropertyTexturePropertyView of "
                 "incorrect type for TestClassProperty.");
          }
        });

    REQUIRE(invokedCallbackCount == 1);
  }

  SUBCASE("Works with options") {
    TextureViewOptions options;
    options.makeImageCopy = true;

    uint32_t invokedCallbackCount = 0;
    view.getPropertyView(
        "TestClassProperty",
        [&expected, &texCoords, &invokedCallbackCount, &model](
            const std::string& /*propertyId*/,
            auto propertyValue) mutable {
          invokedCallbackCount++;
          if constexpr (std::is_same_v<
                            PropertyTexturePropertyView<glm::i8vec2, true>,
                            decltype(propertyValue)>) {
            REQUIRE(
                propertyValue.status() ==
                PropertyTexturePropertyViewStatus::Valid);

            // Clear the original image data.
            std::vector<std::byte> emptyData;
            model.images[model.images.size() - 1].pAsset->pixelData.swap(
                emptyData);

            for (size_t i = 0; i < expected.size(); ++i) {
              glm::dvec2& uv = texCoords[i];
              REQUIRE(propertyValue.getRaw(uv[0], uv[1]) == expected[i]);
              REQUIRE(
                  propertyValue.get(uv[0], uv[1]) == normalize(expected[i]));
            }
          } else {
            FAIL("getPropertyView returned PropertyTexturePropertyView of "
                 "incorrect type for TestClassProperty.");
          }
        },
        options);

    REQUIRE(invokedCallbackCount == 1);
  }
}

TEST_CASE("Test callback for array PropertyTextureProperty") {
  Model model;
  // clang-format off
  std::vector<uint8_t> data = {
    254, 0, 253, 1,
    10, 2, 40, 3,
    30, 0, 0, 2,
    10, 2, 255, 4};
  // clang-format on

  addTextureToModel(
      model,
      Sampler::WrapS::CLAMP_TO_EDGE,
      Sampler::WrapS::CLAMP_TO_EDGE,
      2,
      2,
      4,
      data);
  size_t textureIndex = model.textures.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT16;
  testClassProperty.array = true;
  testClassProperty.count = 2;

  PropertyTexture& propertyTexture = metadata.propertyTextures.emplace_back();
  propertyTexture.classProperty = "TestClass";

  PropertyTextureProperty& propertyTextureProperty =
      propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = static_cast<int32_t>(textureIndex);
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0, 1, 2, 3};

  PropertyTextureView view(model, propertyTexture);
  REQUIRE(view.status() == PropertyTextureViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT16);
  REQUIRE(classProperty->array);
  REQUIRE(classProperty->count == 2);
  REQUIRE(!classProperty->normalized);

  std::vector<std::vector<uint16_t>> expected{
      {254, 509},
      {522, 808},
      {30, 512},
      {522, 1279}};

  std::vector<glm::dvec2> texCoords{
      glm::dvec2(0, 0),
      glm::dvec2(0.5, 0),
      glm::dvec2(0, 0.5),
      glm::dvec2(0.5, 0.5)};

  SUBCASE("Works") {
    uint32_t invokedCallbackCount = 0;
    view.getPropertyView(
        "TestClassProperty",
        [&expected, &texCoords, &invokedCallbackCount](
            const std::string& /*propertyId*/,
            auto propertyValue) mutable {
          invokedCallbackCount++;
          if constexpr (std::is_same_v<
                            PropertyTexturePropertyView<
                                PropertyArrayView<uint16_t>>,
                            decltype(propertyValue)>) {
            REQUIRE(
                propertyValue.status() ==
                PropertyTexturePropertyViewStatus::Valid);

            for (size_t i = 0; i < expected.size(); ++i) {
              std::vector<uint16_t>& expectedArray = expected[i];
              glm::dvec2& uv = texCoords[i];
              PropertyArrayCopy<uint16_t> array =
                  propertyValue.getRaw(uv[0], uv[1]);

              REQUIRE(
                  static_cast<size_t>(array.size()) == expectedArray.size());
              for (int64_t j = 0; j < array.size(); j++) {
                REQUIRE(array[j] == expectedArray[static_cast<size_t>(j)]);
              }

              std::optional<PropertyArrayCopy<uint16_t>> maybeArray =
                  propertyValue.get(uv[0], uv[1]);
              REQUIRE(maybeArray);
              REQUIRE(
                  static_cast<size_t>(maybeArray->size()) ==
                  expectedArray.size());
              for (int64_t j = 0; j < array.size(); j++) {
                REQUIRE(
                    (*maybeArray)[j] == expectedArray[static_cast<size_t>(j)]);
              }
            }
          } else {
            FAIL("getPropertyView returned PropertyTexturePropertyView of "
                 "incorrect type for TestClassProperty.");
          }
        });

    REQUIRE(invokedCallbackCount == 1);
  }

  SUBCASE("Works with options") {
    TextureViewOptions options;
    options.makeImageCopy = true;

    uint32_t invokedCallbackCount = 0;
    view.getPropertyView(
        "TestClassProperty",
        [&expected, &texCoords, &invokedCallbackCount, &model](
            const std::string& /*propertyId*/,
            auto propertyValue) mutable {
          invokedCallbackCount++;
          if constexpr (std::is_same_v<
                            PropertyTexturePropertyView<
                                PropertyArrayView<uint16_t>>,
                            decltype(propertyValue)>) {
            REQUIRE(
                propertyValue.status() ==
                PropertyTexturePropertyViewStatus::Valid);

            // Clear the original image data.
            std::vector<std::byte> emptyData;
            model.images[model.images.size() - 1].pAsset->pixelData.swap(
                emptyData);

            for (size_t i = 0; i < expected.size(); ++i) {
              std::vector<uint16_t>& expectedArray = expected[i];
              glm::dvec2& uv = texCoords[i];
              auto array = propertyValue.getRaw(uv[0], uv[1]);

              REQUIRE(
                  static_cast<size_t>(array.size()) == expectedArray.size());
              for (int64_t j = 0; j < array.size(); j++) {
                REQUIRE(array[j] == expectedArray[static_cast<size_t>(j)]);
              }

              auto maybeArray = propertyValue.get(uv[0], uv[1]);
              REQUIRE(maybeArray);
              REQUIRE(
                  static_cast<size_t>(maybeArray->size()) ==
                  expectedArray.size());
              for (int64_t j = 0; j < array.size(); j++) {
                REQUIRE(
                    (*maybeArray)[j] == expectedArray[static_cast<size_t>(j)]);
              }
            }
          } else {
            FAIL("getPropertyView returned PropertyTexturePropertyView of "
                 "incorrect type for TestClassProperty.");
          }
        },
        options);

    REQUIRE(invokedCallbackCount == 1);
  }
}

TEST_CASE("Test callback for array PropertyTextureProperty (normalized)") {
  Model model;
  // clang-format off
  std::vector<uint8_t> data = {
    254, 0, 253, 1,
    10, 2, 40, 3,
    30, 0, 0, 2,
    10, 2, 255, 4};
  // clang-format on

  addTextureToModel(
      model,
      Sampler::WrapS::CLAMP_TO_EDGE,
      Sampler::WrapS::CLAMP_TO_EDGE,
      2,
      2,
      4,
      data);
  size_t textureIndex = model.textures.size() - 1;

  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::UINT16;
  testClassProperty.array = true;
  testClassProperty.count = 2;
  testClassProperty.normalized = true;

  PropertyTexture& propertyTexture = metadata.propertyTextures.emplace_back();
  propertyTexture.classProperty = "TestClass";

  PropertyTextureProperty& propertyTextureProperty =
      propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = static_cast<int32_t>(textureIndex);
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0, 1, 2, 3};

  PropertyTextureView view(model, propertyTexture);
  REQUIRE(view.status() == PropertyTextureViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT16);
  REQUIRE(classProperty->array);
  REQUIRE(classProperty->count == 2);
  REQUIRE(classProperty->normalized);

  std::vector<std::vector<uint16_t>> expected{
      {254, 509},
      {522, 808},
      {30, 512},
      {522, 1279}};

  std::vector<glm::dvec2> texCoords{
      glm::dvec2(0, 0),
      glm::dvec2(0.5, 0),
      glm::dvec2(0, 0.5),
      glm::dvec2(0.5, 0.5)};

  SUBCASE("Works") {
    uint32_t invokedCallbackCount = 0;
    view.getPropertyView(
        "TestClassProperty",
        [&expected, &texCoords, &invokedCallbackCount](
            const std::string& /*propertyId*/,
            auto propertyValue) mutable {
          invokedCallbackCount++;
          if constexpr (std::is_same_v<
                            PropertyTexturePropertyView<
                                PropertyArrayView<uint16_t>,
                                true>,
                            decltype(propertyValue)>) {
            REQUIRE(
                propertyValue.status() ==
                PropertyTexturePropertyViewStatus::Valid);

            for (size_t i = 0; i < expected.size(); ++i) {
              std::vector<uint16_t>& expectedArray = expected[i];
              glm::dvec2& uv = texCoords[i];
              PropertyArrayCopy<uint16_t> array =
                  propertyValue.getRaw(uv[0], uv[1]);

              REQUIRE(
                  static_cast<size_t>(array.size()) == expectedArray.size());
              for (int64_t j = 0; j < array.size(); j++) {
                REQUIRE(array[j] == expectedArray[static_cast<size_t>(j)]);
              }

              auto maybeArray = propertyValue.get(uv[0], uv[1]);
              REQUIRE(maybeArray);
              REQUIRE(
                  static_cast<size_t>(maybeArray->size()) ==
                  expectedArray.size());
              for (int64_t j = 0; j < array.size(); j++) {
                auto rawValue = expectedArray[static_cast<size_t>(j)];
                REQUIRE((*maybeArray)[j] == normalize(rawValue));
              }
            }
          } else {
            FAIL("getPropertyView returned PropertyTexturePropertyView of "
                 "incorrect type for TestClassProperty.");
          }
        });

    REQUIRE(invokedCallbackCount == 1);
  }

  SUBCASE("Works with options") {
    TextureViewOptions options;
    options.makeImageCopy = true;

    uint32_t invokedCallbackCount = 0;
    view.getPropertyView(
        "TestClassProperty",
        [&expected, &texCoords, &invokedCallbackCount, &model](
            const std::string& /*propertyId*/,
            auto propertyValue) mutable {
          invokedCallbackCount++;
          if constexpr (std::is_same_v<
                            PropertyTexturePropertyView<
                                PropertyArrayView<uint16_t>,
                                true>,
                            decltype(propertyValue)>) {
            REQUIRE(
                propertyValue.status() ==
                PropertyTexturePropertyViewStatus::Valid);

            // Clear the original image data.
            std::vector<std::byte> emptyData;
            model.images[model.images.size() - 1].pAsset->pixelData.swap(
                emptyData);

            for (size_t i = 0; i < expected.size(); ++i) {
              std::vector<uint16_t>& expectedArray = expected[i];
              glm::dvec2& uv = texCoords[i];
              PropertyArrayCopy<uint16_t> array =
                  propertyValue.getRaw(uv[0], uv[1]);

              REQUIRE(
                  static_cast<size_t>(array.size()) == expectedArray.size());
              for (int64_t j = 0; j < array.size(); j++) {
                REQUIRE(array[j] == expectedArray[static_cast<size_t>(j)]);
              }

              auto maybeArray = propertyValue.get(uv[0], uv[1]);
              REQUIRE(maybeArray);
              REQUIRE(
                  static_cast<size_t>(maybeArray->size()) ==
                  expectedArray.size());
              for (int64_t j = 0; j < array.size(); j++) {
                auto rawValue = expectedArray[static_cast<size_t>(j)];
                REQUIRE((*maybeArray)[j] == normalize(rawValue));
              }
            }
          } else {
            FAIL("getPropertyView returned PropertyTexturePropertyView of "
                 "incorrect type for TestClassProperty.");
          }
        },
        options);

    REQUIRE(invokedCallbackCount == 1);
  }
}

TEST_CASE("Test callback on unsupported PropertyTextureProperty") {
  Model model;
  // clang-format off
  std::vector<uint8_t> data = {
    254, 0, 253, 1,
    10, 2, 40, 3,
    30, 0, 0, 2,
    10, 2, 255, 4};
  // clang-format on

  addTextureToModel(
      model,
      Sampler::WrapS::CLAMP_TO_EDGE,
      Sampler::WrapS::CLAMP_TO_EDGE,
      2,
      1,
      8,
      data);
  size_t textureIndex = model.textures.size() - 1;

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
  arrayClassProperty.type = ClassProperty::Type::VEC4;
  arrayClassProperty.componentType = ClassProperty::ComponentType::UINT8;
  arrayClassProperty.array = true;
  arrayClassProperty.count = 2;

  PropertyTexture& propertyTexture = metadata.propertyTextures.emplace_back();
  propertyTexture.classProperty = "TestClass";

  PropertyTextureProperty& doubleProperty =
      propertyTexture.properties["DoubleClassProperty"];
  doubleProperty.index = static_cast<int32_t>(textureIndex);
  doubleProperty.texCoord = 0;
  doubleProperty.channels = {0, 1, 2, 3, 4, 5, 6, 7};

  PropertyTextureProperty& arrayProperty =
      propertyTexture.properties["ArrayClassProperty"];
  arrayProperty.index = static_cast<int32_t>(textureIndex);
  arrayProperty.texCoord = 0;
  arrayProperty.channels = {0, 1, 2, 3, 4, 5, 6, 7};

  PropertyTextureView view(model, propertyTexture);
  REQUIRE(view.status() == PropertyTextureViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("DoubleClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(
      classProperty->componentType == ClassProperty::ComponentType::FLOAT64);
  REQUIRE(!classProperty->array);

  classProperty = view.getClassProperty("ArrayClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::VEC4);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::UINT8);
  REQUIRE(classProperty->array);
  REQUIRE(classProperty->count == 2);

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      "DoubleClassProperty",
      [&invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) mutable {
        invokedCallbackCount++;
        REQUIRE(
            propertyValue.status() ==
            PropertyTexturePropertyViewStatus::ErrorUnsupportedProperty);
      });
  REQUIRE(invokedCallbackCount == 1);

  view.getPropertyView(
      "ArrayClassProperty",
      [&invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) mutable {
        invokedCallbackCount++;
        REQUIRE(
            propertyValue.status() ==
            PropertyTexturePropertyViewStatus::ErrorUnsupportedProperty);
      });
  REQUIRE(invokedCallbackCount == 2);
}

TEST_CASE(
    "Test callback for empty PropertyTextureProperty with default value") {
  Model model;
  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty = testClass.properties["TestClassProperty"];
  testClassProperty.type = ClassProperty::Type::SCALAR;
  testClassProperty.componentType = ClassProperty::ComponentType::INT16;

  const int16_t defaultValue = 10;
  testClassProperty.defaultProperty = defaultValue;

  PropertyTexture& propertyTexture = metadata.propertyTextures.emplace_back();
  propertyTexture.classProperty = "TestClass";

  PropertyTextureView view(model, propertyTexture);
  REQUIRE(view.status() == PropertyTextureViewStatus::Valid);

  const ClassProperty* classProperty =
      view.getClassProperty("TestClassProperty");
  REQUIRE(classProperty);
  REQUIRE(classProperty->type == ClassProperty::Type::SCALAR);
  REQUIRE(classProperty->componentType == ClassProperty::ComponentType::INT16);
  REQUIRE(classProperty->count == std::nullopt);
  REQUIRE(!classProperty->array);
  REQUIRE(!classProperty->normalized);
  REQUIRE(classProperty->defaultProperty);

  std::vector<glm::dvec2> texCoords{
      glm::dvec2(0, 0),
      glm::dvec2(0.5, 0),
      glm::dvec2(0, 0.5),
      glm::dvec2(0.5, 0.5)};

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      "TestClassProperty",
      [defaultValue, &texCoords, &invokedCallbackCount](
          const std::string& /*propertyId*/,
          auto propertyValue) mutable {
        invokedCallbackCount++;
        if constexpr (std::is_same_v<
                          PropertyTexturePropertyView<int16_t>,
                          decltype(propertyValue)>) {
          REQUIRE(
              propertyValue.status() ==
              PropertyTexturePropertyViewStatus::EmptyPropertyWithDefault);
          REQUIRE(propertyValue.defaultValue() == defaultValue);

          for (size_t i = 0; i < texCoords.size(); ++i) {
            glm::dvec2& uv = texCoords[i];
            REQUIRE(propertyValue.get(uv[0], uv[1]) == defaultValue);
          }
        } else {
          FAIL("getPropertyView returned PropertyTexturePropertyView of "
               "incorrect type for TestClassProperty.");
        }
      });

  REQUIRE(invokedCallbackCount == 1);
}
