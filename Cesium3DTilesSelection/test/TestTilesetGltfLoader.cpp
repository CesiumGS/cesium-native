#include "ImplicitQuadtreeLoader.h"
#include "SimplePrepareRendererResource.h"
#include "TilesetLoaderTestUtility.h"

#include <Cesium3DTiles/TileRefine.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/TileLoadResult.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetContentLoaderResult.h>
#include <Cesium3DTilesSelection/TilesetMetadata.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumGeometry/Axis.h>
#include <CesiumGeometry/BoundingSphere.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/PropertyTablePropertyView.h>
#include <CesiumGltf/PropertyTableView.h>
#include <CesiumGltfConverters/registerAllConverterTypes.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumNativeTests/SimpleAssetResponse.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/readFile.h>
#include <CesiumTilesetJson/ExtensionContent3dTilesContentVoxels.h>
#include <CesiumTilesetJson/Schema.h>
#include <CesiumUtility/CreditSystem.h>

#include <doctest/doctest.h>
#include <glm/ext/matrix_double3x3.hpp>
#include <glm/ext/matrix_double4x4.hpp>
#include <spdlog/sinks/ringbuffer_sink.h>
#include <spdlog/spdlog.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace doctest;
using namespace CesiumAsync;
using namespace Cesium3DTiles;
using namespace Cesium3DTilesSelection;
using namespace CesiumGltf;
using namespace CesiumNativeTests;
using namespace CesiumUtility;

namespace {
auto testDataPath =
    std::filesystem::path(Cesium3DTilesSelection_TEST_DATA_DIR) / "2.0";
} // namespace

TEST_CASE("Test creating tileset gltf loader") {
  CesiumGltfConverters::registerAllConverterTypes();

  SUBCASE("Create valid tileset json with REPLACE refinement") {
    auto loaderResult = createTilesetGltfLoader(
        testDataPath / "ReplacementTileset" / "sample.tileset.gltf");

    CHECK_FALSE(loaderResult.errors.hasErrors());

    // check root tile
    Tile* pTilesetGltf = loaderResult.pRootTile.get();
    REQUIRE(pTilesetGltf);
    REQUIRE_EQ(pTilesetGltf->getChildren().size(), 1);
    CHECK_FALSE(pTilesetGltf->getParent());

    Tile* pRootTile = &pTilesetGltf->getChildren()[0];
    CHECK_EQ(pRootTile->getParent(), pTilesetGltf);
    REQUIRE_EQ(pRootTile->getChildren().size(), 4);
    CHECK_EQ(pRootTile->getGeometricError(), 70.0);
    CHECK_EQ(pRootTile->getRefine(), TileRefine::Replace);
    CHECK_EQ(std::get<std::string>(pRootTile->getTileID()), "parent.glb");

    const auto& boundingVolume = pRootTile->getBoundingVolume();
    const auto* pRegion =
        std::get_if<CesiumGeospatial::BoundingRegion>(&boundingVolume);
    REQUIRE(pRegion);
    CHECK_EQ(pRegion->getMinimumHeight(), Approx(0.0));
    CHECK_EQ(pRegion->getMaximumHeight(), Approx(88.0));
    CHECK_EQ(pRegion->getRectangle().getWest(), Approx(-1.3197209591796106));
    CHECK_EQ(pRegion->getRectangle().getEast(), Approx(-1.3196390408203893));
    CHECK_EQ(pRegion->getRectangle().getSouth(), Approx(0.6988424218));
    CHECK_EQ(pRegion->getRectangle().getNorth(), Approx(0.6989055782));

    // Check first child tile and its child
    auto children = pRootTile->getChildren();
    CHECK_EQ(children[0].getParent(), pRootTile);
    REQUIRE_EQ(children[0].getChildren().size(), 1);
    CHECK_EQ(children[0].getGeometricError(), 5.0);
    CHECK_EQ(children[0].getRefine(), TileRefine::Replace);
    CHECK_EQ(std::get<std::string>(children[0].getTileID()), "ll.glb");
    CHECK(std::holds_alternative<CesiumGeospatial::BoundingRegion>(
        children[0].getBoundingVolume()));

    Tile* pGrandChild = &children[0].getChildren()[0];
    CHECK_EQ(pGrandChild->getChildren().size(), 0);
    CHECK_EQ(pGrandChild->getGeometricError(), 0.0);
    CHECK_EQ(pGrandChild->getRefine(), TileRefine::Replace);
    CHECK_EQ(std::get<std::string>(pGrandChild->getTileID()), "ll_ll.glb");
    CHECK(std::holds_alternative<CesiumGeospatial::BoundingRegion>(
        pGrandChild->getBoundingVolume()));

    // Check the rest of the root's children
    CHECK_EQ(children[1].getParent(), pRootTile);
    CHECK_EQ(children[1].getChildren().size(), 0);
    CHECK_EQ(children[1].getGeometricError(), 0.0);
    CHECK_EQ(children[1].getRefine(), TileRefine::Replace);
    CHECK_EQ(std::get<std::string>(children[1].getTileID()), "lr.glb");
    CHECK(std::holds_alternative<CesiumGeospatial::BoundingRegion>(
        children[1].getBoundingVolume()));

    CHECK_EQ(children[2].getParent(), pRootTile);
    CHECK_EQ(children[2].getChildren().size(), 0);
    CHECK_EQ(children[2].getGeometricError(), 0.0);
    CHECK_EQ(children[2].getRefine(), TileRefine::Replace);
    CHECK_EQ(std::get<std::string>(children[2].getTileID()), "ul.glb");
    CHECK(std::holds_alternative<CesiumGeospatial::BoundingRegion>(
        children[2].getBoundingVolume()));

    CHECK_EQ(children[3].getParent(), pRootTile);
    CHECK_EQ(children[3].getChildren().size(), 0);
    CHECK_EQ(children[3].getGeometricError(), 0.0);
    CHECK_EQ(children[3].getRefine(), TileRefine::Replace);
    CHECK_EQ(std::get<std::string>(children[3].getTileID()), "ur.glb");
    CHECK(std::holds_alternative<CesiumGeospatial::BoundingRegion>(
        children[3].getBoundingVolume()));

    // check loader up axis
    CHECK_EQ(loaderResult.pLoader->getUpAxis(), CesiumGeometry::Axis::Y);
  }

  SUBCASE("Create valid tileset json with ADD refinement") {
    auto loaderResult = createTilesetGltfLoader(
        testDataPath / "AdditiveTileset" / "sample.tileset.gltf");

    CHECK_FALSE(loaderResult.errors.hasErrors());

    // check root tile
    auto pTilesetGltf = loaderResult.pRootTile.get();
    REQUIRE(pTilesetGltf);
    CHECK_EQ(pTilesetGltf->getParent(), nullptr);
    REQUIRE_EQ(pTilesetGltf->getChildren().size(), 1);
    auto pRootTile = &pTilesetGltf->getChildren()[0];
    CHECK_EQ(pRootTile->getParent(), pTilesetGltf);
    CHECK_EQ(pRootTile->getChildren().size(), 4);
    CHECK_EQ(pRootTile->getGeometricError(), 70.0);
    CHECK_EQ(pRootTile->getRefine(), TileRefine::Add);
    CHECK_EQ(std::get<std::string>(pRootTile->getTileID()), "parent.gltf");

    const auto& boundingVolume = pRootTile->getBoundingVolume();
    const auto* pRegion =
        std::get_if<CesiumGeospatial::BoundingRegion>(&boundingVolume);
    REQUIRE(pRegion);
    CHECK_EQ(pRegion->getMinimumHeight(), Approx(0.0));
    CHECK_EQ(pRegion->getMaximumHeight(), Approx(88.0));
    CHECK_EQ(pRegion->getRectangle().getWest(), Approx(-1.3197209591796106));
    CHECK_EQ(pRegion->getRectangle().getEast(), Approx(-1.3196390408203893));
    CHECK_EQ(pRegion->getRectangle().getSouth(), Approx(0.6988424218));
    CHECK_EQ(pRegion->getRectangle().getNorth(), Approx(0.6989055782));

    // check children
    std::array<std::string, 4> expectedUrl{
        {"ll.gltf", "lr.gltf", "ul.gltf", "ur.gltf"}};
    auto expectedUrlIt = expectedUrl.begin();
    for (const Tile& child : pRootTile->getChildren()) {
      CHECK_EQ(child.getParent(), pRootTile);
      CHECK_EQ(child.getChildren().size(), 0);
      CHECK_EQ(child.getGeometricError(), 0.0);
      CHECK_EQ(child.getRefine(), TileRefine::Add);
      CHECK_EQ(std::get<std::string>(child.getTileID()), *expectedUrlIt);
      CHECK(std::holds_alternative<CesiumGeospatial::BoundingRegion>(
          child.getBoundingVolume()));
      ++expectedUrlIt;
    }

    // check loader up axis
    CHECK_EQ(loaderResult.pLoader->getUpAxis(), CesiumGeometry::Axis::Y);
  }

  SUBCASE("Load tile that has external content with implicit tiling") {
    auto loaderResult = createTilesetGltfLoader(
        testDataPath / "ImplicitTileset" / "sample.tileset.gltf");

    REQUIRE(loaderResult.pRootTile);
    CHECK(loaderResult.pRootTile->isExternalContent());
    REQUIRE_EQ(loaderResult.pRootTile->getChildren().size(), 1);

    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];
    REQUIRE_EQ(pRootTile->getChildren().size(), 1);

    auto& implicitTile = pRootTile->getChildren().front();
    const auto& tileID =
        std::get<CesiumGeometry::QuadtreeTileID>(implicitTile.getTileID());
    CHECK_EQ(tileID, CesiumGeometry::QuadtreeTileID(0, 0, 0));

    // mock subtree content response
    auto subtreePath =
        testDataPath / "ImplicitTileset" / "subtrees" / "0.0.0.subtree.gltf";

    auto pMockSubtreeResponse = std::make_unique<SimpleAssetResponse>(
        static_cast<uint16_t>(200),
        "doesn't matter",
        CesiumAsync::HttpHeaders{},
        readFile(subtreePath));
    auto pMockSubtreeRequest = std::make_shared<SimpleAssetRequest>(
        "GET",
        "doesn't matter",
        CesiumAsync::HttpHeaders{},
        std::move(pMockSubtreeResponse));

    // mock tile content response
    auto tilePath =
        testDataPath / "ImplicitTileset" / "content" / "0" / "0" / "0.glb";

    auto pMockTileResponse = std::make_unique<SimpleAssetResponse>(
        static_cast<uint16_t>(200),
        "doesn't matter",
        CesiumAsync::HttpHeaders{},
        readFile(tilePath));
    auto pMockTileRequest = std::make_shared<SimpleAssetRequest>(
        "GET",
        "doesn't matter",
        CesiumAsync::HttpHeaders{},
        std::move(pMockTileResponse));

    // create mock asset accessor
    std::map<std::string, std::shared_ptr<SimpleAssetRequest>>
        mockCompletedRequests{
            {"subtrees/0.0.0.subtree.gltf", std::move(pMockSubtreeRequest)},
            {"content/0/0/0.glb", std::move(pMockTileRequest)}};

    std::shared_ptr<SimpleAssetAccessor> pMockAssetAccessor =
        std::make_shared<SimpleAssetAccessor>(std::move(mockCompletedRequests));

    AsyncSystem asyncSystem{std::make_shared<SimpleTaskProcessor>()};

    {
      // loader will tell to retry later since it needs subtree
      TileLoadInput loadInput{
          implicitTile,
          {},
          asyncSystem,
          pMockAssetAccessor,
          spdlog::default_logger(),
          {}};
      auto implicitContentResultFuture =
          loaderResult.pLoader->loadTileContent(loadInput);

      asyncSystem.dispatchMainThreadTasks();

      auto implicitContentResult = implicitContentResultFuture.wait();
      CHECK_EQ(implicitContentResult.state, TileLoadResultState::RetryLater);
    }

    {
      // loader will be able to load the tile the second time around
      TileLoadInput loadInput{
          implicitTile,
          {},
          asyncSystem,
          pMockAssetAccessor,
          spdlog::default_logger(),
          {}};
      auto implicitContentResultFuture =
          loaderResult.pLoader->loadTileContent(loadInput);

      asyncSystem.dispatchMainThreadTasks();

      auto implicitContentResult = implicitContentResultFuture.wait();
      CHECK(std::holds_alternative<CesiumGltf::Model>(
          implicitContentResult.contentKind));
      CHECK_FALSE(implicitContentResult.updatedBoundingVolume);
      CHECK_FALSE(implicitContentResult.updatedContentBoundingVolume);
      CHECK_EQ(implicitContentResult.state, TileLoadResultState::Success);
      CHECK_FALSE(implicitContentResult.tileInitializer);
    }
  }
}
