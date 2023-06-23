#include "CesiumGltf/PropertyTextureView.h"

#include <catch2/catch.hpp>
#include <gsl/span>

#include <bitset>
#include <climits>
#include <cstddef>
#include <cstring>
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
  image.cesium.width = width;
  image.cesium.height = height;
  image.cesium.channels = channels;
  image.cesium.bytesPerChannel = 1;

  std::vector<std::byte>& imageData = image.cesium.pixelData;
  imageData.resize(data.size());
  std::memcpy(imageData.data(), data.data(), data.size());

  Sampler& sampler = model.samplers.emplace_back();
  sampler.wrapS = wrapS;
  sampler.wrapT = wrapT;

  Texture& texture = model.textures.emplace_back();
  texture.sampler = static_cast<int32_t>(model.samplers.size() - 1);
  texture.source = static_cast<int32_t>(model.images.size() - 1);
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
      Sampler::WrapS::CLAMP_TO_EDGE,
      Sampler::WrapS::CLAMP_TO_EDGE,
      2,
      2,
      1,
      data);

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
  propertyTextureProperty.index = 0;
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

  SECTION("Access correct type") {
    PropertyTexturePropertyView<uint8_t> uint8Property =
        view.getPropertyView<uint8_t>("TestClassProperty");
    REQUIRE(uint8Property.status() == PropertyTexturePropertyViewStatus::Valid);

    std::vector<uint8_t> values{
        uint8Property.get(0.0, 0.0),
        uint8Property.get(0.5, 0.0),
        uint8Property.get(0.0, 0.5),
        uint8Property.get(0.5, 0.5),
    };

    for (size_t i = 0; i < values.size(); ++i) {
      REQUIRE(values[i] == data[i]);
    }
  }

  SECTION("Access wrong component type") {
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

  SECTION("Access incorrectly as array") {
    PropertyTexturePropertyView<PropertyArrayView<uint8_t>> arrayInvalid =
        view.getPropertyView<PropertyArrayView<uint8_t>>("TestClassProperty");
    REQUIRE(
        arrayInvalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorArrayTypeMismatch);
  }
}

// TEST_CASE(
//     "Test PropertyTexturePropertyView on property with invalid texture
//     index") {
//   Model model;
//   ExtensionModelExtStructuralMetadata& metadata =
//       model.addExtension<ExtensionModelExtStructuralMetadata>();
//
//   Schema& schema = metadata.schema.emplace();
//   Class& testClass = schema.classes["TestClass"];
//   ClassProperty& testClassProperty =
//   testClass.properties["TestClassProperty"]; testClassProperty.type =
//   ClassProperty::Type::SCALAR; testClassProperty.componentType =
//   ClassProperty::ComponentType::UINT8;
//
//   PropertyTexture propertyTexture;
//   propertyTexture.classProperty = "TestClass";
//
//   PropertyTextureProperty& propertyTextureProperty =
//       propertyTexture.properties["TestClassProperty"];
//   propertyTextureProperty.index = -1;
//   propertyTextureProperty.texCoord = 0;
//  propertyTextureProperty.channels = {0};
//
//  PropertyTexturePropertyView view(
//      model,
//      testClassProperty,
//      propertyTextureProperty);
//  REQUIRE(
//      view.status() ==
//      PropertyTexturePropertyViewStatus::ErrorInvalidTexture);
//}
//
// TEST_CASE(
//    "Test PropertyTexturePropertyView on property with invalid sampler index")
//    {
//  Model model;
//  ExtensionModelExtStructuralMetadata& metadata =
//      model.addExtension<ExtensionModelExtStructuralMetadata>();
//
//  Schema& schema = metadata.schema.emplace();
//  Class& testClass = schema.classes["TestClass"];
//  ClassProperty& testClassProperty =
//  testClass.properties["TestClassProperty"]; testClassProperty.type =
//  ClassProperty::Type::SCALAR; testClassProperty.componentType =
//  ClassProperty::ComponentType::UINT8;
//
//  Image& image = model.images.emplace_back();
//  image.cesium.width = 1;
//  image.cesium.height = 1;
//  Texture& texture = model.textures.emplace_back();
//  texture.sampler = -1;
//  texture.source = 0;
//
//  PropertyTexture propertyTexture;
//  propertyTexture.classProperty = "TestClass";
//
//  PropertyTextureProperty& propertyTextureProperty =
//      propertyTexture.properties["TestClassProperty"];
//  propertyTextureProperty.index = 0;
//  propertyTextureProperty.texCoord = 0;
//  propertyTextureProperty.channels = {0};
//
//  PropertyTexturePropertyView view(
//      model,
//      testClassProperty,
//      propertyTextureProperty);
//  REQUIRE(
//      view.status() ==
//      PropertyTexturePropertyViewStatus::ErrorInvalidTextureSampler);
//}
//
// TEST_CASE(
//    "Test PropertyTexturePropertyView on property with invalid image index") {
//  Model model;
//  ExtensionModelExtStructuralMetadata& metadata =
//      model.addExtension<ExtensionModelExtStructuralMetadata>();
//
//  Schema& schema = metadata.schema.emplace();
//  Class& testClass = schema.classes["TestClass"];
//  ClassProperty& testClassProperty =
//  testClass.properties["TestClassProperty"]; testClassProperty.type =
//  ClassProperty::Type::SCALAR; testClassProperty.componentType =
//  ClassProperty::ComponentType::UINT8;
//
//  model.samplers.emplace_back();
//
//  Texture& texture = model.textures.emplace_back();
//  texture.sampler = 0;
//  texture.source = -1;
//
//  PropertyTexture propertyTexture;
//  propertyTexture.classProperty = "TestClass";
//
//  PropertyTextureProperty& propertyTextureProperty =
//      propertyTexture.properties["TestClassProperty"];
//  propertyTextureProperty.index = 0;
//  propertyTextureProperty.texCoord = 0;
//  propertyTextureProperty.channels = {0};
//
//  PropertyTexturePropertyView view(
//      model,
//      testClassProperty,
//      propertyTextureProperty);
//  REQUIRE(
//      view.status() == PropertyTexturePropertyViewStatus::ErrorInvalidImage);
//}
//
// TEST_CASE("Test PropertyTexturePropertyView on property with empty image") {
//  Model model;
//  ExtensionModelExtStructuralMetadata& metadata =
//      model.addExtension<ExtensionModelExtStructuralMetadata>();
//
//  Schema& schema = metadata.schema.emplace();
//  Class& testClass = schema.classes["TestClass"];
//  ClassProperty& testClassProperty =
//  testClass.properties["TestClassProperty"]; testClassProperty.type =
//  ClassProperty::Type::SCALAR; testClassProperty.componentType =
//  ClassProperty::ComponentType::UINT8;
//
//  Image& image = model.images.emplace_back();
//  image.cesium.width = 0;
//  image.cesium.height = 0;
//
//  model.samplers.emplace_back();
//
//  Texture& texture = model.textures.emplace_back();
//  texture.sampler = 0;
//  texture.source = 0;
//
//  PropertyTexture propertyTexture;
//  propertyTexture.classProperty = "TestClass";
//
//  PropertyTextureProperty& propertyTextureProperty =
//      propertyTexture.properties["TestClassProperty"];
//  propertyTextureProperty.index = 0;
//  propertyTextureProperty.texCoord = 0;
//  propertyTextureProperty.channels = {0};
//
//  PropertyTexturePropertyView view(
//      model,
//      testClassProperty,
//      propertyTextureProperty);
//  REQUIRE(view.status() ==
//  PropertyTexturePropertyViewStatus::ErrorEmptyImage);
//}
//
// TEST_CASE("Test PropertyTextureView on property texture property with zero "
//          "channels") {
//  Model model;
//  ExtensionModelExtStructuralMetadata& metadata =
//      model.addExtension<ExtensionModelExtStructuralMetadata>();
//
//  Schema& schema = metadata.schema.emplace();
//  Class& testClass = schema.classes["TestClass"];
//  ClassProperty& testClassProperty =
//  testClass.properties["TestClassProperty"]; testClassProperty.type =
//  ClassProperty::Type::SCALAR; testClassProperty.componentType =
//  ClassProperty::ComponentType::UINT8;
//
//  Image& image = model.images.emplace_back();
//  image.cesium.width = 1;
//  image.cesium.height = 1;
//  image.cesium.channels = 1;
//
//  model.samplers.emplace_back();
//
//  Texture& texture = model.textures.emplace_back();
//  texture.sampler = 0;
//  texture.source = 0;
//
//  PropertyTexture propertyTexture;
//  propertyTexture.classProperty = "TestClass";
//
//  PropertyTextureProperty& propertyTextureProperty =
//      propertyTexture.properties["TestClassProperty"];
//  propertyTextureProperty.index = 0;
//  propertyTextureProperty.texCoord = 0;
//  propertyTextureProperty.channels = {};
//
//  PropertyTexturePropertyView view(
//      model,
//      testClassProperty,
//      propertyTextureProperty);
//  REQUIRE(
//      view.status() ==
//      PropertyTexturePropertyViewStatus::ErrorInvalidChannels);
//}
//
// TEST_CASE("Test PropertyTextureView on property texture property with too
// many "
//          "channels") {
//  Model model;
//  ExtensionModelExtStructuralMetadata& metadata =
//      model.addExtension<ExtensionModelExtStructuralMetadata>();
//
//  Schema& schema = metadata.schema.emplace();
//  Class& testClass = schema.classes["TestClass"];
//  ClassProperty& testClassProperty =
//  testClass.properties["TestClassProperty"]; testClassProperty.type =
//  ClassProperty::Type::SCALAR; testClassProperty.componentType =
//  ClassProperty::ComponentType::UINT8;
//
//  Image& image = model.images.emplace_back();
//  image.cesium.width = 1;
//  image.cesium.height = 1;
//  image.cesium.channels = 1;
//
//  model.samplers.emplace_back();
//
//  Texture& texture = model.textures.emplace_back();
//  texture.sampler = 0;
//  texture.source = 0;
//
//  PropertyTexture propertyTexture;
//  propertyTexture.classProperty = "TestClass";
//
//  PropertyTextureProperty& propertyTextureProperty =
//      propertyTexture.properties["TestClassProperty"];
//  propertyTextureProperty.index = 0;
//  propertyTextureProperty.texCoord = 0;
//  propertyTextureProperty.channels = {0, 1};
//
//  PropertyTexturePropertyView view(
//      model,
//      testClassProperty,
//      propertyTextureProperty);
//  REQUIRE(
//      view.status() ==
//      PropertyTexturePropertyViewStatus::ErrorInvalidChannels);
//}
//
// TEST_CASE("Test PropertyTextureView on property texture property with "
//          "unsupported class property") {
//  Model model;
//  ExtensionModelExtStructuralMetadata& metadata =
//      model.addExtension<ExtensionModelExtStructuralMetadata>();
//
//  Schema& schema = metadata.schema.emplace();
//  Class& testClass = schema.classes["TestClass"];
//  ClassProperty& testClassProperty =
//  testClass.properties["TestClassProperty"]; testClassProperty.type =
//  ClassProperty::Type::SCALAR; testClassProperty.componentType =
//  ClassProperty::ComponentType::UINT8;
//
//  Image& image = model.images.emplace_back();
//  image.cesium.width = 1;
//  image.cesium.height = 1;
//  image.cesium.channels = 1;
//
//  model.samplers.emplace_back();
//
//  Texture& texture = model.textures.emplace_back();
//  texture.sampler = 0;
//  texture.source = 0;
//
//  PropertyTexture propertyTexture;
//  propertyTexture.classProperty = "TestClass";
//
//  PropertyTextureProperty& propertyTextureProperty =
//      propertyTexture.properties["TestClassProperty"];
//  propertyTextureProperty.index = 0;
//  propertyTextureProperty.texCoord = 0;
//  propertyTextureProperty.channels = {0, 1};
//
//  PropertyTexturePropertyView view(
//      model,
//      testClassProperty,
//      propertyTextureProperty);
//  REQUIRE(
//      view.status() ==
//      PropertyTexturePropertyViewStatus::ErrorInvalidChannels);
//}
/*
TEST_CASE("Test PropertyTexturePropertyView on valid property texture") {
  Model model;
  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty =
testClass.properties["TestClassProperty"]; testClassProperty.type =
ClassProperty::Type::SCALAR; testClassProperty.componentType =
ClassProperty::ComponentType::UINT8;

  Image& image = model.images.emplace_back();
  image.cesium.width = 1;
  image.cesium.height = 1;
  image.cesium.channels = 1;

  model.samplers.emplace_back();

  Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = 0;

  PropertyTexture propertyTexture;
  propertyTexture.classProperty = "TestClass";

  PropertyTextureProperty& propertyTextureProperty =
      propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = 0;
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0};

  PropertyTexturePropertyView view(
      model,
      testClassProperty,
      propertyTextureProperty);
  REQUIRE(view.status() == PropertyTexturePropertyViewStatus::Valid);
}

TEST_CASE("Test getting value from valid view") {
  Model model;
  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  Schema& schema = metadata.schema.emplace();
  Class& testClass = schema.classes["TestClass"];
  ClassProperty& testClassProperty =
testClass.properties["TestClassProperty"]; testClassProperty.type =
ClassProperty::Type::SCALAR; testClassProperty.componentType =
ClassProperty::ComponentType::UINT8;

  std::vector<uint8_t> values{10, 8, 4, 22};

  Image& image = model.images.emplace_back();
  image.cesium.width = 2;
  image.cesium.height = 2;
  image.cesium.channels = 1;
  image.cesium.bytesPerChannel = 1;
  image.cesium.pixelData.resize(values.size());
  std::memcpy(image.cesium.pixelData.data(), values.data(), values.size());

  Sampler& sampler = model.samplers.emplace_back();
  sampler.wrapS = Sampler::WrapS::CLAMP_TO_EDGE;
  sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

  Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = 0;

  PropertyTexture propertyTexture;
  propertyTexture.classProperty = "TestClass";

  PropertyTextureProperty& propertyTextureProperty =
      propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = 0;
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0};

  PropertyTexturePropertyView view(
      model,
      testClassProperty,
      propertyTextureProperty);


}*/
