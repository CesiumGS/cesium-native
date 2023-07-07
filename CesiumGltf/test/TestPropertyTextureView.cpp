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

  SECTION("Access wrong type") {
    PropertyTexturePropertyView<glm::u8vec2> u8vec2Invalid =
        view.getPropertyView<glm::u8vec2>("TestClassProperty");
    REQUIRE(
        u8vec2Invalid.status() ==
        PropertyTexturePropertyViewStatus::ErrorTypeMismatch);
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

  SECTION("Channel and type mismatch") {
    model.images[imageIndex].cesium.channels = 2;
    propertyTextureProperty.channels = {0, 1};
    PropertyTexturePropertyView<uint8_t> uint8Property =
        view.getPropertyView<uint8_t>("TestClassProperty");
    REQUIRE(
        uint8Property.status() ==
        PropertyTexturePropertyViewStatus::ErrorChannelsAndTypeMismatch);
  }

  SECTION("Invalid channel values") {
    propertyTextureProperty.channels = {5};
    PropertyTexturePropertyView<uint8_t> uint8Property =
        view.getPropertyView<uint8_t>("TestClassProperty");
    REQUIRE(
        uint8Property.status() ==
        PropertyTexturePropertyViewStatus::ErrorInvalidChannels);
  }

  SECTION("Zero channel values") {
    propertyTextureProperty.channels.clear();
    PropertyTexturePropertyView<uint8_t> uint8Property =
        view.getPropertyView<uint8_t>("TestClassProperty");
    REQUIRE(
        uint8Property.status() ==
        PropertyTexturePropertyViewStatus::ErrorInvalidChannels);
  }

  SECTION("Invalid bytes per channel") {
    model.images[imageIndex].cesium.bytesPerChannel = 2;
    PropertyTexturePropertyView<uint8_t> uint8Property =
        view.getPropertyView<uint8_t>("TestClassProperty");
    REQUIRE(
        uint8Property.status() ==
        PropertyTexturePropertyViewStatus::ErrorInvalidBytesPerChannel);
  }

  SECTION("Empty image") {
    model.images[imageIndex].cesium.width = 0;
    PropertyTexturePropertyView<uint8_t> uint8Property =
        view.getPropertyView<uint8_t>("TestClassProperty");
    REQUIRE(
        uint8Property.status() ==
        PropertyTexturePropertyViewStatus::ErrorEmptyImage);
  }

  SECTION("Wrong image index") {
    model.textures[textureIndex].source = 1;
    PropertyTexturePropertyView<uint8_t> uint8Property =
        view.getPropertyView<uint8_t>("TestClassProperty");
    REQUIRE(
        uint8Property.status() ==
        PropertyTexturePropertyViewStatus::ErrorInvalidImage);
  }

  SECTION("Wrong sampler index") {
    model.textures[textureIndex].sampler = 1;
    PropertyTexturePropertyView<uint8_t> uint8Property =
        view.getPropertyView<uint8_t>("TestClassProperty");
    REQUIRE(
        uint8Property.status() ==
        PropertyTexturePropertyViewStatus::ErrorInvalidSampler);
  }

  SECTION("Wrong texture index") {
    propertyTextureProperty.index = 1;
    PropertyTexturePropertyView<uint8_t> uint8Property =
        view.getPropertyView<uint8_t>("TestClassProperty");
    REQUIRE(
        uint8Property.status() ==
        PropertyTexturePropertyViewStatus::ErrorInvalidTexture);
  }
}

TEST_CASE("Test vecN PropertyTextureProperty") {
  Model model;
  std::vector<uint8_t> data = {12, 34, 10, 3, 40, 0, 30, 11};

  addTextureToModel(
      model,
      Sampler::WrapS::CLAMP_TO_EDGE,
      Sampler::WrapS::CLAMP_TO_EDGE,
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

  SECTION("Access correct type") {
    PropertyTexturePropertyView<glm::u8vec2> u8vec2Property =
        view.getPropertyView<glm::u8vec2>("TestClassProperty");
    REQUIRE(
        u8vec2Property.status() == PropertyTexturePropertyViewStatus::Valid);

    std::vector<glm::u8vec2> expected{
        glm::u8vec2(12, 34),
        glm::u8vec2(10, 3),
        glm::u8vec2(40, 0),
        glm::u8vec2(30, 11)};
    std::vector<glm::u8vec2> values{
        u8vec2Property.get(0.0, 0.0),
        u8vec2Property.get(0.5, 0.0),
        u8vec2Property.get(0.0, 0.5),
        u8vec2Property.get(0.5, 0.5),
    };
    for (size_t i = 0; i < values.size(); ++i) {
      REQUIRE(values[i] == expected[i]);
    }
  }

  SECTION("Access wrong type") {
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

  SECTION("Access wrong component type") {
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

  SECTION("Channel and type mismatch") {
    model.images[imageIndex].cesium.channels = 4;
    propertyTextureProperty.channels = {0, 1, 2, 3};
    PropertyTexturePropertyView<glm::u8vec2> u8vec2Property =
        view.getPropertyView<glm::u8vec2>("TestClassProperty");
    REQUIRE(
        u8vec2Property.status() ==
        PropertyTexturePropertyViewStatus::ErrorChannelsAndTypeMismatch);
  }

  SECTION("Invalid channel values") {
    propertyTextureProperty.channels = {0, 4};
    PropertyTexturePropertyView<glm::u8vec2> u8vec2Property =
        view.getPropertyView<glm::u8vec2>("TestClassProperty");
    REQUIRE(
        u8vec2Property.status() ==
        PropertyTexturePropertyViewStatus::ErrorInvalidChannels);
  }

  SECTION("Invalid bytes per channel") {
    model.images[imageIndex].cesium.bytesPerChannel = 2;
    PropertyTexturePropertyView<glm::u8vec2> u8vec2Property =
        view.getPropertyView<glm::u8vec2>("TestClassProperty");
    REQUIRE(
        u8vec2Property.status() ==
        PropertyTexturePropertyViewStatus::ErrorInvalidBytesPerChannel);
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

  addTextureToModel(
      model,
      Sampler::WrapS::CLAMP_TO_EDGE,
      Sampler::WrapS::CLAMP_TO_EDGE,
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

  SECTION("Access correct type") {
    PropertyTexturePropertyView<PropertyArrayView<uint8_t>> uint8ArrayProperty =
        view.getPropertyView<PropertyArrayView<uint8_t>>("TestClassProperty");
    REQUIRE(
        uint8ArrayProperty.status() ==
        PropertyTexturePropertyViewStatus::Valid);

    std::vector<PropertyArrayView<uint8_t>> values{
        uint8ArrayProperty.get(0.0, 0.0),
        uint8ArrayProperty.get(0.5, 0.0),
        uint8ArrayProperty.get(0.0, 0.5),
        uint8ArrayProperty.get(0.5, 0.5),
    };

    int64_t size = static_cast<int64_t>(values.size());
    for (int64_t i = 0; i < size; ++i) {
      auto dataStart = data.begin() + i * 3;
      std::vector<uint8_t> expected(dataStart, dataStart + 3);
      const PropertyArrayView<uint8_t>& value = values[static_cast<size_t>(i)];
      REQUIRE(static_cast<size_t>(value.size()) == expected.size());
      for (int64_t j = 0; j < value.size(); j++) {
        REQUIRE(value[j] == expected[static_cast<size_t>(j)]);
      }
    }
  }

  SECTION("Access wrong component type") {
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

  SECTION("Access incorrectly as non-array") {
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

  SECTION("Channel and type mismatch") {
    model.images[imageIndex].cesium.channels = 4;
    propertyTextureProperty.channels = {0, 1, 2, 3};
    PropertyTexturePropertyView<PropertyArrayView<uint8_t>> uint8ArrayProperty =
        view.getPropertyView<PropertyArrayView<uint8_t>>("TestClassProperty");
    REQUIRE(
        uint8ArrayProperty.status() ==
        PropertyTexturePropertyViewStatus::ErrorChannelsAndTypeMismatch);
  }

  SECTION("Invalid channel values") {
    propertyTextureProperty.channels = {0, 4, 1};
    PropertyTexturePropertyView<PropertyArrayView<uint8_t>> uint8ArrayProperty =
        view.getPropertyView<PropertyArrayView<uint8_t>>("TestClassProperty");
    REQUIRE(
        uint8ArrayProperty.status() ==
        PropertyTexturePropertyViewStatus::ErrorInvalidChannels);
  }

  SECTION("Invalid bytes per channel") {
    model.images[imageIndex].cesium.bytesPerChannel = 2;
    PropertyTexturePropertyView<PropertyArrayView<uint8_t>> uint8ArrayProperty =
        view.getPropertyView<PropertyArrayView<uint8_t>>("TestClassProperty");
    REQUIRE(
        uint8ArrayProperty.status() ==
        PropertyTexturePropertyViewStatus::ErrorInvalidBytesPerChannel);
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
          const std::string& /*propertyName*/,
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
                          const std::string& /*propertyName*/,
                          auto propertyValue) mutable {
    invokedCallbackCount++;
    REQUIRE(propertyValue.status() != PropertyTexturePropertyViewStatus::Valid);
  };

  view.getPropertyView("InvalidProperty", testCallback);
  view.getPropertyView("NonexistentProperty", testCallback);

  REQUIRE(invokedCallbackCount == 2);
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

  std::vector<int16_t> expected{-1, 268, 542, -256};
  std::vector<glm::vec2> texCoords{
      glm::vec2(0, 0),
      glm::vec2(0.5, 0),
      glm::vec2(0, 0.5),
      glm::vec2(0.5, 0.5)};

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      "TestClassProperty",
      [&expected, &texCoords, &invokedCallbackCount](
          const std::string& /*propertyName*/,
          auto propertyValue) mutable {
        invokedCallbackCount++;
        if constexpr (std::is_same_v<
                          PropertyTexturePropertyView<int16_t>,
                          decltype(propertyValue)>) {
          REQUIRE(
              propertyValue.status() ==
              PropertyTexturePropertyViewStatus::Valid);

          for (size_t i = 0; i < expected.size(); ++i) {
            glm::vec2& texCoord = texCoords[i];
            REQUIRE(propertyValue.get(texCoord[0], texCoord[1]) == expected[i]);
          }
        } else {
          FAIL("getPropertyView returned PropertyTexturePropertyView of "
               "incorrect type for TestClassProperty.");
        }
      });

  REQUIRE(invokedCallbackCount == 1);
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

  std::vector<glm::i8vec2> expected{
      glm::i8vec2(-1, -1),
      glm::i8vec2(12, 1),
      glm::i8vec2(30, 2),
      glm::i8vec2(0, -1)};
  std::vector<glm::vec2> texCoords{
      glm::vec2(0, 0),
      glm::vec2(0.5, 0),
      glm::vec2(0, 0.5),
      glm::vec2(0.5, 0.5)};

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      "TestClassProperty",
      [&expected, &texCoords, &invokedCallbackCount](
          const std::string& /*propertyName*/,
          auto propertyValue) mutable {
        invokedCallbackCount++;
        if constexpr (std::is_same_v<
                          PropertyTexturePropertyView<glm::i8vec2>,
                          decltype(propertyValue)>) {
          REQUIRE(
              propertyValue.status() ==
              PropertyTexturePropertyViewStatus::Valid);

          for (size_t i = 0; i < expected.size(); ++i) {
            glm::vec2& texCoord = texCoords[i];
            REQUIRE(propertyValue.get(texCoord[0], texCoord[1]) == expected[i]);
          }
        } else {
          FAIL("getPropertyView returned PropertyTexturePropertyView of "
               "incorrect type for TestClassProperty.");
        }
      });

  REQUIRE(invokedCallbackCount == 1);
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

  std::vector<std::vector<uint16_t>> expected{
      {254, 509},
      {522, 808},
      {30, 512},
      {522, 1279}};
  std::vector<glm::vec2> texCoords{
      glm::vec2(0, 0),
      glm::vec2(0.5, 0),
      glm::vec2(0, 0.5),
      glm::vec2(0.5, 0.5)};

  uint32_t invokedCallbackCount = 0;
  view.getPropertyView(
      "TestClassProperty",
      [&expected, &texCoords, &invokedCallbackCount](
          const std::string& /*propertyName*/,
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
            glm::vec2& texCoord = texCoords[i];
            PropertyArrayView<uint16_t> array =
                propertyValue.get(texCoord[0], texCoord[1]);

            REQUIRE(static_cast<size_t>(array.size()) == expectedArray.size());
            for (int64_t j = 0; j < array.size(); j++) {
              REQUIRE(array[j] == expectedArray[static_cast<size_t>(j)]);
            }
          }
        } else {
          FAIL("getPropertyView returned PropertyTexturePropertyView of "
               "incorrect type for TestClassProperty.");
        }
      });

  REQUIRE(invokedCallbackCount == 1);
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
          const std::string& /*propertyName*/,
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
          const std::string& /*propertyName*/,
          auto propertyValue) mutable {
        invokedCallbackCount++;
        REQUIRE(
            propertyValue.status() ==
            PropertyTexturePropertyViewStatus::ErrorUnsupportedProperty);
      });
  REQUIRE(invokedCallbackCount == 2);
}
