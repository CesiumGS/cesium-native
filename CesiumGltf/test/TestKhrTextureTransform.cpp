#include <CesiumGltf/ExtensionKhrTextureTransform.h>
#include <CesiumGltf/KhrTextureTransform.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>

using namespace CesiumGltf;

TEST_CASE("Test KhrTextureTransform default constructor") {
  KhrTextureTransform textureTransform;
  REQUIRE(textureTransform.status() == KhrTextureTransformStatus::Valid);
  REQUIRE(textureTransform.offset() == glm::dvec2{0.0, 0.0});
  REQUIRE(textureTransform.rotation() == 0.0);
  REQUIRE(textureTransform.rotationSineCosine() == glm::dvec2{0.0, 1.0});
  REQUIRE(textureTransform.scale() == glm::dvec2{1.0, 1.0});
}

TEST_CASE("Test KhrTextureTransform constructs with valid extension") {
  ExtensionKhrTextureTransform extension;
  extension.offset = {5, 12};
  extension.rotation = CesiumUtility::Math::PiOverTwo;
  extension.scale = {2.0, 0.5};

  KhrTextureTransform textureTransform(extension);
  REQUIRE(textureTransform.status() == KhrTextureTransformStatus::Valid);
  REQUIRE(textureTransform.offset() == glm::dvec2{5, 12});
  REQUIRE(textureTransform.rotation() == CesiumUtility::Math::PiOverTwo);
  REQUIRE(textureTransform.scale() == glm::dvec2{2.0, 0.5});
  glm::dvec2 sineCosine = textureTransform.rotationSineCosine();
  REQUIRE(CesiumUtility::Math::equalsEpsilon(
      sineCosine.x,
      1.0,
      CesiumUtility::Math::Epsilon6));
  REQUIRE(CesiumUtility::Math::equalsEpsilon(
      sineCosine.y,
      0.0,
      CesiumUtility::Math::Epsilon6));
}

TEST_CASE("Test KhrTextureTransform reports invalid offset") {
  ExtensionKhrTextureTransform extension;
  extension.offset = {5};

  KhrTextureTransform textureTransform(extension);
  REQUIRE(
      textureTransform.status() ==
      KhrTextureTransformStatus::ErrorInvalidOffset);
}

TEST_CASE("Test KhrTextureTransform reports invalid scale") {
  ExtensionKhrTextureTransform extension;
  extension.scale = {5};

  KhrTextureTransform textureTransform(extension);
  REQUIRE(
      textureTransform.status() ==
      KhrTextureTransformStatus::ErrorInvalidScale);
}

TEST_CASE("Test KhrTextureTransform applies identity transform") {
  KhrTextureTransform textureTransform;
  REQUIRE(textureTransform.status() == KhrTextureTransformStatus::Valid);
  REQUIRE(textureTransform.applyTransform(0, 0) == glm::dvec2(0, 0));
  REQUIRE(textureTransform.applyTransform(0.5, 1) == glm::dvec2(0.5, 1));
}

TEST_CASE("Test KhrTextureTransform applies non-identity transform") {
  ExtensionKhrTextureTransform extension;
  extension.offset = {5, 12};
  extension.rotation = CesiumUtility::Math::PiOverTwo;
  extension.scale = {2.0, 0.5};

  KhrTextureTransform textureTransform(extension);
  glm::dvec2 transformedUv = textureTransform.applyTransform(0.0, 0.0);
  REQUIRE(CesiumUtility::Math::equalsEpsilon(
      transformedUv.x,
      5.0,
      CesiumUtility::Math::Epsilon6));
  REQUIRE(CesiumUtility::Math::equalsEpsilon(
      transformedUv.y,
      12.0,
      CesiumUtility::Math::Epsilon6));

  transformedUv = textureTransform.applyTransform(1.0, 1.0);
  // scaled = (2.0, 0.5)
  // rotated = (0.5, -2.0)
  // translated = (5.5, 10.0)
  REQUIRE(CesiumUtility::Math::equalsEpsilon(
      transformedUv.x,
      5.5,
      CesiumUtility::Math::Epsilon6));
  REQUIRE(CesiumUtility::Math::equalsEpsilon(
      transformedUv.y,
      10.0,
      CesiumUtility::Math::Epsilon6));
}
