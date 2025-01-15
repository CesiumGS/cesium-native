#include "ConvertTileToGltf.h"

#include <Cesium3DTilesContent/GltfConverterResult.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/ExtensionExtMeshGpuInstancing.h>

#include <doctest/doctest.h>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

#include <filesystem>

using namespace Cesium3DTilesContent;
using namespace CesiumGltf;

TEST_CASE("I3dmToGltfConverter") {
  SUBCASE("loads a simple i3dm") {
    std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
    testFilePath = testFilePath / "i3dm" / "InstancedWithBatchTable" /
                   "instancedWithBatchTable.i3dm";

    GltfConverterResult result = ConvertTileToGltf::fromI3dm(testFilePath);

    REQUIRE(result.model);
    CHECK(result.model->isExtensionUsed(
        ExtensionExtMeshGpuInstancing::ExtensionName));
    CHECK(result.model->isExtensionRequired(
        ExtensionExtMeshGpuInstancing::ExtensionName));
    REQUIRE(result.model->nodes.size() == 1);

    ExtensionExtMeshGpuInstancing* pExtension =
        result.model->nodes[0].getExtension<ExtensionExtMeshGpuInstancing>();
    REQUIRE(pExtension);

    auto translationIt = pExtension->attributes.find("TRANSLATION");
    REQUIRE(translationIt != pExtension->attributes.end());

    AccessorView<glm::vec3> translations(*result.model, translationIt->second);
    REQUIRE(translations.status() == AccessorViewStatus::Valid);
    CHECK(translations.size() == 25);
  }

  SUBCASE("loads a simple i3dm with orientations") {
    std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
    testFilePath = testFilePath / "i3dm" / "InstancedOrientation" /
                   "instancedOrientation.i3dm";

    GltfConverterResult result = ConvertTileToGltf::fromI3dm(testFilePath);

    REQUIRE(result.model);
    CHECK(result.model->isExtensionUsed(
        ExtensionExtMeshGpuInstancing::ExtensionName));
    CHECK(result.model->isExtensionRequired(
        ExtensionExtMeshGpuInstancing::ExtensionName));
    REQUIRE(result.model->nodes.size() == 1);

    ExtensionExtMeshGpuInstancing* pExtension =
        result.model->nodes[0].getExtension<ExtensionExtMeshGpuInstancing>();
    REQUIRE(pExtension);

    auto translationIt = pExtension->attributes.find("TRANSLATION");
    REQUIRE(translationIt != pExtension->attributes.end());

    AccessorView<glm::vec3> translations(*result.model, translationIt->second);
    REQUIRE(translations.status() == AccessorViewStatus::Valid);
    CHECK(translations.size() == 25);

    auto rotationIt = pExtension->attributes.find("ROTATION");
    REQUIRE(rotationIt != pExtension->attributes.end());

    AccessorView<glm::vec4> rotations(*result.model, rotationIt->second);
    REQUIRE(rotations.status() == AccessorViewStatus::Valid);
    CHECK(rotations.size() == 25);
  }

  SUBCASE("reports an error if the glTF is v1, which is unsupported") {
    std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
    testFilePath =
        testFilePath / "i3dm" / "ObsoleteGltf" / "instancedWithBatchTable.i3dm";

    GltfConverterResult result = ConvertTileToGltf::fromI3dm(testFilePath);

    REQUIRE(!result.model);
    CHECK(result.errors.hasErrors());
  }
}
