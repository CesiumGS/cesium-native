#include "catch2/catch.hpp"
#include "SkirtMeshMetadata.h"

TEST_CASE("Test converting skirt mesh metadata to gltf extras") {
	Cesium3DTiles::SkirtMeshMetadata skirtMeshMetadata;
	REQUIRE(skirtMeshMetadata.noSkirtIndicesBegin == 0);
}
