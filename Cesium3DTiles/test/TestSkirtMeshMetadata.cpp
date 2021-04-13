#include "CesiumUtility/Math.h"
#include "SkirtMeshMetadata.h"
#include "catch2/catch.hpp"

using namespace Cesium3DTiles;
using namespace CesiumUtility;

TEST_CASE("Test converting skirt mesh metadata to gltf extras") {
  SkirtMeshMetadata skirtMeshMetadata;
  skirtMeshMetadata.noSkirtIndicesBegin = 0;
  skirtMeshMetadata.noSkirtIndicesCount = 12;
  skirtMeshMetadata.meshCenter = glm::dvec3(23.4, 12.3, 11.0);
  skirtMeshMetadata.skirtWestHeight = 12.2;
  skirtMeshMetadata.skirtSouthHeight = 0.2;
  skirtMeshMetadata.skirtEastHeight = 24.2;
  skirtMeshMetadata.skirtNorthHeight = 10.0;

  JsonValue::Object extras =
      SkirtMeshMetadata::createGltfExtras(skirtMeshMetadata);
  REQUIRE(extras.find("skirtMeshMetadata") != extras.end());

  JsonValue& gltfSkirt = extras["skirtMeshMetadata"];
  JsonValue::Array* pNoSkirtRange =
      gltfSkirt.getValueForKey<JsonValue::Array>("noSkirtRange");
  REQUIRE(pNoSkirtRange != nullptr);
  REQUIRE((*pNoSkirtRange)[0].getNumber(-1.0) == 0.0);
  REQUIRE((*pNoSkirtRange)[1].getNumber(-1.0) == 12.0);

  JsonValue::Array* pMeshCenter =
      gltfSkirt.getValueForKey<JsonValue::Array>("meshCenter");
  REQUIRE(Math::equalsEpsilon(
      (*pMeshCenter)[0].getNumber(0.0),
      skirtMeshMetadata.meshCenter.x,
      Math::EPSILON7));
  REQUIRE(Math::equalsEpsilon(
      (*pMeshCenter)[1].getNumber(0.0),
      skirtMeshMetadata.meshCenter.y,
      Math::EPSILON7));
  REQUIRE(Math::equalsEpsilon(
      (*pMeshCenter)[2].getNumber(0.0),
      skirtMeshMetadata.meshCenter.z,
      Math::EPSILON7));

  double* pSkirtWestHeight =
      gltfSkirt.getValueForKey<JsonValue::Number>("skirtWestHeight");
  REQUIRE(Math::equalsEpsilon(
      *pSkirtWestHeight,
      skirtMeshMetadata.skirtWestHeight,
      Math::EPSILON7));

  double* pSkirtSouthHeight =
      gltfSkirt.getValueForKey<JsonValue::Number>("skirtSouthHeight");
  REQUIRE(Math::equalsEpsilon(
      *pSkirtSouthHeight,
      skirtMeshMetadata.skirtSouthHeight,
      Math::EPSILON7));

  double* pSkirtEastHeight =
      gltfSkirt.getValueForKey<JsonValue::Number>("skirtEastHeight");
  REQUIRE(Math::equalsEpsilon(
      *pSkirtEastHeight,
      skirtMeshMetadata.skirtEastHeight,
      Math::EPSILON7));

  double* pSkirtNorthHeight =
      gltfSkirt.getValueForKey<JsonValue::Number>("skirtNorthHeight");
  REQUIRE(Math::equalsEpsilon(
      *pSkirtNorthHeight,
      skirtMeshMetadata.skirtNorthHeight,
      Math::EPSILON7));
}

TEST_CASE("Test converting gltf extras to skirt mesh metadata") {
  // mock gltf extras for skirt mesh metadata
  JsonValue::Object gltfSkirtMeshMetadata = {
      {"noSkirtRange", JsonValue::Array{0, 12}},
      {"meshCenter", JsonValue::Array{1.0, 2.0, 3.0}},
      {"skirtWestHeight", 12.4},
      {"skirtSouthHeight", 10.0},
      {"skirtEastHeight", 2.4},
      {"skirtNorthHeight", 1.4}};

  SECTION("Gltf Extras has correct format") {
    JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

    SkirtMeshMetadata skirtMeshMetadata =
        *SkirtMeshMetadata::parseFromGltfExtras(extras);

    REQUIRE(skirtMeshMetadata.noSkirtIndicesBegin == 0);
    REQUIRE(skirtMeshMetadata.noSkirtIndicesCount == 12);
    REQUIRE(Math::equalsEpsilon(
        skirtMeshMetadata.meshCenter.x,
        1.0,
        Math::EPSILON7));
    REQUIRE(Math::equalsEpsilon(
        skirtMeshMetadata.meshCenter.y,
        2.0,
        Math::EPSILON7));
    REQUIRE(Math::equalsEpsilon(
        skirtMeshMetadata.meshCenter.z,
        3.0,
        Math::EPSILON7));
    REQUIRE(Math::equalsEpsilon(
        skirtMeshMetadata.skirtWestHeight,
        12.4,
        Math::EPSILON7));
    REQUIRE(Math::equalsEpsilon(
        skirtMeshMetadata.skirtSouthHeight,
        10.0,
        Math::EPSILON7));
    REQUIRE(Math::equalsEpsilon(
        skirtMeshMetadata.skirtEastHeight,
        2.4,
        Math::EPSILON7));
    REQUIRE(Math::equalsEpsilon(
        skirtMeshMetadata.skirtNorthHeight,
        1.4,
        Math::EPSILON7));
  }

  SECTION("Gltf Extras has incorrect noSkirtRange field") {
    SECTION("missing noSkirtRange field") {
      gltfSkirtMeshMetadata.erase("noSkirtRange");

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }

    SECTION("noSkirtRange field has wrong type") {
      gltfSkirtMeshMetadata["noSkirtRange"] = 12;

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }

    SECTION("noSkirtRange field has only one element array") {
      gltfSkirtMeshMetadata["noSkirtRange"] = JsonValue::Array{0};

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }

    SECTION("noSkirtRange field has two elements array but not integer") {
      gltfSkirtMeshMetadata["noSkirtRange"] =
          JsonValue::Array{"first", "second"};

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }
  }

  SECTION("Gltf Extras has incorrect meshCenter field") {
    SECTION("missing meshCenter field") {
      gltfSkirtMeshMetadata.erase("meshCenter");

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }

    SECTION("meshCenter field has wrong type") {
      gltfSkirtMeshMetadata["meshCenter"] = 12;

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }

    SECTION("meshCenter field is 2 elements array") {
      gltfSkirtMeshMetadata["meshCenter"] = JsonValue::Array{1.0, 2.0};

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }

    SECTION("meshCenter field is 3 elements array but wrong type") {
      gltfSkirtMeshMetadata["meshCenter"] = JsonValue::Array{1.0, 2.0, "third"};

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }
  }

  SECTION("Gltf Extras has incorrect skirtWestHeight field") {
    SECTION("missing skirtWestHeight field") {
      gltfSkirtMeshMetadata.erase("skirtWestHeight");

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }

    SECTION("skirtWestHeight field has wrong type") {
      gltfSkirtMeshMetadata["skirtWestHeight"] = "string";

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }
  }

  SECTION("Gltf Extras has incorrect skirtSouthHeight field") {
    SECTION("missing skirtSouthHeight field") {
      gltfSkirtMeshMetadata.erase("skirtSouthHeight");

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }

    SECTION("skirtSouthHeight field has wrong type") {
      gltfSkirtMeshMetadata["skirtSouthHeight"] = "string";

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }
  }

  SECTION("Gltf Extras has incorrect skirtEastHeight field") {
    SECTION("missing skirtEastHeight field") {
      gltfSkirtMeshMetadata.erase("skirtEastHeight");

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }

    SECTION("skirtEastHeight field has wrong type") {
      gltfSkirtMeshMetadata["skirtEastHeight"] = "string";

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }
  }

  SECTION("Gltf Extras has incorrect skirtNorthHeight field") {
    SECTION("missing skirtNorthHeight field") {
      gltfSkirtMeshMetadata.erase("skirtNorthHeight");

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }

    SECTION("skirtNorthHeight field has wrong type") {
      gltfSkirtMeshMetadata["skirtNorthHeight"] = "string";

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }
  }
}
