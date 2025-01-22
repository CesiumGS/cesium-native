#include <CesiumGltf/ExtensionExtMeshFeatures.h>
#include <CesiumGltf/ExtensionKhrTextureTransform.h>
#include <CesiumGltf/FeatureId.h>
#include <CesiumGltf/FeatureIdTexture.h>
#include <CesiumGltf/FeatureIdTextureView.h>
#include <CesiumGltf/Image.h>
#include <CesiumGltf/KhrTextureTransform.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/Sampler.h>
#include <CesiumGltf/Texture.h>
#include <CesiumGltf/TextureView.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>

#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <vector>

using namespace CesiumGltf;

TEST_CASE("Test FeatureIdTextureView on feature ID texture with invalid "
          "texture index") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();
  Sampler& sampler = model.samplers.emplace_back();
  sampler.wrapS = Sampler::WrapS::CLAMP_TO_EDGE;
  sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

  ExtensionExtMeshFeatures& meshFeatures =
      primitive.addExtension<ExtensionExtMeshFeatures>();

  FeatureIdTexture featureIdTexture;
  featureIdTexture.index = -1;
  featureIdTexture.texCoord = 0;
  featureIdTexture.channels = {0};

  FeatureId featureId = meshFeatures.featureIds.emplace_back();
  featureId.texture = featureIdTexture;

  FeatureIdTextureView view(model, featureIdTexture);
  REQUIRE(view.status() == FeatureIdTextureViewStatus::ErrorInvalidTexture);
}

TEST_CASE("Test FeatureIdTextureView on feature ID texture with invalid image "
          "index") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();
  Sampler& sampler = model.samplers.emplace_back();
  sampler.wrapS = Sampler::WrapS::CLAMP_TO_EDGE;
  sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

  Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = -1;

  ExtensionExtMeshFeatures& meshFeatures =
      primitive.addExtension<ExtensionExtMeshFeatures>();

  FeatureIdTexture featureIdTexture;
  featureIdTexture.index = 0;
  featureIdTexture.texCoord = 0;
  featureIdTexture.channels = {0};

  FeatureId featureId = meshFeatures.featureIds.emplace_back();
  featureId.texture = featureIdTexture;

  FeatureIdTextureView view(model, featureIdTexture);
  REQUIRE(view.status() == FeatureIdTextureViewStatus::ErrorInvalidImage);
}

TEST_CASE("Test FeatureIdTextureView on feature ID texture with empty image") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();
  Sampler& sampler = model.samplers.emplace_back();
  sampler.wrapS = Sampler::WrapS::CLAMP_TO_EDGE;
  sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

  Image& image = model.images.emplace_back();
  image.pAsset.emplace();
  image.pAsset->width = 0;
  image.pAsset->height = 0;

  Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = 0;

  ExtensionExtMeshFeatures& meshFeatures =
      primitive.addExtension<ExtensionExtMeshFeatures>();

  FeatureIdTexture featureIdTexture;
  featureIdTexture.index = 0;
  featureIdTexture.texCoord = 0;
  featureIdTexture.channels = {0};

  FeatureId featureId = meshFeatures.featureIds.emplace_back();
  featureId.texture = featureIdTexture;

  FeatureIdTextureView view(model, featureIdTexture);
  REQUIRE(view.status() == FeatureIdTextureViewStatus::ErrorEmptyImage);
}

TEST_CASE("Test FeatureIdTextureView on feature ID texture with too many bytes "
          "per channel") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();
  Sampler& sampler = model.samplers.emplace_back();
  sampler.wrapS = Sampler::WrapS::CLAMP_TO_EDGE;
  sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

  Image& image = model.images.emplace_back();
  image.pAsset.emplace();
  image.pAsset->width = 1;
  image.pAsset->height = 1;
  image.pAsset->bytesPerChannel = 2;

  Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = 0;

  ExtensionExtMeshFeatures& meshFeatures =
      primitive.addExtension<ExtensionExtMeshFeatures>();

  FeatureIdTexture featureIdTexture;
  featureIdTexture.index = 0;
  featureIdTexture.texCoord = 0;
  featureIdTexture.channels = {0};

  FeatureId featureId = meshFeatures.featureIds.emplace_back();
  featureId.texture = featureIdTexture;

  FeatureIdTextureView view(model, featureIdTexture);
  REQUIRE(
      view.status() ==
      FeatureIdTextureViewStatus::ErrorInvalidImageBytesPerChannel);
}

TEST_CASE(
    "Test FeatureIdTextureView on feature ID texture with zero channels") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();
  Sampler& sampler = model.samplers.emplace_back();
  sampler.wrapS = Sampler::WrapS::CLAMP_TO_EDGE;
  sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

  Image& image = model.images.emplace_back();
  image.pAsset.emplace();
  image.pAsset->width = 1;
  image.pAsset->height = 1;

  Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = 0;

  ExtensionExtMeshFeatures& meshFeatures =
      primitive.addExtension<ExtensionExtMeshFeatures>();

  FeatureIdTexture featureIdTexture;
  featureIdTexture.index = 0;
  featureIdTexture.texCoord = 0;
  featureIdTexture.channels = {};

  FeatureId featureId = meshFeatures.featureIds.emplace_back();
  featureId.texture = featureIdTexture;

  FeatureIdTextureView view(model, featureIdTexture);
  REQUIRE(view.status() == FeatureIdTextureViewStatus::ErrorInvalidChannels);
}

TEST_CASE(
    "Test FeatureIdTextureView on feature ID texture with too many channels") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();
  Sampler& sampler = model.samplers.emplace_back();
  sampler.wrapS = Sampler::WrapS::CLAMP_TO_EDGE;
  sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

  Image& image = model.images.emplace_back();
  image.pAsset.emplace();
  image.pAsset->width = 1;
  image.pAsset->height = 1;

  Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = 0;

  ExtensionExtMeshFeatures& meshFeatures =
      primitive.addExtension<ExtensionExtMeshFeatures>();

  FeatureIdTexture featureIdTexture;
  featureIdTexture.index = 0;
  featureIdTexture.texCoord = 0;
  featureIdTexture.channels = {0, 1, 2, 3, 3};

  FeatureId featureId = meshFeatures.featureIds.emplace_back();
  featureId.texture = featureIdTexture;

  FeatureIdTextureView view(model, featureIdTexture);
  REQUIRE(view.status() == FeatureIdTextureViewStatus::ErrorInvalidChannels);
}

TEST_CASE("Test FeatureIdTextureView on feature ID texture with out of range "
          "channel") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();
  Sampler& sampler = model.samplers.emplace_back();
  sampler.wrapS = Sampler::WrapS::CLAMP_TO_EDGE;
  sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

  Image& image = model.images.emplace_back();
  image.pAsset.emplace();
  image.pAsset->width = 1;
  image.pAsset->height = 1;

  Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = 0;

  ExtensionExtMeshFeatures& meshFeatures =
      primitive.addExtension<ExtensionExtMeshFeatures>();

  FeatureIdTexture featureIdTexture;
  featureIdTexture.index = 0;
  featureIdTexture.texCoord = 0;
  featureIdTexture.channels = {4};

  FeatureId featureId = meshFeatures.featureIds.emplace_back();
  featureId.texture = featureIdTexture;

  FeatureIdTextureView view(model, featureIdTexture);
  REQUIRE(view.status() == FeatureIdTextureViewStatus::ErrorInvalidChannels);
}

TEST_CASE("Test FeatureIdTextureView on valid feature ID texture") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();
  Sampler& sampler = model.samplers.emplace_back();
  sampler.wrapS = Sampler::WrapS::CLAMP_TO_EDGE;
  sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

  Image& image = model.images.emplace_back();
  image.pAsset.emplace();
  image.pAsset->width = 1;
  image.pAsset->height = 1;

  Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = 0;

  ExtensionExtMeshFeatures& meshFeatures =
      primitive.addExtension<ExtensionExtMeshFeatures>();

  FeatureIdTexture featureIdTexture;
  featureIdTexture.index = 0;
  featureIdTexture.texCoord = 0;
  featureIdTexture.channels = {0};

  FeatureId featureId = meshFeatures.featureIds.emplace_back();
  featureId.texture = featureIdTexture;

  FeatureIdTextureView view(model, featureIdTexture);
  REQUIRE(view.status() == FeatureIdTextureViewStatus::Valid);
}

TEST_CASE("Test FeatureIdTextureView with applyKhrTextureTransformExtension = "
          "false") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();
  Sampler& sampler = model.samplers.emplace_back();
  sampler.wrapS = Sampler::WrapS::CLAMP_TO_EDGE;
  sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

  Image& image = model.images.emplace_back();
  image.pAsset.emplace();
  image.pAsset->width = 1;
  image.pAsset->height = 1;

  Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = 0;

  ExtensionExtMeshFeatures& meshFeatures =
      primitive.addExtension<ExtensionExtMeshFeatures>();

  FeatureIdTexture featureIdTexture;
  featureIdTexture.index = 0;
  featureIdTexture.texCoord = 0;
  featureIdTexture.channels = {0};

  ExtensionKhrTextureTransform& textureTransformExtension =
      featureIdTexture.addExtension<ExtensionKhrTextureTransform>();
  textureTransformExtension.offset = {1.0, 2.0};
  textureTransformExtension.rotation = CesiumUtility::Math::PiOverTwo;
  textureTransformExtension.scale = {2.0, 0.5};
  textureTransformExtension.texCoord = 10;

  FeatureId featureId = meshFeatures.featureIds.emplace_back();
  featureId.texture = featureIdTexture;

  FeatureIdTextureView view(model, featureIdTexture);
  REQUIRE(view.status() == FeatureIdTextureViewStatus::Valid);

  auto textureTransform = view.getTextureTransform();
  REQUIRE(textureTransform != std::nullopt);
  REQUIRE(textureTransform->offset() == glm::dvec2(1.0, 2.0));
  REQUIRE(textureTransform->rotation() == CesiumUtility::Math::PiOverTwo);
  REQUIRE(textureTransform->scale() == glm::dvec2(2.0, 0.5));

  // Texcoord is not overridden by value in KHR_texture_transform.
  REQUIRE(view.getTexCoordSetIndex() == 0);
  REQUIRE(textureTransform->getTexCoordSetIndex() == 10);
}

TEST_CASE("Test FeatureIdTextureView with applyKhrTextureTransformExtension = "
          "true") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();
  Sampler& sampler = model.samplers.emplace_back();
  sampler.wrapS = Sampler::WrapS::CLAMP_TO_EDGE;
  sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

  Image& image = model.images.emplace_back();
  image.pAsset.emplace();
  image.pAsset->width = 1;
  image.pAsset->height = 1;

  Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = 0;

  ExtensionExtMeshFeatures& meshFeatures =
      primitive.addExtension<ExtensionExtMeshFeatures>();

  FeatureIdTexture featureIdTexture;
  featureIdTexture.index = 0;
  featureIdTexture.texCoord = 0;
  featureIdTexture.channels = {0};

  ExtensionKhrTextureTransform& textureTransformExtension =
      featureIdTexture.addExtension<ExtensionKhrTextureTransform>();
  textureTransformExtension.offset = {1.0, 2.0};
  textureTransformExtension.rotation = CesiumUtility::Math::PiOverTwo;
  textureTransformExtension.scale = {2.0, 0.5};
  textureTransformExtension.texCoord = 10;

  FeatureId featureId = meshFeatures.featureIds.emplace_back();
  featureId.texture = featureIdTexture;

  TextureViewOptions options;
  options.applyKhrTextureTransformExtension = true;
  FeatureIdTextureView view(model, featureIdTexture, options);
  REQUIRE(view.status() == FeatureIdTextureViewStatus::Valid);

  auto textureTransform = view.getTextureTransform();
  REQUIRE(textureTransform != std::nullopt);
  REQUIRE(textureTransform->offset() == glm::dvec2(1.0, 2.0));
  REQUIRE(textureTransform->rotation() == CesiumUtility::Math::PiOverTwo);
  REQUIRE(textureTransform->scale() == glm::dvec2(2.0, 0.5));

  // Texcoord is overridden by value in KHR_texture_transform.
  REQUIRE(
      view.getTexCoordSetIndex() == textureTransform->getTexCoordSetIndex());
  REQUIRE(textureTransform->getTexCoordSetIndex() == 10);
}

TEST_CASE("Test FeatureIdTextureView with makeImageCopy = true") {
  std::vector<uint8_t> featureIDs{1, 2, 0, 7};

  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();
  Sampler& sampler = model.samplers.emplace_back();
  sampler.wrapS = Sampler::WrapS::CLAMP_TO_EDGE;
  sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

  Image& image = model.images.emplace_back();
  image.pAsset.emplace();
  image.pAsset->width = 2;
  image.pAsset->height = 2;
  image.pAsset->channels = 1;
  image.pAsset->bytesPerChannel = 1;
  image.pAsset->pixelData.resize(featureIDs.size());
  std::memcpy(
      image.pAsset->pixelData.data(),
      featureIDs.data(),
      featureIDs.size());

  Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = 0;

  ExtensionExtMeshFeatures& meshFeatures =
      primitive.addExtension<ExtensionExtMeshFeatures>();

  FeatureIdTexture featureIdTexture;
  featureIdTexture.index = 0;
  featureIdTexture.texCoord = 0;
  featureIdTexture.channels = {0};

  FeatureId featureId = meshFeatures.featureIds.emplace_back();
  featureId.texture = featureIdTexture;

  TextureViewOptions options;
  options.makeImageCopy = true;
  FeatureIdTextureView view(model, featureIdTexture, options);
  REQUIRE(view.status() == FeatureIdTextureViewStatus::Valid);

  // Clear the original image data.
  std::vector<std::byte> emptyData;
  image.pAsset->pixelData.swap(emptyData);

  const ImageAsset* pImage = view.getImage();
  REQUIRE(pImage);
  REQUIRE(pImage->width == image.pAsset->width);
  REQUIRE(pImage->height == image.pAsset->height);
  REQUIRE(pImage->channels == image.pAsset->channels);
  REQUIRE(pImage->bytesPerChannel == image.pAsset->bytesPerChannel);
  REQUIRE(pImage->pixelData.size() == featureIDs.size());
}

TEST_CASE("Test getFeatureID on invalid feature ID texture view") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();
  Sampler& sampler = model.samplers.emplace_back();
  sampler.wrapS = Sampler::WrapS::CLAMP_TO_EDGE;
  sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

  Image& image = model.images.emplace_back();
  image.pAsset.emplace();
  image.pAsset->width = 1;
  image.pAsset->height = 1;

  Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = 0;

  ExtensionExtMeshFeatures& meshFeatures =
      primitive.addExtension<ExtensionExtMeshFeatures>();

  FeatureIdTexture featureIdTexture;
  featureIdTexture.index = 0;
  featureIdTexture.texCoord = 0;
  featureIdTexture.channels = {4};

  FeatureId featureId = meshFeatures.featureIds.emplace_back();
  featureId.texture = featureIdTexture;

  FeatureIdTextureView view(model, featureIdTexture);
  REQUIRE(view.status() == FeatureIdTextureViewStatus::ErrorInvalidChannels);
  REQUIRE(view.getFeatureID(0, 0) == -1);
}

TEST_CASE("Test getFeatureID on valid feature ID texture view") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();
  Sampler& sampler = model.samplers.emplace_back();
  sampler.wrapS = Sampler::WrapS::CLAMP_TO_EDGE;
  sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

  std::vector<uint8_t> featureIDs{1, 2, 0, 7};

  Image& image = model.images.emplace_back();
  image.pAsset.emplace();
  image.pAsset->width = 2;
  image.pAsset->height = 2;
  image.pAsset->channels = 1;
  image.pAsset->bytesPerChannel = 1;
  image.pAsset->pixelData.resize(featureIDs.size());
  std::memcpy(
      image.pAsset->pixelData.data(),
      featureIDs.data(),
      featureIDs.size());

  Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = 0;

  ExtensionExtMeshFeatures& meshFeatures =
      primitive.addExtension<ExtensionExtMeshFeatures>();

  FeatureIdTexture featureIdTexture;
  featureIdTexture.index = 0;
  featureIdTexture.texCoord = 0;
  featureIdTexture.channels = {0};

  FeatureId featureId = meshFeatures.featureIds.emplace_back();
  featureId.texture = featureIdTexture;

  FeatureIdTextureView view(model, featureIdTexture);
  REQUIRE(view.status() == FeatureIdTextureViewStatus::Valid);
  REQUIRE(view.getFeatureID(0, 0) == 1);
  REQUIRE(view.getFeatureID(1, 0) == 2);
  REQUIRE(view.getFeatureID(0, 1) == 0);
  REQUIRE(view.getFeatureID(1, 1) == 7);
}

TEST_CASE("Test getFeatureID on view with applyKhrTextureTransformExtension = "
          "false") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();
  Sampler& sampler = model.samplers.emplace_back();
  sampler.wrapS = Sampler::WrapS::CLAMP_TO_EDGE;
  sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

  std::vector<uint8_t> featureIDs{1, 2, 0, 7};

  Image& image = model.images.emplace_back();
  image.pAsset.emplace();
  image.pAsset->width = 2;
  image.pAsset->height = 2;
  image.pAsset->channels = 1;
  image.pAsset->bytesPerChannel = 1;
  image.pAsset->pixelData.resize(featureIDs.size());
  std::memcpy(
      image.pAsset->pixelData.data(),
      featureIDs.data(),
      featureIDs.size());

  Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = 0;

  ExtensionExtMeshFeatures& meshFeatures =
      primitive.addExtension<ExtensionExtMeshFeatures>();

  FeatureIdTexture featureIdTexture;
  featureIdTexture.index = 0;
  featureIdTexture.texCoord = 0;
  featureIdTexture.channels = {0};

  ExtensionKhrTextureTransform& textureTransformExtension =
      featureIdTexture.addExtension<ExtensionKhrTextureTransform>();
  textureTransformExtension.offset = {0.5, -0.5};
  textureTransformExtension.rotation = CesiumUtility::Math::PiOverTwo;
  textureTransformExtension.scale = {0.5, 0.5};

  FeatureId featureId = meshFeatures.featureIds.emplace_back();
  featureId.texture = featureIdTexture;

  FeatureIdTextureView view(model, featureIdTexture);
  REQUIRE(view.status() == FeatureIdTextureViewStatus::Valid);
  REQUIRE(view.getFeatureID(0, 0) == 1);
  REQUIRE(view.getFeatureID(1, 0) == 2);
  REQUIRE(view.getFeatureID(0, 1) == 0);
  REQUIRE(view.getFeatureID(1, 1) == 7);
}

TEST_CASE(
    "Test getFeatureID on view with applyKhrTextureTransformExtension = true") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();
  Sampler& sampler = model.samplers.emplace_back();
  sampler.wrapS = Sampler::WrapS::REPEAT;
  sampler.wrapT = Sampler::WrapT::REPEAT;

  std::vector<uint8_t> featureIDs{1, 2, 0, 7};

  Image& image = model.images.emplace_back();
  image.pAsset.emplace();
  image.pAsset->width = 2;
  image.pAsset->height = 2;
  image.pAsset->channels = 1;
  image.pAsset->bytesPerChannel = 1;
  image.pAsset->pixelData.resize(featureIDs.size());
  std::memcpy(
      image.pAsset->pixelData.data(),
      featureIDs.data(),
      featureIDs.size());

  Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = 0;

  ExtensionExtMeshFeatures& meshFeatures =
      primitive.addExtension<ExtensionExtMeshFeatures>();

  FeatureIdTexture featureIdTexture;
  featureIdTexture.index = 0;
  featureIdTexture.texCoord = 0;
  featureIdTexture.channels = {0};

  ExtensionKhrTextureTransform& textureTransformExtension =
      featureIdTexture.addExtension<ExtensionKhrTextureTransform>();
  textureTransformExtension.offset = {0.5, -0.5};
  textureTransformExtension.rotation = CesiumUtility::Math::PiOverTwo;
  textureTransformExtension.scale = {0.5, 0.5};

  FeatureId featureId = meshFeatures.featureIds.emplace_back();
  featureId.texture = featureIdTexture;

  TextureViewOptions options;
  options.applyKhrTextureTransformExtension = true;
  FeatureIdTextureView view(model, featureIdTexture, options);
  REQUIRE(view.status() == FeatureIdTextureViewStatus::Valid);
  // (0, 0) -> (0.5, -0.5) -> wraps to (0.5, 0.5)
  // (1, 0) -> (0.5, -1) -> wraps to (0.5, 0)
  // (0, 1) -> (1, -0.5) -> wraps to (0, 0.5)
  // (1, 1) -> (1, -1) -> wraps to (0.0, 0.0)
  REQUIRE(view.getFeatureID(0, 0) == 7);
  REQUIRE(view.getFeatureID(1, 0) == 2);
  REQUIRE(view.getFeatureID(0, 1) == 0);
  REQUIRE(view.getFeatureID(1, 1) == 1);
}

TEST_CASE("Test getFeatureId on view with makeImageCopy = true") {
  std::vector<uint8_t> featureIDs{1, 2, 0, 7};

  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();
  Sampler& sampler = model.samplers.emplace_back();
  sampler.wrapS = Sampler::WrapS::CLAMP_TO_EDGE;
  sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

  Image& image = model.images.emplace_back();
  image.pAsset.emplace();
  image.pAsset->width = 2;
  image.pAsset->height = 2;
  image.pAsset->channels = 1;
  image.pAsset->bytesPerChannel = 1;
  image.pAsset->pixelData.resize(featureIDs.size());
  std::memcpy(
      image.pAsset->pixelData.data(),
      featureIDs.data(),
      featureIDs.size());

  Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = 0;

  ExtensionExtMeshFeatures& meshFeatures =
      primitive.addExtension<ExtensionExtMeshFeatures>();

  FeatureIdTexture featureIdTexture;
  featureIdTexture.index = 0;
  featureIdTexture.texCoord = 0;
  featureIdTexture.channels = {0};

  FeatureId featureId = meshFeatures.featureIds.emplace_back();
  featureId.texture = featureIdTexture;

  TextureViewOptions options;
  options.makeImageCopy = true;
  FeatureIdTextureView view(model, featureIdTexture, options);
  REQUIRE(view.status() == FeatureIdTextureViewStatus::Valid);

  // Clear the original image data.
  std::vector<std::byte> emptyData;
  image.pAsset->pixelData.swap(emptyData);

  REQUIRE(view.getFeatureID(0, 0) == 1);
  REQUIRE(view.getFeatureID(1, 0) == 2);
  REQUIRE(view.getFeatureID(0, 1) == 0);
  REQUIRE(view.getFeatureID(1, 1) == 7);
}

TEST_CASE("Test getFeatureID rounds to nearest pixel") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();
  Sampler& sampler = model.samplers.emplace_back();
  sampler.wrapS = Sampler::WrapS::CLAMP_TO_EDGE;
  sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

  std::vector<uint8_t> featureIDs{1, 2, 0, 7};

  Image& image = model.images.emplace_back();
  image.pAsset.emplace();
  image.pAsset->width = 2;
  image.pAsset->height = 2;
  image.pAsset->channels = 1;
  image.pAsset->bytesPerChannel = 1;
  image.pAsset->pixelData.resize(featureIDs.size());
  std::memcpy(
      image.pAsset->pixelData.data(),
      featureIDs.data(),
      featureIDs.size());

  Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = 0;

  ExtensionExtMeshFeatures& meshFeatures =
      primitive.addExtension<ExtensionExtMeshFeatures>();

  FeatureIdTexture featureIdTexture;
  featureIdTexture.index = 0;
  featureIdTexture.texCoord = 0;
  featureIdTexture.channels = {0};

  FeatureId featureId = meshFeatures.featureIds.emplace_back();
  featureId.texture = featureIdTexture;

  FeatureIdTextureView view(model, featureIdTexture);
  REQUIRE(view.status() == FeatureIdTextureViewStatus::Valid);
  REQUIRE(view.getFeatureID(0.1, 0.4) == 1);
  REQUIRE(view.getFeatureID(0.86, 0.2) == 2);
  REQUIRE(view.getFeatureID(0.29, 0.555) == 0);
  REQUIRE(view.getFeatureID(0.99, 0.81) == 7);
}

TEST_CASE("Test getFeatureID clamps values") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();
  Sampler& sampler = model.samplers.emplace_back();
  sampler.wrapS = Sampler::WrapS::CLAMP_TO_EDGE;
  sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

  std::vector<uint8_t> featureIDs{1, 2, 0, 7};

  Image& image = model.images.emplace_back();
  image.pAsset.emplace();
  image.pAsset->width = 2;
  image.pAsset->height = 2;
  image.pAsset->channels = 1;
  image.pAsset->bytesPerChannel = 1;

  auto& data = image.pAsset->pixelData;
  data.resize(featureIDs.size() * sizeof(uint8_t));
  std::memcpy(data.data(), featureIDs.data(), data.size());

  Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = 0;

  ExtensionExtMeshFeatures& meshFeatures =
      primitive.addExtension<ExtensionExtMeshFeatures>();

  FeatureIdTexture featureIdTexture;
  featureIdTexture.index = 0;
  featureIdTexture.texCoord = 0;
  featureIdTexture.channels = {0};

  FeatureId featureId = meshFeatures.featureIds.emplace_back();
  featureId.texture = featureIdTexture;

  FeatureIdTextureView view(model, featureIdTexture);
  REQUIRE(view.status() == FeatureIdTextureViewStatus::Valid);
  REQUIRE(view.getFeatureID(-1, -1) == 1);
  REQUIRE(view.getFeatureID(2, 0) == 2);
  REQUIRE(view.getFeatureID(-1, 2) == 0);
  REQUIRE(view.getFeatureID(3, 4) == 7);
}

TEST_CASE("Test getFeatureID handles multiple channels") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();
  Sampler& sampler = model.samplers.emplace_back();
  sampler.wrapS = Sampler::WrapS::CLAMP_TO_EDGE;
  sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

  std::vector<uint16_t> featureIDs{260, 512, 8, 17};

  Image& image = model.images.emplace_back();
  image.pAsset.emplace();
  image.pAsset->width = 2;
  image.pAsset->height = 2;
  image.pAsset->channels = 2;
  image.pAsset->bytesPerChannel = 1;

  auto& data = image.pAsset->pixelData;
  data.resize(featureIDs.size() * sizeof(uint16_t));
  std::memcpy(data.data(), featureIDs.data(), data.size());

  Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = 0;

  ExtensionExtMeshFeatures& meshFeatures =
      primitive.addExtension<ExtensionExtMeshFeatures>();

  FeatureIdTexture featureIdTexture;
  featureIdTexture.index = 0;
  featureIdTexture.texCoord = 0;
  featureIdTexture.channels = {0, 1};

  FeatureId featureId = meshFeatures.featureIds.emplace_back();
  featureId.texture = featureIdTexture;

  FeatureIdTextureView view(model, featureIdTexture);
  REQUIRE(view.status() == FeatureIdTextureViewStatus::Valid);
  REQUIRE(view.getFeatureID(0, 0) == 260);
  REQUIRE(view.getFeatureID(1, 0) == 512);
  REQUIRE(view.getFeatureID(0, 1) == 8);
  REQUIRE(view.getFeatureID(1, 1) == 17);
}

TEST_CASE("Check FeatureIdTextureView sampling with different wrap values") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();
  Sampler& sampler = model.samplers.emplace_back();

  std::vector<uint8_t> featureIDs{1, 2, 0, 7};

  Image& image = model.images.emplace_back();
  image.pAsset.emplace();
  image.pAsset->width = 2;
  image.pAsset->height = 2;
  image.pAsset->channels = 1;
  image.pAsset->bytesPerChannel = 1;

  auto& data = image.pAsset->pixelData;
  data.resize(featureIDs.size() * sizeof(uint8_t));
  std::memcpy(data.data(), featureIDs.data(), data.size());

  Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = 0;

  ExtensionExtMeshFeatures& meshFeatures =
      primitive.addExtension<ExtensionExtMeshFeatures>();

  FeatureIdTexture featureIdTexture;
  featureIdTexture.index = 0;
  featureIdTexture.texCoord = 0;
  featureIdTexture.channels = {0};

  FeatureId featureId = meshFeatures.featureIds.emplace_back();
  featureId.texture = featureIdTexture;

  FeatureIdTextureView view(model, featureIdTexture);

  SUBCASE("REPEAT") {
    sampler.wrapS = Sampler::WrapS::REPEAT;
    sampler.wrapT = Sampler::WrapT::REPEAT;

    std::vector<glm::dvec2> uvs{
        glm::dvec2(1.0, 0),
        glm::dvec2(-1.5, 0),
        glm::dvec2(0, -0.5),
        glm::dvec2(1.5, -0.5)};
    for (size_t i = 0; i < uvs.size(); i++) {
      glm::dvec2 uv = uvs[i];

      REQUIRE(view.getFeatureID(uv[0], uv[1]) == static_cast<int64_t>(data[i]));
    }
  }

  SUBCASE("MIRRORED_REPEAT") {
    sampler.wrapS = Sampler::WrapS::MIRRORED_REPEAT;
    sampler.wrapT = Sampler::WrapT::MIRRORED_REPEAT;

    // REPEAT:   | 1 2 3 | 1 2 3 |
    // MIRRORED: | 1 2 3 | 3 2 1 |
    // Sampling 0.6 is equal to sampling 1.4 or -0.6.

    std::vector<glm::dvec2> uvs{
        glm::dvec2(2.0, 0),
        glm::dvec2(-0.75, 0),
        glm::dvec2(0, 1.25),
        glm::dvec2(-1.25, 2.75)};
    for (size_t i = 0; i < uvs.size(); i++) {
      glm::dvec2 uv = uvs[i];

      REQUIRE(view.getFeatureID(uv[0], uv[1]) == static_cast<int64_t>(data[i]));
    }
  }

  SUBCASE("CLAMP_TO_EDGE") {
    sampler.wrapS = Sampler::WrapS::CLAMP_TO_EDGE;
    sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

    std::vector<glm::dvec2> uvs{
        glm::dvec2(-1.0, 0),
        glm::dvec2(1.4, 0),
        glm::dvec2(0, 2.0),
        glm::dvec2(1.5, 1.5)};
    for (size_t i = 0; i < uvs.size(); i++) {
      glm::dvec2 uv = uvs[i];

      REQUIRE(view.getFeatureID(uv[0], uv[1]) == static_cast<int64_t>(data[i]));
    }
  }

  SUBCASE("Mismatched wrap values") {
    sampler.wrapS = Sampler::WrapS::REPEAT;
    sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

    std::vector<glm::dvec2> uvs{
        glm::dvec2(1.0, 0),
        glm::dvec2(-1.5, -1.0),
        glm::dvec2(0, 1.5),
        glm::dvec2(1.5, 1.5)};
    for (size_t i = 0; i < uvs.size(); i++) {
      glm::dvec2 uv = uvs[i];

      REQUIRE(view.getFeatureID(uv[0], uv[1]) == static_cast<int64_t>(data[i]));
    }
  }
}
