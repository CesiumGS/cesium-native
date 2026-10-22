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
#include <CesiumGeometry/Transforms.h>
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

TEST_CASE("Test TilesetGltfLoader") {
  CesiumGltfConverters::registerAllConverterTypes();

  SUBCASE("Loads tileset with REPLACE refinement") {
    auto loaderResult = createTilesetGltfLoader(
        testDataPath / "ReplacementTileset" / "root.tileset.gltf");

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

    CHECK_EQ(loaderResult.pLoader->getUpAxis(), CesiumGeometry::Axis::Z);
  }

  SUBCASE("Loads tileset with ADD refinement") {
    auto loaderResult = createTilesetGltfLoader(
        testDataPath / "AdditiveTileset" / "root.tileset.gltf");

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

    CHECK_EQ(loaderResult.pLoader->getUpAxis(), CesiumGeometry::Axis::Z);
  }

  SUBCASE("Loads tileset with sphere bounding volume") {
    auto loaderResult = createTilesetGltfLoader(
        testDataPath / "MinimalTilesets" / "SphereBoundingVolume.tileset.gltf");

    REQUIRE(loaderResult.pRootTile);
    REQUIRE_EQ(loaderResult.pRootTile->getChildren().size(), 1);

    const Tile& rootTile = loaderResult.pRootTile->getChildren()[0];
    const auto* pSphere = std::get_if<CesiumGeometry::BoundingSphere>(
        &rootTile.getBoundingVolume());
    REQUIRE(pSphere);

    // Since the tileset glTF is defined in local space, a y-up to z-up
    // transform will have been applied to the bounding volume.
    CHECK_EQ(pSphere->getCenter(), glm::dvec3(0.0, 0.0, 10.0));
    CHECK_EQ(pSphere->getRadius(), 141.4214);

    CHECK_EQ(loaderResult.pLoader->getUpAxis(), CesiumGeometry::Axis::Y);
  }

  SUBCASE("Loads tileset with box bounding volume") {
    auto loaderResult = createTilesetGltfLoader(
        testDataPath / "MinimalTilesets" / "BoxBoundingVolume.tileset.gltf");

    REQUIRE(loaderResult.pRootTile);
    REQUIRE_EQ(loaderResult.pRootTile->getChildren().size(), 1);

    const Tile& rootTile = loaderResult.pRootTile->getChildren()[0];
    const auto* pBox = std::get_if<CesiumGeometry::OrientedBoundingBox>(
        &rootTile.getBoundingVolume());
    REQUIRE(pBox);

    // Since the tileset glTF is defined in local space, a y-up to z-up
    // transform will have been applied to the bounding volume.
    const glm::dmat3& halfAxis = pBox->getHalfAxes();
    CHECK_EQ(halfAxis[0], glm::dvec3(100.0, 0.0, 0.0));
    CHECK_EQ(halfAxis[1], glm::dvec3(0.0, 0.0, 10.0));
    CHECK_EQ(halfAxis[2], glm::dvec3(0.0, -100.0, 0.0));
    CHECK_EQ(pBox->getCenter(), glm::dvec3(0.0, 0.0, 10.0));

    CHECK_EQ(loaderResult.pLoader->getUpAxis(), CesiumGeometry::Axis::Y);
  }

  SUBCASE("Ignores tiles with missing bounding volumes") {
    auto loaderResult = createTilesetGltfLoader(
        testDataPath / "MinimalTilesets" /
        "MissingBoundingVolumes.tileset.gltf");

    CHECK_FALSE(loaderResult.errors.hasErrors());
    REQUIRE(loaderResult.pRootTile);
    REQUIRE_EQ(loaderResult.pRootTile->getChildren().size(), 1);

    // None of the children defined a bounding volume, so they should not be
    // loaded.
    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];
    CHECK(pRootTile->getChildren().empty());

    CHECK_EQ(loaderResult.pLoader->getUpAxis(), CesiumGeometry::Axis::Z);
  }

  SUBCASE("Handles tiles with improperly capitalized Refinement field") {
    auto loaderResult = createTilesetGltfLoader(
        testDataPath / "MinimalTilesets" / "UncapitalizedRefine.tileset.gltf");

    CHECK_FALSE(loaderResult.errors.hasErrors());
    REQUIRE(loaderResult.pRootTile);
    REQUIRE_EQ(loaderResult.pRootTile->getChildren().size(), 1);
    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];
    CHECK_EQ(pRootTile->getGeometricError(), Approx(70.0));
    CHECK_EQ(pRootTile->getRefine(), TileRefine::Add);
    CHECK_EQ(pRootTile->getChildren().size(), 4);
    for (const Tile& child : pRootTile->getChildren()) {
      CHECK_EQ(child.getGeometricError(), Approx(5.0));
      CHECK_EQ(child.getRefine(), TileRefine::Replace);
    }

    CHECK_EQ(loaderResult.pLoader->getUpAxis(), CesiumGeometry::Axis::Z);
  }

  SUBCASE("Scales geometric error with tile transform") {
    auto loaderResult = createTilesetGltfLoader(
        testDataPath / "MinimalTilesets" / "ScaleGeometricError.tileset.gltf");

    CHECK_FALSE(loaderResult.errors.hasErrors());
    REQUIRE(loaderResult.pRootTile);
    REQUIRE_EQ(loaderResult.pRootTile->getChildren().size(), 1);

    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];
    CHECK_EQ(pRootTile->getGeometricError(), Approx(210.0));
    CHECK_EQ(pRootTile->getChildren().size(), 4);
    for (const Tile& child : pRootTile->getChildren()) {
      CHECK_EQ(child.getGeometricError(), Approx(15.0));
    }

    CHECK_EQ(loaderResult.pLoader->getUpAxis(), CesiumGeometry::Axis::Z);
  }

  SUBCASE("Loads tile with render content") {
    auto loaderResult = createTilesetGltfLoader(
        testDataPath / "ReplacementTileset" / "root.tileset.gltf");
    REQUIRE(loaderResult.pRootTile);
    REQUIRE_EQ(loaderResult.pRootTile->getChildren().size(), 1);

    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];

    const auto* pTileId = std::get_if<std::string>(&pRootTile->getTileID());
    REQUIRE(pTileId);
    CHECK_EQ(*pTileId, "parent.glb");

    // check tile content
    auto tileLoadResult = mockLoadTileContent(
        testDataPath / "ReplacementTileset" / *pTileId,
        *loaderResult.pLoader,
        *pRootTile);
    CHECK(
        std::holds_alternative<CesiumGltf::Model>(tileLoadResult.contentKind));
    CHECK_UNARY_FALSE(tileLoadResult.updatedBoundingVolume);
    CHECK_UNARY_FALSE(tileLoadResult.updatedContentBoundingVolume);
    CHECK_EQ(tileLoadResult.state, TileLoadResultState::Success);
    CHECK_FALSE(tileLoadResult.tileInitializer);
  }

  SUBCASE("Load tile that has external content") {
    auto loaderResult = createTilesetGltfLoader(
        testDataPath / "AdditiveExternalTilesets" / "root.tileset.gltf");

    REQUIRE(loaderResult.pRootTile);
    REQUIRE_EQ(loaderResult.pRootTile->getChildren().size(), 1);

    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];

    const auto* pTileId = std::get_if<std::string>(&pRootTile->getTileID());
    REQUIRE(pTileId);
    CHECK_EQ(*pTileId, "external.tileset.gltf");

    // check tile content
    auto tileLoadResult = mockLoadTileContent(
        testDataPath / "AdditiveExternalTilesets" / *pTileId,
        *loaderResult.pLoader,
        *pRootTile);
    CHECK_UNARY_FALSE(tileLoadResult.updatedBoundingVolume);
    CHECK_UNARY_FALSE(tileLoadResult.updatedContentBoundingVolume);
    CHECK(std::holds_alternative<TileExternalContent>(
        tileLoadResult.contentKind));
    CHECK_EQ(tileLoadResult.state, TileLoadResultState::Success);
    CHECK(tileLoadResult.tileInitializer);

    // check that tile is really an external tile
    pRootTile->getContent().setContentKind(
        std::make_unique<TileExternalContent>(
            std::get<TileExternalContent>(tileLoadResult.contentKind)));
    tileLoadResult.tileInitializer(*pRootTile);
    const auto& children = pRootTile->getChildren();
    REQUIRE_EQ(children.size(), 1);

    const Tile& parentTile = children[0];
    const auto* pParentTileId =
        std::get_if<std::string>(&parentTile.getTileID());
    REQUIRE(pParentTileId);
    CHECK_EQ(*pParentTileId, "parent.gltf");
    CHECK_EQ(parentTile.getGeometricError(), Approx(70.0));

    std::vector<std::string> expectedChildUrls{
        "tileset3/ll.tileset.gltf",
        "lr.gltf",
        "ul.gltf",
        "ur.gltf"};
    const auto& parentChildren = parentTile.getChildren();
    for (std::size_t i = 0; i < parentChildren.size(); ++i) {
      const Tile& child = parentChildren[i];
      const auto* pChildTileId = std::get_if<std::string>(&child.getTileID());
      REQUIRE(pChildTileId);
      CHECK_EQ(*pChildTileId, expectedChildUrls[i]);
      CHECK_EQ(child.getGeometricError(), Approx(0.0));
      CHECK_EQ(child.getRefine(), TileRefine::Add);
      CHECK(std::holds_alternative<CesiumGeospatial::BoundingRegion>(
          child.getBoundingVolume()));
    }
  }

  SUBCASE("Loads tile that has external content through implicit tiling") {
    auto loaderResult = createTilesetGltfLoader(
        testDataPath / "ImplicitTileset" / "root.tileset.gltf");

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

  SUBCASE("Loads tileset with tile missing content") {
    auto pLog = std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(3);
    spdlog::default_logger()->sinks().emplace_back(pLog);

    auto loaderResult = createTilesetGltfLoader(
        testDataPath / "MinimalTilesets" / "ErrorMissingContent.tileset.gltf");
    REQUIRE(loaderResult.pRootTile);
    REQUIRE_EQ(loaderResult.pRootTile->getChildren().size(), 1);

    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];

    const auto* pTileID = std::get_if<std::string>(&pRootTile->getTileID());
    REQUIRE(pTileID);
    CHECK_EQ(*pTileID, "nonexistent.gltf");

    auto tileLoadResult = mockLoadTileContent(
        testDataPath / "MinimalTilesets" / *pTileID,
        *loaderResult.pLoader,
        *pRootTile);
    CHECK_EQ(tileLoadResult.state, TileLoadResultState::Failed);

    std::vector<std::string> logMessages = pLog->last_formatted();
    REQUIRE_EQ(logMessages.size(), 1);
    REQUIRE(
        logMessages.back()
            .substr(0, logMessages.back().find_last_not_of("\n\r") + 1)
            .ends_with(
                "Received status code 404 for tile content nonexistent.gltf"));
  }

  SUBCASE("Loads tileset with empty tile") {
    std::shared_ptr<SimpleAssetAccessor> pMockAssetAccessor =
        std::make_shared<SimpleAssetAccessor>(
            std::map<std::string, std::shared_ptr<SimpleAssetRequest>>());
    AsyncSystem asyncSystem{std::make_shared<SimpleTaskProcessor>()};

    auto loaderResult = createTilesetGltfLoader(
        testDataPath / "MinimalTilesets" / "EmptyTile.tileset.gltf");
    CHECK_FALSE(loaderResult.errors.hasErrors());
    REQUIRE(loaderResult.pRootTile);
    REQUIRE_EQ(loaderResult.pRootTile->getChildren().size(), 1);

    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];
    CHECK_EQ(pRootTile->getGeometricError(), Approx(70.0));
    CHECK_EQ(pRootTile->getChildren().size(), 1);

    Tile& child = pRootTile->getChildren().front();
    auto future = loaderResult.pLoader->loadTileContent(TileLoadInput{
        child,
        {},
        asyncSystem,
        pMockAssetAccessor,
        spdlog::default_logger(),
        {}});
    TileLoadResult result = future.wait();
    REQUIRE_EQ(result.state, TileLoadResultState::Success);
    TileEmptyContent* pEmptyContent =
        std::get_if<TileEmptyContent>(&result.contentKind);
    REQUIRE(pEmptyContent);
    child.getContent().setContentKind(*pEmptyContent);
    CHECK(child.isEmptyContent());

    CHECK_EQ(loaderResult.pLoader->getUpAxis(), CesiumGeometry::Axis::Z);
  }

  SUBCASE("Loads tileset with quadtree implicit tile") {
    auto loaderResult = createTilesetGltfLoader(
        testDataPath / "MinimalTilesets" / "ImplicitQuadtree.tileset.gltf");
    CHECK_FALSE(loaderResult.errors.hasErrors());
    REQUIRE(loaderResult.pRootTile);
    CHECK(loaderResult.pRootTile->isExternalContent());
    REQUIRE_EQ(loaderResult.pRootTile->getChildren().size(), 1);

    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];
    CHECK(pRootTile->isExternalContent());
    REQUIRE_EQ(pRootTile->getChildren().size(), 1);

    const Tile& child = pRootTile->getChildren().front();
    CHECK_NE(child.getLoader(), loaderResult.pLoader.get());
    CHECK_EQ(child.getGeometricError(), pRootTile->getGeometricError());
    CHECK_EQ(child.getRefine(), pRootTile->getRefine());
    CHECK_EQ(child.getTransform(), pRootTile->getTransform());
    CHECK_EQ(
        std::get<CesiumGeometry::QuadtreeTileID>(child.getTileID()),
        CesiumGeometry::QuadtreeTileID(0, 0, 0));

    CHECK_EQ(loaderResult.pLoader->getUpAxis(), CesiumGeometry::Axis::Y);
    CHECK_EQ(
        pRootTile->getTransform(),
        CesiumGeometry::Transforms::Y_UP_TO_Z_UP * glm::dmat4(glm::dmat3(2.0)));
  }

  SUBCASE("Loads tileset with octree implicit tile") {
    auto loaderResult = createTilesetGltfLoader(
        testDataPath / "MinimalTilesets" / "ImplicitOctree.tileset.gltf");
    CHECK_FALSE(loaderResult.errors.hasErrors());
    REQUIRE(loaderResult.pRootTile);
    CHECK(loaderResult.pRootTile->isExternalContent());
    REQUIRE_EQ(loaderResult.pRootTile->getChildren().size(), 1);

    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];
    CHECK(pRootTile->isExternalContent());
    REQUIRE_EQ(pRootTile->getChildren().size(), 1);

    const Tile& child = pRootTile->getChildren().front();
    CHECK_NE(child.getLoader(), loaderResult.pLoader.get());
    CHECK_EQ(child.getGeometricError(), pRootTile->getGeometricError());
    CHECK_EQ(child.getRefine(), pRootTile->getRefine());
    CHECK_EQ(child.getTransform(), pRootTile->getTransform());
    CHECK_EQ(
        std::get<CesiumGeometry::OctreeTileID>(child.getTileID()),
        CesiumGeometry::OctreeTileID(0, 0, 0, 0));

    CHECK_EQ(loaderResult.pLoader->getUpAxis(), CesiumGeometry::Axis::Y);
    CHECK_EQ(
        pRootTile->getTransform(),
        CesiumGeometry::Transforms::Y_UP_TO_Z_UP * glm::dmat4(glm::dmat3(2.0)));
  }
}
