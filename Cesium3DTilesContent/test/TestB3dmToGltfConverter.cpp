#include "ConvertTileToGltf.h"

#include <CesiumGltf/ExtensionCesiumRTC.h>

#include <catch2/catch.hpp>

using namespace Cesium3DTilesContent;
using namespace CesiumGltf;

TEST_CASE("B3dmToGltfConverter") {
  SECTION("includes CESIUM_RTC extension in extensionsUsed") {
    std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
    testFilePath = testFilePath / "BatchTables" / "batchedWithJson.b3dm";

    GltfConverterResult result = ConvertTileToGltf::fromB3dm(testFilePath);

    REQUIRE(result.model);

    Model& gltf = *result.model;
    REQUIRE(gltf.getExtension<ExtensionCesiumRTC>() != nullptr);
    CHECK(gltf.isExtensionUsed(ExtensionCesiumRTC::ExtensionName));
    CHECK(gltf.isExtensionRequired(ExtensionCesiumRTC::ExtensionName));
  }
}
