#include "CesiumGltf/StructuralMetadataPropertyTexturePropertyView.h"

#include <catch2/catch.hpp>
#include <gsl/span>

#include <bitset>
#include <climits>
#include <cstddef>
#include <cstring>
#include <vector>

using namespace CesiumGltf;
using namespace CesiumGltf::StructuralMetadata;

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

TEST_CASE("Test PropertyTextureView on property table property with negative "
          "texcoord set index") {
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

  model.samplers.emplace_back();

  Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = 0;

  ExtensionExtStructuralMetadataPropertyTexture propertyTexture;
  propertyTexture.classProperty = "TestClass";

  ExtensionExtStructuralMetadataPropertyTextureProperty&
      propertyTextureProperty = propertyTexture.properties["TestClassProperty"];
  propertyTextureProperty.index = 0;
  propertyTextureProperty.texCoord = -1;
  propertyTextureProperty.channels = {0};

  PropertyTexturePropertyView view(
      model,
      testClassProperty,
      propertyTextureProperty);
  REQUIRE(
      view.status() ==
      PropertyTexturePropertyViewStatus::ErrorInvalidTexCoordSetIndex);
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
