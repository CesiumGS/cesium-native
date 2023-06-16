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

  ExtensionExtMeshFeaturesFeatureIdTexture featureIdTexture;
  featureIdTexture.index = -1;
  featureIdTexture.texCoord = 0;
  featureIdTexture.channels = {0};

  ExtensionExtMeshFeaturesFeatureId featureId =
      meshFeatures.featureIds.emplace_back();
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

  ExtensionExtMeshFeaturesFeatureIdTexture featureIdTexture;
  featureIdTexture.index = 0;
  featureIdTexture.texCoord = 0;
  featureIdTexture.channels = {0};

  ExtensionExtMeshFeaturesFeatureId featureId =
      meshFeatures.featureIds.emplace_back();
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

  ExtensionExtMeshFeaturesFeatureIdTexture featureIdTexture;
  featureIdTexture.index = 0;
  featureIdTexture.texCoord = 0;
  featureIdTexture.channels = {0};

  ExtensionExtMeshFeaturesFeatureId featureId =
      meshFeatures.featureIds.emplace_back();
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

  ExtensionExtMeshFeaturesFeatureIdTexture featureIdTexture;
  featureIdTexture.index = 0;
  featureIdTexture.texCoord = 0;
  featureIdTexture.channels = {0};

  ExtensionExtMeshFeaturesFeatureId featureId =
      meshFeatures.featureIds.emplace_back();
  featureId.texture = featureIdTexture;

  FeatureIdTextureView view(model, featureIdTexture);
  REQUIRE(
      view.status() ==
      FeatureIdTextureViewStatus::ErrorInvalidImageBytesPerChannel);
}

TEST_CASE(
    "Test FeatureIdTextureView on feature ID texture with negative texcoord "
    "set index") {
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

  ExtensionExtMeshFeaturesFeatureIdTexture featureIdTexture;
  featureIdTexture.index = 0;
  featureIdTexture.texCoord = -1;
  featureIdTexture.channels = {0};

  ExtensionExtMeshFeaturesFeatureId featureId =
      meshFeatures.featureIds.emplace_back();
  featureId.texture = featureIdTexture;

  FeatureIdTextureView view(model, featureIdTexture);
  REQUIRE(
      view.status() ==
      FeatureIdTextureViewStatus::ErrorInvalidTexCoordSetIndex);
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

  ExtensionExtMeshFeaturesFeatureIdTexture featureIdTexture;
  featureIdTexture.index = 0;
  featureIdTexture.texCoord = 0;
  featureIdTexture.channels = {};

  ExtensionExtMeshFeaturesFeatureId featureId =
      meshFeatures.featureIds.emplace_back();
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

  ExtensionExtMeshFeaturesFeatureIdTexture featureIdTexture;
  featureIdTexture.index = 0;
  featureIdTexture.texCoord = 0;
  featureIdTexture.channels = {0, 1, 2, 3, 3};

  ExtensionExtMeshFeaturesFeatureId featureId =
      meshFeatures.featureIds.emplace_back();
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

  ExtensionExtMeshFeaturesFeatureIdTexture featureIdTexture;
  featureIdTexture.index = 0;
  featureIdTexture.texCoord = 0;
  featureIdTexture.channels = {4};

  ExtensionExtMeshFeaturesFeatureId featureId =
      meshFeatures.featureIds.emplace_back();
  featureId.texture = featureIdTexture;

  FeatureIdTextureView view(model, featureIdTexture);
  REQUIRE(view.status() == FeatureIdTextureViewStatus::ErrorInvalidChannels);
}
