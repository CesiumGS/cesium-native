#include "CesiumGltf/ExtensionExtMeshFeatures.h"
#include "CesiumGltf/FeatureIdTextureView.h"

#include <catch2/catch.hpp>
#include <gsl/span>

#include <bitset>
#include <climits>
#include <cstddef>
#include <cstring>
#include <vector>

using namespace CesiumGltf;

TEST_CASE("Test FeatureIdTextureView on feature ID texture with invalid "
          "texture index") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

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

  Image& image = model.images.emplace_back();
  image.cesium.width = 0;
  image.cesium.height = 0;

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

  Image& image = model.images.emplace_back();
  image.cesium.width = 1;
  image.cesium.height = 1;
  image.cesium.bytesPerChannel = 2;

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

  Image& image = model.images.emplace_back();
  image.cesium.width = 1;
  image.cesium.height = 1;

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

  Image& image = model.images.emplace_back();
  image.cesium.width = 1;
  image.cesium.height = 1;

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

  Image& image = model.images.emplace_back();
  image.cesium.width = 1;
  image.cesium.height = 1;

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

  Image& image = model.images.emplace_back();
  image.cesium.width = 1;
  image.cesium.height = 1;

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

TEST_CASE("Test getFeatureID on invalid feature ID texture view") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

  Image& image = model.images.emplace_back();
  image.cesium.width = 1;
  image.cesium.height = 1;

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

  std::vector<uint8_t> featureIDs{1, 2, 0, 7};

  Image& image = model.images.emplace_back();
  image.cesium.width = 2;
  image.cesium.height = 2;
  image.cesium.channels = 1;
  image.cesium.bytesPerChannel = 1;
  image.cesium.pixelData.resize(featureIDs.size());
  std::memcpy(
      image.cesium.pixelData.data(),
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

TEST_CASE("Test getFeatureID rounds to nearest pixel") {
  Model model;
  Mesh& mesh = model.meshes.emplace_back();
  MeshPrimitive& primitive = mesh.primitives.emplace_back();

  std::vector<uint8_t> featureIDs{1, 2, 0, 7};

  Image& image = model.images.emplace_back();
  image.cesium.width = 2;
  image.cesium.height = 2;
  image.cesium.channels = 1;
  image.cesium.bytesPerChannel = 1;
  image.cesium.pixelData.resize(featureIDs.size());
  std::memcpy(
      image.cesium.pixelData.data(),
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

  std::vector<uint8_t> featureIDs{1, 2, 0, 7};

  Image& image = model.images.emplace_back();
  image.cesium.width = 2;
  image.cesium.height = 2;
  image.cesium.channels = 1;
  image.cesium.bytesPerChannel = 1;

  auto& data = image.cesium.pixelData;
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

  std::vector<uint16_t> featureIDs{260, 512, 8, 17};

  Image& image = model.images.emplace_back();
  image.cesium.width = 2;
  image.cesium.height = 2;
  image.cesium.channels = 2;
  image.cesium.bytesPerChannel = 1;

  auto& data = image.cesium.pixelData;
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
