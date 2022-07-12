#include "SimpleAssetAccessor.h"
#include "SimpleAssetRequest.h"
#include "SimpleAssetResponse.h"
#include "SimplePrepareRendererResource.h"
#include "SimpleTaskProcessor.h"
#include "TilesetJsonLoader.h"
#include "readFile.h"

#include <catch2/catch.hpp>

#include <cstddef>
#include <memory>
#include <string>

using namespace CesiumAsync;
using namespace Cesium3DTilesSelection;

namespace {
std::filesystem::path testDataPath = Cesium3DTilesSelection_TEST_DATA_DIR;

TilesetExternals
createMockTilesetExternals(std::vector<std::byte>&& tilesetContent) {
  auto pMockCompletedResponse = std::make_unique<SimpleAssetResponse>(
      static_cast<uint16_t>(200),
      "doesn't matter",
      CesiumAsync::HttpHeaders{},
      std::move(tilesetContent));
  auto pMockCompletedRequest = std::make_shared<SimpleAssetRequest>(
      "GET",
      "tileset.json",
      CesiumAsync::HttpHeaders{},
      std::move(pMockCompletedResponse));

  std::map<std::string, std::shared_ptr<SimpleAssetRequest>>
      mockCompletedRequests;
  mockCompletedRequests.insert(
      {"tileset.json", std::move(pMockCompletedRequest)});

  std::shared_ptr<SimpleAssetAccessor> pMockAssetAccessor =
      std::make_shared<SimpleAssetAccessor>(std::move(mockCompletedRequests));

  auto pMockPrepareRendererResource =
      std::make_shared<SimplePrepareRendererResource>();

  auto pMockCreditSystem = std::make_shared<CreditSystem>();

  AsyncSystem asyncSystem{std::make_shared<SimpleTaskProcessor>()};

  return TilesetExternals{
      std::move(pMockAssetAccessor),
      std::move(pMockPrepareRendererResource),
      std::move(asyncSystem),
      std::move(pMockCreditSystem)};
}

TileLoadResult loadTileContent(
    const std::filesystem::path& tilePath,
    TilesetContentLoader& loader,
    Tile& tile) {
  auto pMockCompletedResponse = std::make_unique<SimpleAssetResponse>(
      static_cast<uint16_t>(200),
      "doesn't matter",
      CesiumAsync::HttpHeaders{},
      readFile(tilePath));
  auto pMockCompletedRequest = std::make_shared<SimpleAssetRequest>(
      "GET",
      "tileset.json",
      CesiumAsync::HttpHeaders{},
      std::move(pMockCompletedResponse));

  std::map<std::string, std::shared_ptr<SimpleAssetRequest>>
      mockCompletedRequests;
  mockCompletedRequests.insert(
      {tilePath.filename().string(), std::move(pMockCompletedRequest)});

  std::shared_ptr<SimpleAssetAccessor> pMockAssetAccessor =
      std::make_shared<SimpleAssetAccessor>(std::move(mockCompletedRequests));

  AsyncSystem asyncSystem{std::make_shared<SimpleTaskProcessor>()};

  auto tileLoadResultFuture = loader.loadTileContent(
      tile,
      {},
      asyncSystem,
      pMockAssetAccessor,
      spdlog::default_logger(),
      {});

  asyncSystem.dispatchMainThreadTasks();

  return tileLoadResultFuture.wait();
}
} // namespace

TEST_CASE("Test creating tileset json loader") {

  SECTION("Create valid tileset json with REPLACE refinement") {
    auto tilesetData =
        readFile(testDataPath / "ReplaceTileset" / "tileset.json");

    auto loaderResult = TilesetJsonLoader::createLoader(
                            createMockTilesetExternals(std::move(tilesetData)),
                            "tileset.json",
                            {})
                            .wait();

    CHECK(!loaderResult.errors.hasErrors());
    CHECK(loaderResult.gltfUpAxis == CesiumGeometry::Axis::Y);

    // check root tile
    auto pRootTile = loaderResult.pRootTile.get();
    CHECK(pRootTile != nullptr);
    CHECK(pRootTile->getParent() == nullptr);
    CHECK(pRootTile->getChildren().size() == 4);
    CHECK(pRootTile->getGeometricError() == 70.0);
    CHECK(pRootTile->getRefine() == TileRefine::Replace);
    CHECK(std::get<std::string>(pRootTile->getTileID()) == "parent.b3dm");

    const auto& boundingVolume = pRootTile->getBoundingVolume();
    const auto* pRegion =
        std::get_if<CesiumGeospatial::BoundingRegion>(&boundingVolume);
    CHECK(pRegion != nullptr);
    CHECK(pRegion->getMinimumHeight() == Approx(0.0));
    CHECK(pRegion->getMaximumHeight() == Approx(88.0));
    CHECK(pRegion->getRectangle().getWest() == Approx(-1.3197209591796106));
    CHECK(pRegion->getRectangle().getEast() == Approx(-1.3196390408203893));
    CHECK(pRegion->getRectangle().getSouth() == Approx(0.6988424218));
    CHECK(pRegion->getRectangle().getNorth() == Approx(0.6989055782));

    // check root children
    auto children = pRootTile->getChildren();
    CHECK(children[0].getParent() == pRootTile);
    CHECK(children[0].getChildren().size() == 1);
    CHECK(children[0].getGeometricError() == 5.0);
    CHECK(children[0].getRefine() == TileRefine::Replace);
    CHECK(std::get<std::string>(children[0].getTileID()) == "ll.b3dm");
    CHECK(std::holds_alternative<CesiumGeospatial::BoundingRegion>(
        children[0].getBoundingVolume()));

    CHECK(children[1].getParent() == pRootTile);
    CHECK(children[1].getChildren().size() == 0);
    CHECK(children[1].getGeometricError() == 0.0);
    CHECK(children[1].getRefine() == TileRefine::Replace);
    CHECK(std::get<std::string>(children[1].getTileID()) == "lr.b3dm");
    CHECK(std::holds_alternative<CesiumGeospatial::BoundingRegion>(
        children[1].getBoundingVolume()));

    CHECK(children[2].getParent() == pRootTile);
    CHECK(children[2].getChildren().size() == 0);
    CHECK(children[2].getGeometricError() == 0.0);
    CHECK(children[2].getRefine() == TileRefine::Replace);
    CHECK(std::get<std::string>(children[2].getTileID()) == "ur.b3dm");
    CHECK(std::holds_alternative<CesiumGeospatial::BoundingRegion>(
        children[2].getBoundingVolume()));

    CHECK(children[3].getParent() == pRootTile);
    CHECK(children[3].getChildren().size() == 0);
    CHECK(children[3].getGeometricError() == 0.0);
    CHECK(children[3].getRefine() == TileRefine::Replace);
    CHECK(std::get<std::string>(children[3].getTileID()) == "ul.b3dm");
    CHECK(std::holds_alternative<CesiumGeospatial::BoundingRegion>(
        children[3].getBoundingVolume()));
  }

  SECTION("Create valid tileset json with ADD refinement") {
    auto tilesetData = readFile(testDataPath / "AddTileset" / "tileset2.json");

    auto loaderResult = TilesetJsonLoader::createLoader(
                            createMockTilesetExternals(std::move(tilesetData)),
                            "tileset.json",
                            {})
                            .wait();

    CHECK(!loaderResult.errors.hasErrors());
    CHECK(loaderResult.gltfUpAxis == CesiumGeometry::Axis::Y);

    // check root tile
    auto pRootTile = loaderResult.pRootTile.get();
    CHECK(pRootTile != nullptr);
    CHECK(pRootTile->getParent() == nullptr);
    CHECK(pRootTile->getChildren().size() == 4);
    CHECK(pRootTile->getGeometricError() == 70.0);
    CHECK(pRootTile->getRefine() == TileRefine::Add);
    CHECK(std::get<std::string>(pRootTile->getTileID()) == "parent.b3dm");

    const auto& boundingVolume = pRootTile->getBoundingVolume();
    const auto* pRegion =
        std::get_if<CesiumGeospatial::BoundingRegion>(&boundingVolume);
    CHECK(pRegion != nullptr);
    CHECK(pRegion->getMinimumHeight() == Approx(0.0));
    CHECK(pRegion->getMaximumHeight() == Approx(88.0));
    CHECK(pRegion->getRectangle().getWest() == Approx(-1.3197209591796106));
    CHECK(pRegion->getRectangle().getEast() == Approx(-1.3196390408203893));
    CHECK(pRegion->getRectangle().getSouth() == Approx(0.6988424218));
    CHECK(pRegion->getRectangle().getNorth() == Approx(0.6989055782));

    // check children
    std::array<std::string, 4> expectedUrl{
        {"tileset3/tileset3.json", "lr.b3dm", "ur.b3dm", "ul.b3dm"}};
    auto expectedUrlIt = expectedUrl.begin();
    for (const Tile& child : pRootTile->getChildren()) {
      CHECK(child.getParent() == pRootTile);
      CHECK(child.getChildren().size() == 0);
      CHECK(child.getGeometricError() == 0.0);
      CHECK(child.getRefine() == TileRefine::Add);
      CHECK(std::get<std::string>(child.getTileID()) == *expectedUrlIt);
      CHECK(std::holds_alternative<CesiumGeospatial::BoundingRegion>(
          child.getBoundingVolume()));
      ++expectedUrlIt;
    }
  }

  SECTION("Tileset has tile with no bounding volume field") {
    auto tilesetData = readFile(
        testDataPath / "MultipleKindsOfTilesets" /
        "NoBoundingVolumeTileset.json");

    auto loaderResult = TilesetJsonLoader::createLoader(
                            createMockTilesetExternals(std::move(tilesetData)),
                            "tileset.json",
                            {})
                            .wait();

    CHECK(!loaderResult.errors.hasErrors());
    CHECK(loaderResult.gltfUpAxis == CesiumGeometry::Axis::Y);
    CHECK(loaderResult.pRootTile);
    CHECK(loaderResult.pRootTile->getChildren().empty());
  }

  SECTION("Tileset has tile with no geometric error field") {
    auto tilesetData = readFile(
        testDataPath / "MultipleKindsOfTilesets" /
        "NoGeometricErrorTileset.json");

    auto loaderResult = TilesetJsonLoader::createLoader(
                            createMockTilesetExternals(std::move(tilesetData)),
                            "tileset.json",
                            {})
                            .wait();

    CHECK(!loaderResult.errors.hasErrors());
    CHECK(loaderResult.gltfUpAxis == CesiumGeometry::Axis::Y);
    CHECK(loaderResult.pRootTile);
    CHECK(loaderResult.pRootTile->getGeometricError() == Approx(70.0));
    CHECK(loaderResult.pRootTile->getChildren().size() == 4);
    for (const Tile& child : loaderResult.pRootTile->getChildren()) {
      CHECK(child.getGeometricError() == Approx(35.0));
    }
  }

  SECTION("Tileset has tile with no capitalized Refinement field") {
    auto tilesetData = readFile(
        testDataPath / "MultipleKindsOfTilesets" /
        "NoCapitalizedRefineTileset.json");

    auto loaderResult = TilesetJsonLoader::createLoader(
                            createMockTilesetExternals(std::move(tilesetData)),
                            "tileset.json",
                            {})
                            .wait();

    CHECK(!loaderResult.errors.hasErrors());
    CHECK(loaderResult.gltfUpAxis == CesiumGeometry::Axis::Y);
    CHECK(loaderResult.pRootTile);
    CHECK(loaderResult.pRootTile->getGeometricError() == Approx(70.0));
    CHECK(loaderResult.pRootTile->getRefine() == TileRefine::Add);
    CHECK(loaderResult.pRootTile->getChildren().size() == 4);
    for (const Tile& child : loaderResult.pRootTile->getChildren()) {
      CHECK(child.getGeometricError() == Approx(5.0));
      CHECK(child.getRefine() == TileRefine::Replace);
    }
  }

  SECTION("Scale geometric error along with tile transform") {
    auto tilesetData = readFile(
        testDataPath / "MultipleKindsOfTilesets" /
        "ScaleGeometricErrorTileset.json");

    auto loaderResult = TilesetJsonLoader::createLoader(
                            createMockTilesetExternals(std::move(tilesetData)),
                            "tileset.json",
                            {})
                            .wait();

    CHECK(!loaderResult.errors.hasErrors());
    CHECK(loaderResult.gltfUpAxis == CesiumGeometry::Axis::Y);
    CHECK(loaderResult.pRootTile);
    CHECK(loaderResult.pRootTile->getGeometricError() == Approx(210.0));
    CHECK(loaderResult.pRootTile->getChildren().size() == 4);
    for (const Tile& child : loaderResult.pRootTile->getChildren()) {
      CHECK(child.getGeometricError() == Approx(15.0));
    }
  }

  SECTION("Tileset with empty tile") {
    auto tilesetData = readFile(
        testDataPath / "MultipleKindsOfTilesets" / "EmptyTileTileset.json");

    auto loaderResult = TilesetJsonLoader::createLoader(
                            createMockTilesetExternals(std::move(tilesetData)),
                            "tileset.json",
                            {})
                            .wait();
    CHECK(!loaderResult.errors.hasErrors());
    CHECK(loaderResult.gltfUpAxis == CesiumGeometry::Axis::Y);
    CHECK(loaderResult.pRootTile);
    CHECK(loaderResult.pRootTile->getGeometricError() == Approx(70.0));
    CHECK(loaderResult.pRootTile->getChildren().size() == 1);

    const Tile& child = loaderResult.pRootTile->getChildren().front();
    CHECK(child.isEmptyContent());
  }

  SECTION("Tileset with quadtree implicit tile") {
    auto tilesetData = readFile(
        testDataPath / "MultipleKindsOfTilesets" /
        "QuadtreeImplicitTileset.json");

    auto loaderResult = TilesetJsonLoader::createLoader(
                            createMockTilesetExternals(std::move(tilesetData)),
                            "tileset.json",
                            {})
                            .wait();
    CHECK(!loaderResult.errors.hasErrors());
    CHECK(loaderResult.pRootTile);
    CHECK(loaderResult.pRootTile->isExternalContent());
    CHECK(loaderResult.pRootTile->getChildren().size() == 1);
    CHECK(
        loaderResult.pRootTile->getTransform() == glm::dmat4(glm::dmat3(2.0)));

    const Tile& child = loaderResult.pRootTile->getChildren().front();
    CHECK(child.getContent().getLoader() != loaderResult.pLoader.get());
    CHECK(
        child.getGeometricError() ==
        loaderResult.pRootTile->getGeometricError());
    CHECK(child.getRefine() == loaderResult.pRootTile->getRefine());
    CHECK(child.getTransform() == loaderResult.pRootTile->getTransform());
    CHECK(
        std::get<CesiumGeometry::QuadtreeTileID>(child.getTileID()) ==
        CesiumGeometry::QuadtreeTileID(0, 0, 0));
  }

  SECTION("Tileset with octree implicit tile") {
    auto tilesetData = readFile(
        testDataPath / "MultipleKindsOfTilesets" /
        "OctreeImplicitTileset.json");

    auto loaderResult = TilesetJsonLoader::createLoader(
                            createMockTilesetExternals(std::move(tilesetData)),
                            "tileset.json",
                            {})
                            .wait();
    CHECK(!loaderResult.errors.hasErrors());
    CHECK(loaderResult.pRootTile);
    CHECK(loaderResult.pRootTile->isExternalContent());
    CHECK(loaderResult.pRootTile->getChildren().size() == 1);
    CHECK(
        loaderResult.pRootTile->getTransform() == glm::dmat4(glm::dmat3(2.0)));

    const Tile& child = loaderResult.pRootTile->getChildren().front();
    CHECK(child.getContent().getLoader() != loaderResult.pLoader.get());
    CHECK(
        child.getGeometricError() ==
        loaderResult.pRootTile->getGeometricError());
    CHECK(child.getRefine() == loaderResult.pRootTile->getRefine());
    CHECK(child.getTransform() == loaderResult.pRootTile->getTransform());
    CHECK(
        std::get<CesiumGeometry::OctreeTileID>(child.getTileID()) ==
        CesiumGeometry::OctreeTileID(0, 0, 0, 0));
  }
}

TEST_CASE("Test loading individual tile of tileset json") {
  SECTION("Load tile that has render content") {
    auto tilesetData =
        readFile(testDataPath / "ReplaceTileset" / "tileset.json");

    auto loaderResult = TilesetJsonLoader::createLoader(
                            createMockTilesetExternals(std::move(tilesetData)),
                            "tileset.json",
                            {})
                            .wait();
    CHECK(loaderResult.pRootTile);

    const auto& tileID =
        std::get<std::string>(loaderResult.pRootTile->getTileID());
    CHECK(tileID == "parent.b3dm");

    // check tile content
    auto tileLoadResult = loadTileContent(
        testDataPath / "ReplaceTileset" / tileID,
        *loaderResult.pLoader,
        *loaderResult.pRootTile);
    CHECK(tileLoadResult.updatedBoundingVolume == std::nullopt);
    CHECK(tileLoadResult.updatedContentBoundingVolume == std::nullopt);
    CHECK(tileLoadResult.state == TileLoadResultState::Success);
    CHECK(!tileLoadResult.tileInitializer);

    const auto& renderContent =
        std::get<TileRenderContent>(tileLoadResult.contentKind);
    CHECK(renderContent.model);
  }

  SECTION("Load tile that has external content") {
    auto tilesetData = readFile(testDataPath / "AddTileset" / "tileset.json");

    auto loaderResult = TilesetJsonLoader::createLoader(
                            createMockTilesetExternals(std::move(tilesetData)),
                            "tileset.json",
                            {})
                            .wait();

    CHECK(loaderResult.pRootTile);

    const auto& tileID =
        std::get<std::string>(loaderResult.pRootTile->getTileID());
    CHECK(tileID == "tileset2.json");

    // check tile content
    auto tileLoadResult = loadTileContent(
        testDataPath / "AddTileset" / tileID,
        *loaderResult.pLoader,
        *loaderResult.pRootTile);
    CHECK(tileLoadResult.updatedBoundingVolume == std::nullopt);
    CHECK(tileLoadResult.updatedContentBoundingVolume == std::nullopt);
    CHECK(std::holds_alternative<TileExternalContent>(
        tileLoadResult.contentKind));
    CHECK(tileLoadResult.state == TileLoadResultState::Success);
    CHECK(tileLoadResult.tileInitializer);

    // check tile is really an external tile
    loaderResult.pRootTile->getContent().setContentKind(
        tileLoadResult.contentKind);
    tileLoadResult.tileInitializer(*loaderResult.pRootTile);
    const auto& children = loaderResult.pRootTile->getChildren();
    CHECK(children.size() == 1);

    const Tile& parentB3dmTile = children[0];
    CHECK(std::get<std::string>(parentB3dmTile.getTileID()) == "parent.b3dm");
    CHECK(parentB3dmTile.getGeometricError() == Approx(70.0));

    std::vector<std::string> expectedChildUrls{
        "tileset3/tileset3.json",
        "lr.b3dm",
        "ur.b3dm",
        "ul.b3dm"};
    const auto& parentB3dmChildren = parentB3dmTile.getChildren();
    for (std::size_t i = 0; i < parentB3dmChildren.size(); ++i) {
      const Tile& child = parentB3dmChildren[i];
      CHECK(std::get<std::string>(child.getTileID()) == expectedChildUrls[i]);
      CHECK(child.getGeometricError() == Approx(0.0));
      CHECK(child.getRefine() == TileRefine::Add);
      CHECK(std::holds_alternative<CesiumGeospatial::BoundingRegion>(
          child.getBoundingVolume()));
    }
  }

  SECTION("Load tile that has external content with implicit extensions") {
    auto tilesetData =
        readFile(testDataPath / "ImplicitTileset" / "tileset.json");

    auto loaderResult = TilesetJsonLoader::createLoader(
                            createMockTilesetExternals(std::move(tilesetData)),
                            "tileset.json",
                            {})
                            .wait();

    CHECK(loaderResult.pRootTile);
    CHECK(loaderResult.pRootTile->isExternalContent());
    CHECK(loaderResult.pRootTile->getChildren().size() == 1);

    auto& implicitTile = loaderResult.pRootTile->getChildren().front();
    const auto& tileID =
        std::get<CesiumGeometry::QuadtreeTileID>(implicitTile.getTileID());
    CHECK(tileID == CesiumGeometry::QuadtreeTileID(0, 0, 0));

    // mock subtree content response
    auto subtreePath =
        testDataPath / "ImplicitTileset" / "subtrees" / "0.0.0.json";

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
        testDataPath / "ImplicitTileset" / "content" / "0" / "0" / "0.b3dm";

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
            {"subtrees/0.0.0.json", std::move(pMockSubtreeRequest)},
            {"content/0/0/0.b3dm", std::move(pMockTileRequest)}};

    std::shared_ptr<SimpleAssetAccessor> pMockAssetAccessor =
        std::make_shared<SimpleAssetAccessor>(std::move(mockCompletedRequests));

    AsyncSystem asyncSystem{std::make_shared<SimpleTaskProcessor>()};

    {
      // loader will tell to retry later since it needs subtree
      auto implicitContentResultFuture = loaderResult.pLoader->loadTileContent(
          implicitTile,
          {},
          asyncSystem,
          pMockAssetAccessor,
          spdlog::default_logger(),
          {});

      asyncSystem.dispatchMainThreadTasks();

      auto implicitContentResult = implicitContentResultFuture.wait();
      CHECK(implicitContentResult.state == TileLoadResultState::RetryLater);
    }

    {
      // loader will be able to load the tile the second time around
      auto implicitContentResultFuture = loaderResult.pLoader->loadTileContent(
          implicitTile,
          {},
          asyncSystem,
          pMockAssetAccessor,
          spdlog::default_logger(),
          {});

      asyncSystem.dispatchMainThreadTasks();

      auto implicitContentResult = implicitContentResultFuture.wait();
      const auto& renderContent =
          std::get<TileRenderContent>(implicitContentResult.contentKind);
      CHECK(renderContent.model);
      CHECK(!implicitContentResult.updatedBoundingVolume);
      CHECK(!implicitContentResult.updatedContentBoundingVolume);
      CHECK(implicitContentResult.state == TileLoadResultState::Success);
      CHECK(!implicitContentResult.tileInitializer);
    }
  }
}
