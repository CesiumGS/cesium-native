#include "CesiumGltf/PropertyTexturePropertyView.h"

#include <catch2/catch.hpp>
#include <gsl/span>

#include <bitset>
#include <climits>
#include <cstddef>
#include <cstring>
#include <vector>

using namespace CesiumGltf;

TEST_CASE(
    "Test PropertyTexturePropertyView on property with invalid texture index") {
  Model model;
  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  ExtensionExtStructuralMetadataSchema& schema = metadata.schema.emplace();
  ExtensionExtStructuralMetadataClass& testClass = schema.classes["TestClass"];
  ExtensionExtStructuralMetadataClassProperty& testClassProperty =
      testClass.properties["TestClassProperty"];
  testClassProperty.type =
      ExtensionExtStructuralMetadataClassProperty::Type::SCALAR;
  testClassProperty.componentType =
      ExtensionExtStructuralMetadataClassProperty::ComponentType::UINT8;

  ExtensionExtStructuralMetadataPropertyTexture propertyTexture;
  propertyTexture.classProperty = "TestClass";

  ExtensionExtStructuralMetadataPropertyTextureProperty&
      propertyTextureProperty = propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = -1;
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0};

  PropertyTexturePropertyView view(
      model,
      testClassProperty,
      propertyTextureProperty);
  REQUIRE(
      view.status() == PropertyTexturePropertyViewStatus::ErrorInvalidTexture);
}

TEST_CASE(
    "Test PropertyTexturePropertyView on property with invalid sampler index") {
  Model model;
  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  ExtensionExtStructuralMetadataSchema& schema = metadata.schema.emplace();
  ExtensionExtStructuralMetadataClass& testClass = schema.classes["TestClass"];
  ExtensionExtStructuralMetadataClassProperty& testClassProperty =
      testClass.properties["TestClassProperty"];
  testClassProperty.type =
      ExtensionExtStructuralMetadataClassProperty::Type::SCALAR;
  testClassProperty.componentType =
      ExtensionExtStructuralMetadataClassProperty::ComponentType::UINT8;

  Image& image = model.images.emplace_back();
  image.cesium.width = 1;
  image.cesium.height = 1;
  Texture& texture = model.textures.emplace_back();
  texture.sampler = -1;
  texture.source = 0;

  ExtensionExtStructuralMetadataPropertyTexture propertyTexture;
  propertyTexture.classProperty = "TestClass";

  ExtensionExtStructuralMetadataPropertyTextureProperty&
      propertyTextureProperty = propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = 0;
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0};

  PropertyTexturePropertyView view(
      model,
      testClassProperty,
      propertyTextureProperty);
  REQUIRE(
      view.status() ==
      PropertyTexturePropertyViewStatus::ErrorInvalidTextureSampler);
}

TEST_CASE(
    "Test PropertyTexturePropertyView on property with invalid image index") {
  Model model;
  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  ExtensionExtStructuralMetadataSchema& schema = metadata.schema.emplace();
  ExtensionExtStructuralMetadataClass& testClass = schema.classes["TestClass"];
  ExtensionExtStructuralMetadataClassProperty& testClassProperty =
      testClass.properties["TestClassProperty"];
  testClassProperty.type =
      ExtensionExtStructuralMetadataClassProperty::Type::SCALAR;
  testClassProperty.componentType =
      ExtensionExtStructuralMetadataClassProperty::ComponentType::UINT8;

  model.samplers.emplace_back();

  Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = -1;

  ExtensionExtStructuralMetadataPropertyTexture propertyTexture;
  propertyTexture.classProperty = "TestClass";

  ExtensionExtStructuralMetadataPropertyTextureProperty&
      propertyTextureProperty = propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = 0;
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0};

  PropertyTexturePropertyView view(
      model,
      testClassProperty,
      propertyTextureProperty);
  REQUIRE(
      view.status() == PropertyTexturePropertyViewStatus::ErrorInvalidImage);
}

TEST_CASE("Test PropertyTexturePropertyView on property with empty image") {
  Model model;
  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  ExtensionExtStructuralMetadataSchema& schema = metadata.schema.emplace();
  ExtensionExtStructuralMetadataClass& testClass = schema.classes["TestClass"];
  ExtensionExtStructuralMetadataClassProperty& testClassProperty =
      testClass.properties["TestClassProperty"];
  testClassProperty.type =
      ExtensionExtStructuralMetadataClassProperty::Type::SCALAR;
  testClassProperty.componentType =
      ExtensionExtStructuralMetadataClassProperty::ComponentType::UINT8;

  Image& image = model.images.emplace_back();
  image.cesium.width = 0;
  image.cesium.height = 0;

  model.samplers.emplace_back();

  Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = 0;

  ExtensionExtStructuralMetadataPropertyTexture propertyTexture;
  propertyTexture.classProperty = "TestClass";

  ExtensionExtStructuralMetadataPropertyTextureProperty&
      propertyTextureProperty = propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = 0;
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0};

  PropertyTexturePropertyView view(
      model,
      testClassProperty,
      propertyTextureProperty);
  REQUIRE(view.status() == PropertyTexturePropertyViewStatus::ErrorEmptyImage);
}

TEST_CASE("Test PropertyTextureView on property texture property with zero "
          "channels") {
  Model model;
  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  ExtensionExtStructuralMetadataSchema& schema = metadata.schema.emplace();
  ExtensionExtStructuralMetadataClass& testClass = schema.classes["TestClass"];
  ExtensionExtStructuralMetadataClassProperty& testClassProperty =
      testClass.properties["TestClassProperty"];
  testClassProperty.type =
      ExtensionExtStructuralMetadataClassProperty::Type::SCALAR;
  testClassProperty.componentType =
      ExtensionExtStructuralMetadataClassProperty::ComponentType::UINT8;

  Image& image = model.images.emplace_back();
  image.cesium.width = 1;
  image.cesium.height = 1;
  image.cesium.channels = 1;

  model.samplers.emplace_back();

  Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = 0;

  ExtensionExtStructuralMetadataPropertyTexture propertyTexture;
  propertyTexture.classProperty = "TestClass";

  ExtensionExtStructuralMetadataPropertyTextureProperty&
      propertyTextureProperty = propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = 0;
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {};

  PropertyTexturePropertyView view(
      model,
      testClassProperty,
      propertyTextureProperty);
  REQUIRE(
      view.status() == PropertyTexturePropertyViewStatus::ErrorInvalidChannels);
}

TEST_CASE("Test PropertyTextureView on property texture property with too many "
          "channels") {
  Model model;
  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  ExtensionExtStructuralMetadataSchema& schema = metadata.schema.emplace();
  ExtensionExtStructuralMetadataClass& testClass = schema.classes["TestClass"];
  ExtensionExtStructuralMetadataClassProperty& testClassProperty =
      testClass.properties["TestClassProperty"];
  testClassProperty.type =
      ExtensionExtStructuralMetadataClassProperty::Type::SCALAR;
  testClassProperty.componentType =
      ExtensionExtStructuralMetadataClassProperty::ComponentType::UINT8;

  Image& image = model.images.emplace_back();
  image.cesium.width = 1;
  image.cesium.height = 1;
  image.cesium.channels = 1;

  model.samplers.emplace_back();

  Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = 0;

  ExtensionExtStructuralMetadataPropertyTexture propertyTexture;
  propertyTexture.classProperty = "TestClass";

  ExtensionExtStructuralMetadataPropertyTextureProperty&
      propertyTextureProperty = propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = 0;
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0, 1};

  PropertyTexturePropertyView view(
      model,
      testClassProperty,
      propertyTextureProperty);
  REQUIRE(
      view.status() == PropertyTexturePropertyViewStatus::ErrorInvalidChannels);
}

TEST_CASE("Test PropertyTexturePropertyView on valid property texture") {
  Model model;
  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  ExtensionExtStructuralMetadataSchema& schema = metadata.schema.emplace();
  ExtensionExtStructuralMetadataClass& testClass = schema.classes["TestClass"];
  ExtensionExtStructuralMetadataClassProperty& testClassProperty =
      testClass.properties["TestClassProperty"];
  testClassProperty.type =
      ExtensionExtStructuralMetadataClassProperty::Type::SCALAR;
  testClassProperty.componentType =
      ExtensionExtStructuralMetadataClassProperty::ComponentType::UINT8;

  Image& image = model.images.emplace_back();
  image.cesium.width = 1;
  image.cesium.height = 1;
  image.cesium.channels = 1;

  model.samplers.emplace_back();

  Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = 0;

  ExtensionExtStructuralMetadataPropertyTexture propertyTexture;
  propertyTexture.classProperty = "TestClass";

  ExtensionExtStructuralMetadataPropertyTextureProperty&
      propertyTextureProperty = propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = 0;
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0};

  PropertyTexturePropertyView view(
      model,
      testClassProperty,
      propertyTextureProperty);
  REQUIRE(view.status() == PropertyTexturePropertyViewStatus::Valid);
}

TEST_CASE("Test getSwizzle") {
  Model model;
  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  ExtensionExtStructuralMetadataSchema& schema = metadata.schema.emplace();
  ExtensionExtStructuralMetadataClass& testClass = schema.classes["TestClass"];
  ExtensionExtStructuralMetadataClassProperty& testClassProperty =
      testClass.properties["TestClassProperty"];
  testClassProperty.type =
      ExtensionExtStructuralMetadataClassProperty::Type::SCALAR;
  testClassProperty.componentType =
      ExtensionExtStructuralMetadataClassProperty::ComponentType::UINT8;
  testClassProperty.count = 4;

  Image& image = model.images.emplace_back();
  image.cesium.width = 1;
  image.cesium.height = 1;
  image.cesium.channels = 4;

  model.samplers.emplace_back();

  Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = 0;

  ExtensionExtStructuralMetadataPropertyTexture propertyTexture;
  propertyTexture.classProperty = "TestClass";

  ExtensionExtStructuralMetadataPropertyTextureProperty&
      propertyTextureProperty = propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = 0;
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0, 2, 3, 1};

  PropertyTexturePropertyView view(
      model,
      testClassProperty,
      propertyTextureProperty);
  REQUIRE(view.status() == PropertyTexturePropertyViewStatus::Valid);
  REQUIRE(view.getCount() == 4);
  REQUIRE(view.getSwizzle() == "rbag");
}

TEST_CASE("Test getting value from invalid view") {
  Model model;
  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  ExtensionExtStructuralMetadataSchema& schema = metadata.schema.emplace();
  ExtensionExtStructuralMetadataClass& testClass = schema.classes["TestClass"];
  ExtensionExtStructuralMetadataClassProperty& testClassProperty =
      testClass.properties["TestClassProperty"];
  testClassProperty.type =
      ExtensionExtStructuralMetadataClassProperty::Type::SCALAR;
  testClassProperty.componentType =
      ExtensionExtStructuralMetadataClassProperty::ComponentType::UINT8;

  Image& image = model.images.emplace_back();
  image.cesium.width = 0;
  image.cesium.height = 1;
  image.cesium.channels = 1;

  model.samplers.emplace_back();

  Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = 0;

  ExtensionExtStructuralMetadataPropertyTexture propertyTexture;
  propertyTexture.classProperty = "TestClass";

  ExtensionExtStructuralMetadataPropertyTextureProperty&
      propertyTextureProperty = propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = 0;
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0};

  PropertyTexturePropertyView view(
      model,
      testClassProperty,
      propertyTextureProperty);
  REQUIRE(view.status() != PropertyTexturePropertyViewStatus::Valid);
  PropertyTexturePropertyValue<uint8_t> value = view.get<uint8_t>(0, 0);
  REQUIRE(value.components[0] == 0);
  REQUIRE(value.components[1] == 0);
  REQUIRE(value.components[2] == 0);
  REQUIRE(value.components[3] == 0);
}

TEST_CASE("Test getting value from valid view") {
  Model model;
  ExtensionModelExtStructuralMetadata& metadata =
      model.addExtension<ExtensionModelExtStructuralMetadata>();

  ExtensionExtStructuralMetadataSchema& schema = metadata.schema.emplace();
  ExtensionExtStructuralMetadataClass& testClass = schema.classes["TestClass"];
  ExtensionExtStructuralMetadataClassProperty& testClassProperty =
      testClass.properties["TestClassProperty"];
  testClassProperty.type =
      ExtensionExtStructuralMetadataClassProperty::Type::SCALAR;
  testClassProperty.componentType =
      ExtensionExtStructuralMetadataClassProperty::ComponentType::UINT8;

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

  ExtensionExtStructuralMetadataPropertyTexture propertyTexture;
  propertyTexture.classProperty = "TestClass";

  ExtensionExtStructuralMetadataPropertyTextureProperty&
      propertyTextureProperty = propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = 0;
  propertyTextureProperty.texCoord = 0;
  propertyTextureProperty.channels = {0};

  PropertyTexturePropertyView view(
      model,
      testClassProperty,
      propertyTextureProperty);
  REQUIRE(view.status() == PropertyTexturePropertyViewStatus::Valid);

  std::vector<PropertyTexturePropertyValue<uint8_t>> viewValues{
      view.get<uint8_t>(0, 0),
      view.get<uint8_t>(1.0, 0),
      view.get<uint8_t>(0, 1.0),
      view.get<uint8_t>(1.0, 1.0)};

  for (size_t i = 0; i < viewValues.size(); i++) {
    PropertyTexturePropertyValue<uint8_t>& actual = viewValues[i];
    REQUIRE(actual.components[0] == values[i]);
    REQUIRE(actual.components[1] == 0);
    REQUIRE(actual.components[2] == 0);
    REQUIRE(actual.components[3] == 0);
  }
}
