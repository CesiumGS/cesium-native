#include <CesiumGltfContent/SkirtMeshMetadata.h>
#include <CesiumUtility/JsonValue.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>

#include <optional>

using namespace CesiumGltfContent;
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
  const auto* pNoSkirtRange =
      gltfSkirt.getValuePtrForKey<JsonValue::Array>("noSkirtRange");
  REQUIRE(pNoSkirtRange);
  REQUIRE((*pNoSkirtRange)[0].getSafeNumberOrDefault<double>(-1.0) == 0.0);
  REQUIRE((*pNoSkirtRange)[1].getSafeNumberOrDefault<double>(-1.0) == 12.0);

  const auto* pMeshCenter =
      gltfSkirt.getValuePtrForKey<JsonValue::Array>("meshCenter");
  REQUIRE(Math::equalsEpsilon(
      (*pMeshCenter)[0].getSafeNumberOrDefault<double>(0.0),
      skirtMeshMetadata.meshCenter.x,
      Math::Epsilon7));
  REQUIRE(Math::equalsEpsilon(
      (*pMeshCenter)[1].getSafeNumberOrDefault<double>(0.0),
      skirtMeshMetadata.meshCenter.y,
      Math::Epsilon7));
  REQUIRE(Math::equalsEpsilon(
      (*pMeshCenter)[2].getSafeNumberOrDefault<double>(0.0),
      skirtMeshMetadata.meshCenter.z,
      Math::Epsilon7));

  const std::optional<double> maybeSkirtWestHeight =
      gltfSkirt.getSafeNumericalValueForKey<double>("skirtWestHeight");
  REQUIRE(maybeSkirtWestHeight);
  REQUIRE(Math::equalsEpsilon(
      *maybeSkirtWestHeight,
      skirtMeshMetadata.skirtWestHeight,
      Math::Epsilon7));

  const std::optional<double> maybeSkirtSouthHeight =
      gltfSkirt.getSafeNumericalValueForKey<double>("skirtSouthHeight");
  REQUIRE(maybeSkirtSouthHeight);
  REQUIRE(Math::equalsEpsilon(
      *maybeSkirtSouthHeight,
      skirtMeshMetadata.skirtSouthHeight,
      Math::Epsilon7));

  const std::optional<double> maybeSkirtEastHeight =
      gltfSkirt.getSafeNumericalValueForKey<double>("skirtEastHeight");
  REQUIRE(maybeSkirtEastHeight);
  REQUIRE(Math::equalsEpsilon(
      *maybeSkirtEastHeight,
      skirtMeshMetadata.skirtEastHeight,
      Math::Epsilon7));

  const std::optional<double> maybeSkirtNorthHeight =
      gltfSkirt.getSafeNumericalValueForKey<double>("skirtNorthHeight");
  REQUIRE(maybeSkirtNorthHeight);
  REQUIRE(Math::equalsEpsilon(
      *maybeSkirtNorthHeight,
      skirtMeshMetadata.skirtNorthHeight,
      Math::Epsilon7));
}

TEST_CASE("Test converting gltf extras to skirt mesh metadata") {
  // mock gltf extras for skirt mesh metadata
  JsonValue::Object gltfSkirtMeshMetadata = {
      {"noSkirtRange", JsonValue::Array{0, 12, 24, 48}},
      {"meshCenter", JsonValue::Array{1.0, 2.0, 3.0}},
      {"skirtWestHeight", 12.4},
      {"skirtSouthHeight", 10.0},
      {"skirtEastHeight", 2.4},
      {"skirtNorthHeight", 1.4}};

  SUBCASE("Gltf Extras has correct format") {
    JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

    SkirtMeshMetadata skirtMeshMetadata =
        *SkirtMeshMetadata::parseFromGltfExtras(extras);

    REQUIRE(skirtMeshMetadata.noSkirtIndicesBegin == 0);
    REQUIRE(skirtMeshMetadata.noSkirtIndicesCount == 12);
    REQUIRE(skirtMeshMetadata.noSkirtVerticesBegin == 24);
    REQUIRE(skirtMeshMetadata.noSkirtVerticesCount == 48);
    REQUIRE(Math::equalsEpsilon(
        skirtMeshMetadata.meshCenter.x,
        1.0,
        Math::Epsilon7));
    REQUIRE(Math::equalsEpsilon(
        skirtMeshMetadata.meshCenter.y,
        2.0,
        Math::Epsilon7));
    REQUIRE(Math::equalsEpsilon(
        skirtMeshMetadata.meshCenter.z,
        3.0,
        Math::Epsilon7));
    REQUIRE(Math::equalsEpsilon(
        skirtMeshMetadata.skirtWestHeight,
        12.4,
        Math::Epsilon7));
    REQUIRE(Math::equalsEpsilon(
        skirtMeshMetadata.skirtSouthHeight,
        10.0,
        Math::Epsilon7));
    REQUIRE(Math::equalsEpsilon(
        skirtMeshMetadata.skirtEastHeight,
        2.4,
        Math::Epsilon7));
    REQUIRE(Math::equalsEpsilon(
        skirtMeshMetadata.skirtNorthHeight,
        1.4,
        Math::Epsilon7));
  }

  SUBCASE("Gltf Extras has incorrect noSkirtRange field") {
    SUBCASE("missing noSkirtRange field") {
      gltfSkirtMeshMetadata.erase("noSkirtRange");

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }

    SUBCASE("noSkirtRange field has wrong type") {
      gltfSkirtMeshMetadata["noSkirtRange"] = 12;

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }

    SUBCASE("noSkirtRange field has only one element array") {
      gltfSkirtMeshMetadata["noSkirtRange"] = JsonValue::Array{0};

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }

    SUBCASE("noSkirtRange field has two elements array but not integer") {
      gltfSkirtMeshMetadata["noSkirtRange"] =
          JsonValue::Array{"first", "second"};

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }
  }

  SUBCASE("Gltf Extras has incorrect meshCenter field") {
    SUBCASE("missing meshCenter field") {
      gltfSkirtMeshMetadata.erase("meshCenter");

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }

    SUBCASE("meshCenter field has wrong type") {
      gltfSkirtMeshMetadata["meshCenter"] = 12;

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }

    SUBCASE("meshCenter field is 2 elements array") {
      gltfSkirtMeshMetadata["meshCenter"] = JsonValue::Array{1.0, 2.0};

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }

    SUBCASE("meshCenter field is 3 elements array but wrong type") {
      gltfSkirtMeshMetadata["meshCenter"] = JsonValue::Array{1.0, 2.0, "third"};

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }
  }

  SUBCASE("Gltf Extras has incorrect skirtWestHeight field") {
    SUBCASE("missing skirtWestHeight field") {
      gltfSkirtMeshMetadata.erase("skirtWestHeight");

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }

    SUBCASE("skirtWestHeight field has wrong type") {
      gltfSkirtMeshMetadata["skirtWestHeight"] = "string";

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }
  }

  SUBCASE("Gltf Extras has incorrect skirtSouthHeight field") {
    SUBCASE("missing skirtSouthHeight field") {
      gltfSkirtMeshMetadata.erase("skirtSouthHeight");

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }

    SUBCASE("skirtSouthHeight field has wrong type") {
      gltfSkirtMeshMetadata["skirtSouthHeight"] = "string";

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }
  }

  SUBCASE("Gltf Extras has incorrect skirtEastHeight field") {
    SUBCASE("missing skirtEastHeight field") {
      gltfSkirtMeshMetadata.erase("skirtEastHeight");

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }

    SUBCASE("skirtEastHeight field has wrong type") {
      gltfSkirtMeshMetadata["skirtEastHeight"] = "string";

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }
  }

  SUBCASE("Gltf Extras has incorrect skirtNorthHeight field") {
    SUBCASE("missing skirtNorthHeight field") {
      gltfSkirtMeshMetadata.erase("skirtNorthHeight");

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }

    SUBCASE("skirtNorthHeight field has wrong type") {
      gltfSkirtMeshMetadata["skirtNorthHeight"] = "string";

      JsonValue::Object extras = {{"skirtMeshMetadata", gltfSkirtMeshMetadata}};

      REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
    }
  }
}
