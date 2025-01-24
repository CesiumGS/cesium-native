#include "ConvertTileToGltf.h"

#include <Cesium3DTilesContent/GltfConverterResult.h>
#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/BufferView.h>
#include <CesiumGltf/ExtensionCesiumRTC.h>
#include <CesiumGltf/Mesh.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>

#include <doctest/doctest.h>

#include <cstddef>
#include <filesystem>

using namespace Cesium3DTilesContent;
using namespace CesiumGltf;

TEST_CASE("B3dmToGltfConverter") {
  SUBCASE("includes CESIUM_RTC extension in extensionsUsed") {
    std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
    testFilePath = testFilePath / "BatchTables" / "batchedWithJson.b3dm";

    GltfConverterResult result = ConvertTileToGltf::fromB3dm(testFilePath);

    REQUIRE(result.model);

    Model& gltf = *result.model;
    REQUIRE(gltf.getExtension<ExtensionCesiumRTC>() != nullptr);
    CHECK(gltf.isExtensionUsed(ExtensionCesiumRTC::ExtensionName));
    CHECK(gltf.isExtensionRequired(ExtensionCesiumRTC::ExtensionName));
  }

  SUBCASE("Index bufferViews created from Draco are valid") {
    std::filesystem::path testFilePath = Cesium3DTilesSelection_TEST_DATA_DIR;
    testFilePath =
        testFilePath / "BatchTables" / "batchedWithBatchTable-draco.b3dm";

    GltfConverterResult result = ConvertTileToGltf::fromB3dm(testFilePath);
    REQUIRE(result.model);

    Model& gltf = *result.model;
    CHECK(!gltf.meshes.empty());

    for (const Mesh& mesh : gltf.meshes) {
      CHECK(!mesh.primitives.empty());
      for (const MeshPrimitive& primitive : mesh.primitives) {
        REQUIRE(primitive.indices >= 0);
        REQUIRE(size_t(primitive.indices) < gltf.accessors.size());
        const Accessor& indicesAccessor =
            gltf.accessors[size_t(primitive.indices)];

        REQUIRE(indicesAccessor.bufferView >= 0);
        REQUIRE(sizeof(indicesAccessor.bufferView) < gltf.bufferViews.size());
        const BufferView& indicesBufferView =
            gltf.bufferViews[size_t(indicesAccessor.bufferView)];

        CHECK(!indicesBufferView.byteStride.has_value());
        CHECK(
            indicesBufferView.target ==
            BufferView::Target::ELEMENT_ARRAY_BUFFER);

        auto positionIt = primitive.attributes.find("POSITION");
        REQUIRE(positionIt != primitive.attributes.end());
        REQUIRE(positionIt->second >= 0);
        REQUIRE(size_t(positionIt->second) < gltf.accessors.size());
        const Accessor& positionAccessor =
            gltf.accessors[size_t(positionIt->second)];

        REQUIRE(positionAccessor.bufferView >= 0);
        REQUIRE(sizeof(positionAccessor.bufferView) < gltf.bufferViews.size());
        const BufferView& positionBufferView =
            gltf.bufferViews[size_t(positionAccessor.bufferView)];

        CHECK(positionBufferView.target == BufferView::Target::ARRAY_BUFFER);
      }
    }
  }
}
