#include "catch2/catch.hpp"
#include "CesiumUtility/Math.h"
#include "SkirtMeshMetadata.h"

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

	tinygltf::Value extras = SkirtMeshMetadata::createGltfExtras(skirtMeshMetadata);
	REQUIRE(extras.IsObject());
	REQUIRE(extras.Has("skirtMeshMetadata"));

	tinygltf::Value gltfSkirt = extras.Get("skirtMeshMetadata");
	tinygltf::Value noSkirtRange = gltfSkirt.Get("noSkirtRange");
	REQUIRE(noSkirtRange.Get(0).GetNumberAsInt() == 0);
	REQUIRE(noSkirtRange.Get(1).GetNumberAsInt() == 12);

	tinygltf::Value meshCenter = gltfSkirt.Get("meshCenter");
	REQUIRE(Math::equalsEpsilon(meshCenter.Get(0).GetNumberAsDouble(), skirtMeshMetadata.meshCenter.x, Math::EPSILON7));
	REQUIRE(Math::equalsEpsilon(meshCenter.Get(1).GetNumberAsDouble(), skirtMeshMetadata.meshCenter.y, Math::EPSILON7));
	REQUIRE(Math::equalsEpsilon(meshCenter.Get(2).GetNumberAsDouble(), skirtMeshMetadata.meshCenter.z, Math::EPSILON7));

	tinygltf::Value skirtWestHeight = gltfSkirt.Get("skirtWestHeight");
	REQUIRE(Math::equalsEpsilon(skirtWestHeight.GetNumberAsDouble(), skirtMeshMetadata.skirtWestHeight, Math::EPSILON7));

	tinygltf::Value skirtSouthHeight = gltfSkirt.Get("skirtSouthHeight");
	REQUIRE(Math::equalsEpsilon(skirtSouthHeight.GetNumberAsDouble(), skirtMeshMetadata.skirtSouthHeight, Math::EPSILON7));

	tinygltf::Value skirtEastHeight = gltfSkirt.Get("skirtEastHeight");
	REQUIRE(Math::equalsEpsilon(skirtEastHeight.GetNumberAsDouble(), skirtMeshMetadata.skirtEastHeight, Math::EPSILON7));

	tinygltf::Value skirtNorthHeight = gltfSkirt.Get("skirtNorthHeight");
	REQUIRE(Math::equalsEpsilon(skirtNorthHeight.GetNumberAsDouble(), skirtMeshMetadata.skirtNorthHeight, Math::EPSILON7));
}

TEST_CASE("Test converting gltf extras to skirt mesh metadata") {
	// mock gltf extras for skirt mesh metadata
	tinygltf::Value::Object gltfSkirtMeshMetadata;
	gltfSkirtMeshMetadata.insert({ "noSkirtRange", tinygltf::Value(tinygltf::Value::Array({
		tinygltf::Value(0), tinygltf::Value(12)})) });
	gltfSkirtMeshMetadata.insert({ "meshCenter", tinygltf::Value(tinygltf::Value::Array({
		tinygltf::Value(1.0), tinygltf::Value(2.0), tinygltf::Value(3.0)})) });
	gltfSkirtMeshMetadata.insert({ "skirtWestHeight", tinygltf::Value(12.4) });
	gltfSkirtMeshMetadata.insert({ "skirtSouthHeight", tinygltf::Value(10.0) });
	gltfSkirtMeshMetadata.insert({ "skirtEastHeight", tinygltf::Value(2.4) });
	gltfSkirtMeshMetadata.insert({ "skirtNorthHeight", tinygltf::Value(1.4) });

	SECTION("Gltf Extras has correct format") {
		tinygltf::Value extras = tinygltf::Value(
			tinygltf::Value::Object{ {"skirtMeshMetadata", tinygltf::Value(gltfSkirtMeshMetadata)} });

		SkirtMeshMetadata skirtMeshMetadata = *SkirtMeshMetadata::parseFromGltfExtras(extras);

		REQUIRE(skirtMeshMetadata.noSkirtIndicesBegin == 0);
		REQUIRE(skirtMeshMetadata.noSkirtIndicesCount == 12);
		REQUIRE(Math::equalsEpsilon(skirtMeshMetadata.meshCenter.x, 1.0, Math::EPSILON7));
		REQUIRE(Math::equalsEpsilon(skirtMeshMetadata.meshCenter.y, 2.0, Math::EPSILON7));
		REQUIRE(Math::equalsEpsilon(skirtMeshMetadata.meshCenter.z, 3.0, Math::EPSILON7));
		REQUIRE(Math::equalsEpsilon(skirtMeshMetadata.skirtWestHeight, 12.4, Math::EPSILON7));
		REQUIRE(Math::equalsEpsilon(skirtMeshMetadata.skirtSouthHeight, 10.0, Math::EPSILON7));
		REQUIRE(Math::equalsEpsilon(skirtMeshMetadata.skirtEastHeight, 2.4, Math::EPSILON7));
		REQUIRE(Math::equalsEpsilon(skirtMeshMetadata.skirtNorthHeight, 1.4, Math::EPSILON7));
	}

	SECTION("Gltf Extras has incorrect noSkirtRange field") {
		SECTION("missing noSkirtRange field") {
			gltfSkirtMeshMetadata.erase("noSkirtRange");

			tinygltf::Value extras = tinygltf::Value(
				tinygltf::Value::Object{ {"skirtMeshMetadata", tinygltf::Value(gltfSkirtMeshMetadata)} });

			REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
		}

		SECTION("noSkirtRange field has wrong type") {
			gltfSkirtMeshMetadata["noSkirtRange"] = tinygltf::Value(12);

			tinygltf::Value extras = tinygltf::Value(
				tinygltf::Value::Object{ {"skirtMeshMetadata", tinygltf::Value(gltfSkirtMeshMetadata)} });

			REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
		}

		SECTION("noSkirtRange field has only one element array") {
			gltfSkirtMeshMetadata["noSkirtRange"] = tinygltf::Value(tinygltf::Value::Array({tinygltf::Value(0)}));
			tinygltf::Value extras = tinygltf::Value(
				tinygltf::Value::Object{ {"skirtMeshMetadata", tinygltf::Value(gltfSkirtMeshMetadata)} });

			REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
		}

		SECTION("noSkirtRange field has two elements array but not integer") {
			gltfSkirtMeshMetadata["noSkirtRange"] = tinygltf::Value(
				tinygltf::Value::Array({tinygltf::Value("first"), tinygltf::Value("second")}));
			tinygltf::Value extras = tinygltf::Value(
				tinygltf::Value::Object{ {"skirtMeshMetadata", tinygltf::Value(gltfSkirtMeshMetadata)} });

			REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
		}
	}

	SECTION("Gltf Extras has incorrect meshCenter field") {
		SECTION("missing meshCenter field") {
			gltfSkirtMeshMetadata.erase("meshCenter");

			tinygltf::Value extras = tinygltf::Value(
				tinygltf::Value::Object{ {"skirtMeshMetadata", tinygltf::Value(gltfSkirtMeshMetadata)} });

			REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
		}

		SECTION("meshCenter field has wrong type") {
			gltfSkirtMeshMetadata["meshCenter"] = tinygltf::Value(12);

			tinygltf::Value extras = tinygltf::Value(
				tinygltf::Value::Object{ {"skirtMeshMetadata", tinygltf::Value(gltfSkirtMeshMetadata)} });

			REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
		}

		SECTION("meshCenter field is 2 elements array") {
			gltfSkirtMeshMetadata["meshCenter"] = tinygltf::Value(
				tinygltf::Value::Array({tinygltf::Value(1.0), tinygltf::Value(2.0)}));

			tinygltf::Value extras = tinygltf::Value(
				tinygltf::Value::Object{ {"skirtMeshMetadata", tinygltf::Value(gltfSkirtMeshMetadata)} });

			REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
		}

		SECTION("meshCenter field is 3 elements array but wrong type") {
			gltfSkirtMeshMetadata["meshCenter"] = tinygltf::Value(
				tinygltf::Value::Array({tinygltf::Value(1.0), tinygltf::Value(2.0), tinygltf::Value("third")}));

			tinygltf::Value extras = tinygltf::Value(
				tinygltf::Value::Object{ {"skirtMeshMetadata", tinygltf::Value(gltfSkirtMeshMetadata)} });

			REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
		}
	}

	SECTION("Gltf Extras has incorrect skirtWestHeight field") {
		SECTION("missing skirtWestHeight field") {
			gltfSkirtMeshMetadata.erase("skirtWestHeight");

			tinygltf::Value extras = tinygltf::Value(
				tinygltf::Value::Object{ {"skirtMeshMetadata", tinygltf::Value(gltfSkirtMeshMetadata)} });

			REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
		}

		SECTION("skirtWestHeight field has wrong type") {
			gltfSkirtMeshMetadata["skirtWestHeight"] = tinygltf::Value("string");

			tinygltf::Value extras = tinygltf::Value(
				tinygltf::Value::Object{ {"skirtMeshMetadata", tinygltf::Value(gltfSkirtMeshMetadata)} });

			REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
		}
	}

	SECTION("Gltf Extras has incorrect skirtSouthHeight field") {
		SECTION("missing skirtSouthHeight field") {
			gltfSkirtMeshMetadata.erase("skirtSouthHeight");

			tinygltf::Value extras = tinygltf::Value(
				tinygltf::Value::Object{ {"skirtMeshMetadata", tinygltf::Value(gltfSkirtMeshMetadata)} });

			REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
		}

		SECTION("skirtSouthHeight field has wrong type") {
			gltfSkirtMeshMetadata["skirtSouthHeight"] = tinygltf::Value("string");

			tinygltf::Value extras = tinygltf::Value(
				tinygltf::Value::Object{ {"skirtMeshMetadata", tinygltf::Value(gltfSkirtMeshMetadata)} });

			REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
		}
	}

	SECTION("Gltf Extras has incorrect skirtEastHeight field") {
		SECTION("missing skirtEastHeight field") {
			gltfSkirtMeshMetadata.erase("skirtEastHeight");

			tinygltf::Value extras = tinygltf::Value(
				tinygltf::Value::Object{ {"skirtMeshMetadata", tinygltf::Value(gltfSkirtMeshMetadata)} });

			REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
		}

		SECTION("skirtEastHeight field has wrong type") {
			gltfSkirtMeshMetadata["skirtEastHeight"] = tinygltf::Value("string");

			tinygltf::Value extras = tinygltf::Value(
				tinygltf::Value::Object{ {"skirtMeshMetadata", tinygltf::Value(gltfSkirtMeshMetadata)} });

			REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
		}
	}

	SECTION("Gltf Extras has incorrect skirtNorthHeight field") {
		SECTION("missing skirtNorthHeight field") {
			gltfSkirtMeshMetadata.erase("skirtNorthHeight");

			tinygltf::Value extras = tinygltf::Value(
				tinygltf::Value::Object{ {"skirtMeshMetadata", tinygltf::Value(gltfSkirtMeshMetadata)} });

			REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
		}

		SECTION("skirtNorthHeight field has wrong type") {
			gltfSkirtMeshMetadata["skirtNorthHeight"] = tinygltf::Value("string");

			tinygltf::Value extras = tinygltf::Value(
				tinygltf::Value::Object{ {"skirtMeshMetadata", tinygltf::Value(gltfSkirtMeshMetadata)} });

			REQUIRE(SkirtMeshMetadata::parseFromGltfExtras(extras) == std::nullopt);
		}
	}
}
