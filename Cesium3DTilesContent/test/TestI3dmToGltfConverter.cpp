#include "ConvertTileToGltf.h"

#include <Cesium3DTilesContent/GltfConverterResult.h>
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/ExtensionExtInstanceFeatures.h>
#include <CesiumGltf/ExtensionExtMeshGpuInstancing.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>

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

  SUBCASE("loads an i3dm with metadata") {
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
    CHECK(result.model->isExtensionUsed(
        ExtensionExtInstanceFeatures::ExtensionName));
    for (Node& node : result.model->nodes) {
      if (node.getExtension<ExtensionExtMeshGpuInstancing>()) {
        auto* pInstanceExt = node.getExtension<ExtensionExtInstanceFeatures>();
        REQUIRE(pInstanceExt);
        REQUIRE(pInstanceExt->featureIds.size() == 1);
        CHECK(pInstanceExt->featureIds[0].featureCount == 25);
        CHECK(!pInstanceExt->featureIds[0].attribute.has_value());
        REQUIRE(pInstanceExt->featureIds[0].propertyTable.has_value());
        CHECK(*pInstanceExt->featureIds[0].propertyTable == 0);
        CHECK(!pInstanceExt->featureIds[0].nullFeatureId.has_value());
        CHECK(!pInstanceExt->featureIds[0].label.has_value());
      }
    }
    auto* pStructuralMetadataExt =
        result.model->getExtension<ExtensionModelExtStructuralMetadata>();
    REQUIRE(pStructuralMetadataExt);
    REQUIRE(pStructuralMetadataExt->propertyTables.size() == 1);
    PropertyTable& propertyTable = pStructuralMetadataExt->propertyTables[0];
    CHECK(propertyTable.classProperty == "default");
    auto heightIt = propertyTable.properties.find("Height");
    REQUIRE(heightIt != propertyTable.properties.end());
  }
}
