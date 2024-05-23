#include "ImplicitQuadtreeLoader.h"
#include "SimplePrepareRendererResource.h"
#include "TilesetJsonLoader.h"

#include <Cesium3DTilesContent/registerAllTileContentTypes.h>
#include <Cesium3DTilesSelection/TileWorkManager.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumNativeTests/SimpleAssetResponse.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/readFile.h>

#include <catch2/catch.hpp>

#include <cstddef>
#include <memory>
#include <string>

using namespace CesiumAsync;
using namespace Cesium3DTilesSelection;
using namespace CesiumNativeTests;
using namespace CesiumUtility;

namespace {
std::filesystem::path testDataPath = Cesium3DTilesSelection_TEST_DATA_DIR;

TilesetExternals createMockTilesetExternals(const std::string& tilesetPath) {
  auto tilesetContent = readFile(tilesetPath);
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
  mockCompletedRequests.insert({tilesetPath, std::move(pMockCompletedRequest)});

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

TilesetContentLoaderResult<TilesetJsonLoader>
createLoader(const std::filesystem::path& tilesetPath) {
  std::string tilesetPathStr = tilesetPath.string();
  auto externals = createMockTilesetExternals(tilesetPathStr);
  auto loaderResultFuture =
      TilesetJsonLoader::createLoader(externals, tilesetPathStr, {});
  externals.asyncSystem.dispatchMainThreadTasks();

  return loaderResultFuture.wait();
}

TileLoadResult loadTileContent(
    const std::filesystem::path& tilePath,
    const std::string& tileUrl,
    TilesetContentLoader& loader,
    Tile& tile) {
  auto pMockCompletedResponse = std::make_unique<SimpleAssetResponse>(
      static_cast<uint16_t>(200),
      "doesn't matter",
      CesiumAsync::HttpHeaders{},
      readFile(tilePath));
  auto pMockCompletedRequest = std::make_shared<SimpleAssetRequest>(
      "GET",
      tileUrl,
      CesiumAsync::HttpHeaders{},
      std::move(pMockCompletedResponse));

  std::map<std::string, std::shared_ptr<SimpleAssetRequest>>
      mockCompletedRequests;
  mockCompletedRequests.insert(
      {tilePath.filename().string(), std::move(pMockCompletedRequest)});

  std::shared_ptr<SimpleAssetAccessor> pMockAssetAccessor =
      std::make_shared<SimpleAssetAccessor>(std::move(mockCompletedRequests));

  AsyncSystem asyncSystem{std::make_shared<SimpleTaskProcessor>()};

  RequestData requestData;
  TileLoaderCallback processingCallback;
  loader.getLoadWork(&tile, requestData, processingCallback);

  std::shared_ptr<TileWorkManager> workManager =
      std::make_shared<TileWorkManager>(
          asyncSystem,
          pMockAssetAccessor,
          spdlog::default_logger());

  std::vector<TileWorkManager::Order> orders;
  orders.push_back(TileWorkManager::Order{
      requestData,
      TileProcessingData{&tile, processingCallback},
      TileLoadPriorityGroup::Normal,
      0});

  TileWorkManager::TileDispatchFunc tileDispatch =
      [pLoader = &loader, asyncSystem, workManager](
          TileProcessingData& processingData,
          const CesiumAsync::UrlResponseDataMap& responseDataMap,
          TileWorkManager::Work* work) mutable {
        assert(processingData.pTile);
        assert(processingData.loaderCallback);
        Tile* pTile = processingData.pTile;

        TileLoadInput loadInput{
            *pTile,
            {},
            asyncSystem,
            spdlog::default_logger(),
            responseDataMap};

        processingData.loaderCallback(loadInput, pLoader)
            .thenInMainThread(
                [_work = work, workManager](TileLoadResult&& result) mutable {
                  _work->tileLoadResult = std::move(result);
                  workManager->SignalWorkComplete(_work);
                  TileWorkManager::TryDispatchProcessing(workManager);
                });
      };

  TileWorkManager::RasterDispatchFunc rasterDispatch =
      [](RasterProcessingData&,
         const CesiumAsync::UrlResponseDataMap&,
         TileWorkManager::Work*) {};

  workManager->SetDispatchFunctions(tileDispatch, rasterDispatch);

  size_t maxRequests = 20;
  std::vector<const TileWorkManager::Work*> workCreated;
  TileWorkManager::TryAddOrders(workManager, orders, maxRequests, workCreated);
  assert(workCreated.size() == 1);

  asyncSystem.dispatchMainThreadTasks();

  std::vector<TileWorkManager::DoneOrder> doneOrders;
  std::vector<TileWorkManager::FailedOrder> failedOrders;
  workManager->TakeCompletedWork(doneOrders, failedOrders);

  assert(doneOrders.size() == 1);
  assert(failedOrders.size() == 0);

  auto tileLoadResult = doneOrders.begin()->loadResult;

  return tileLoadResult;
}
} // namespace

TEST_CASE("Test creating tileset json loader") {
  Cesium3DTilesContent::registerAllTileContentTypes();

  SECTION("Create valid tileset json with REPLACE refinement") {
    auto loaderResult =
        createLoader(testDataPath / "ReplaceTileset" / "tileset.json");

    CHECK(!loaderResult.errors.hasErrors());

    // check root tile
    auto pTilesetJson = loaderResult.pRootTile.get();
    REQUIRE(pTilesetJson);
    REQUIRE(pTilesetJson->getChildren().size() == 1);
    CHECK(pTilesetJson->getParent() == nullptr);
    auto pRootTile = &pTilesetJson->getChildren()[0];
    CHECK(pRootTile->getParent() == pTilesetJson);
    REQUIRE(pRootTile->getChildren().size() == 4);
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

    // check loader up axis
    CHECK(loaderResult.pLoader->getUpAxis() == CesiumGeometry::Axis::Y);
  }

  SECTION("Create valid tileset json with ADD refinement") {
    auto loaderResult =
        createLoader(testDataPath / "AddTileset" / "tileset2.json");

    CHECK(!loaderResult.errors.hasErrors());

    // check root tile
    auto pTilesetJson = loaderResult.pRootTile.get();
    REQUIRE(pTilesetJson != nullptr);
    CHECK(pTilesetJson->getParent() == nullptr);
    REQUIRE(pTilesetJson->getChildren().size() == 1);
    auto pRootTile = &pTilesetJson->getChildren()[0];
    CHECK(pRootTile->getParent() == pTilesetJson);
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

    // check loader up axis
    CHECK(loaderResult.pLoader->getUpAxis() == CesiumGeometry::Axis::Y);
  }

  SECTION("Tileset has tile with sphere bounding volume") {
    auto loaderResult = createLoader(
        testDataPath / "MultipleKindsOfTilesets" /
        "SphereBoundingVolumeTileset.json");

    REQUIRE(loaderResult.pRootTile);
    REQUIRE(loaderResult.pRootTile->getChildren().size() == 1);

    const Tile& rootTile = loaderResult.pRootTile->getChildren()[0];
    const CesiumGeometry::BoundingSphere& sphere =
        std::get<CesiumGeometry::BoundingSphere>(rootTile.getBoundingVolume());
    CHECK(sphere.getCenter() == glm::dvec3(0.0, 0.0, 10.0));
    CHECK(sphere.getRadius() == 141.4214);

    // check loader up axis
    CHECK(loaderResult.pLoader->getUpAxis() == CesiumGeometry::Axis::Y);
  }

  SECTION("Tileset has tile with box bounding volume") {
    auto loaderResult = createLoader(
        testDataPath / "MultipleKindsOfTilesets" /
        "BoxBoundingVolumeTileset.json");

    REQUIRE(loaderResult.pRootTile);
    REQUIRE(loaderResult.pRootTile->getChildren().size() == 1);

    const Tile& rootTile = loaderResult.pRootTile->getChildren()[0];
    const CesiumGeometry::OrientedBoundingBox& box =
        std::get<CesiumGeometry::OrientedBoundingBox>(
            rootTile.getBoundingVolume());
    const glm::dmat3& halfAxis = box.getHalfAxes();
    CHECK(halfAxis[0] == glm::dvec3(100.0, 0.0, 0.0));
    CHECK(halfAxis[1] == glm::dvec3(0.0, 100.0, 0.0));
    CHECK(halfAxis[2] == glm::dvec3(0.0, 0.0, 10.0));
    CHECK(box.getCenter() == glm::dvec3(0.0, 0.0, 10.0));
  }

  SECTION("Tileset has tile with no bounding volume field") {
    auto loaderResult = createLoader(
        testDataPath / "MultipleKindsOfTilesets" /
        "NoBoundingVolumeTileset.json");

    CHECK(!loaderResult.errors.hasErrors());
    REQUIRE(loaderResult.pRootTile);
    REQUIRE(loaderResult.pRootTile->getChildren().size() == 1);
    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];
    CHECK(pRootTile->getChildren().empty());

    // check loader up axis
    CHECK(loaderResult.pLoader->getUpAxis() == CesiumGeometry::Axis::Y);
  }

  SECTION("Tileset has tile with no geometric error field") {
    auto loaderResult = createLoader(
        testDataPath / "MultipleKindsOfTilesets" /
        "NoGeometricErrorTileset.json");

    CHECK(!loaderResult.errors.hasErrors());
    REQUIRE(loaderResult.pRootTile);
    REQUIRE(loaderResult.pRootTile->getChildren().size() == 1);
    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];
    CHECK(pRootTile->getGeometricError() == Approx(70.0));
    CHECK(pRootTile->getChildren().size() == 4);
    for (const Tile& child : pRootTile->getChildren()) {
      CHECK(child.getGeometricError() == Approx(35.0));
    }

    // check loader up axis
    CHECK(loaderResult.pLoader->getUpAxis() == CesiumGeometry::Axis::Y);
  }

  SECTION("Tileset has tile with no capitalized Refinement field") {
    auto loaderResult = createLoader(
        testDataPath / "MultipleKindsOfTilesets" /
        "NoCapitalizedRefineTileset.json");

    CHECK(!loaderResult.errors.hasErrors());
    REQUIRE(loaderResult.pRootTile);
    REQUIRE(loaderResult.pRootTile->getChildren().size() == 1);
    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];
    CHECK(pRootTile->getGeometricError() == Approx(70.0));
    CHECK(pRootTile->getRefine() == TileRefine::Add);
    CHECK(pRootTile->getChildren().size() == 4);
    for (const Tile& child : pRootTile->getChildren()) {
      CHECK(child.getGeometricError() == Approx(5.0));
      CHECK(child.getRefine() == TileRefine::Replace);
    }

    // check loader up axis
    CHECK(loaderResult.pLoader->getUpAxis() == CesiumGeometry::Axis::Y);
  }

  SECTION("Scale geometric error along with tile transform") {
    auto loaderResult = createLoader(
        testDataPath / "MultipleKindsOfTilesets" /
        "ScaleGeometricErrorTileset.json");

    CHECK(!loaderResult.errors.hasErrors());
    REQUIRE(loaderResult.pRootTile);
    REQUIRE(loaderResult.pRootTile->getChildren().size() == 1);
    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];
    CHECK(pRootTile->getGeometricError() == Approx(210.0));
    CHECK(pRootTile->getChildren().size() == 4);
    for (const Tile& child : pRootTile->getChildren()) {
      CHECK(child.getGeometricError() == Approx(15.0));
    }

    // check loader up axis
    CHECK(loaderResult.pLoader->getUpAxis() == CesiumGeometry::Axis::Y);
  }

  SECTION("Tileset with empty tile") {
    auto loaderResult = createLoader(
        testDataPath / "MultipleKindsOfTilesets" / "EmptyTileTileset.json");
    CHECK(!loaderResult.errors.hasErrors());
    REQUIRE(loaderResult.pRootTile);
    REQUIRE(loaderResult.pRootTile->getChildren().size() == 1);
    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];
    CHECK(pRootTile->getGeometricError() == Approx(70.0));
    CHECK(pRootTile->getChildren().size() == 1);

    const Tile& child = pRootTile->getChildren().front();
    CHECK(child.isEmptyContent());

    // check loader up axis
    CHECK(loaderResult.pLoader->getUpAxis() == CesiumGeometry::Axis::Y);
  }

  SECTION("Tileset with quadtree implicit tile") {
    auto loaderResult = createLoader(
        testDataPath / "MultipleKindsOfTilesets" /
        "QuadtreeImplicitTileset.json");
    CHECK(!loaderResult.errors.hasErrors());
    REQUIRE(loaderResult.pRootTile);
    CHECK(loaderResult.pRootTile->isExternalContent());
    REQUIRE(loaderResult.pRootTile->getChildren().size() == 1);
    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];
    CHECK(pRootTile->isExternalContent());
    REQUIRE(pRootTile->getChildren().size() == 1);
    CHECK(pRootTile->getTransform() == glm::dmat4(glm::dmat3(2.0)));

    const Tile& child = pRootTile->getChildren().front();
    CHECK(child.getLoader() != loaderResult.pLoader.get());
    CHECK(child.getGeometricError() == pRootTile->getGeometricError());
    CHECK(child.getRefine() == pRootTile->getRefine());
    CHECK(child.getTransform() == pRootTile->getTransform());
    CHECK(
        std::get<CesiumGeometry::QuadtreeTileID>(child.getTileID()) ==
        CesiumGeometry::QuadtreeTileID(0, 0, 0));
  }

  SECTION("Tileset with octree implicit tile") {
    auto loaderResult = createLoader(
        testDataPath / "MultipleKindsOfTilesets" /
        "OctreeImplicitTileset.json");
    CHECK(!loaderResult.errors.hasErrors());
    REQUIRE(loaderResult.pRootTile);
    CHECK(loaderResult.pRootTile->isExternalContent());
    REQUIRE(loaderResult.pRootTile->getChildren().size() == 1);
    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];
    CHECK(pRootTile->isExternalContent());
    REQUIRE(pRootTile->getChildren().size() == 1);
    CHECK(pRootTile->getTransform() == glm::dmat4(glm::dmat3(2.0)));

    const Tile& child = pRootTile->getChildren().front();
    CHECK(child.getLoader() != loaderResult.pLoader.get());
    CHECK(child.getGeometricError() == pRootTile->getGeometricError());
    CHECK(child.getRefine() == pRootTile->getRefine());
    CHECK(child.getTransform() == pRootTile->getTransform());
    CHECK(
        std::get<CesiumGeometry::OctreeTileID>(child.getTileID()) ==
        CesiumGeometry::OctreeTileID(0, 0, 0, 0));
  }

  SECTION("Tileset with metadata") {
    auto loaderResult =
        createLoader(testDataPath / "WithMetadata" / "tileset.json");

    CHECK(!loaderResult.errors.hasErrors());
    REQUIRE(loaderResult.pLoader);
    REQUIRE(loaderResult.pRootTile);

    TileExternalContent* pExternal =
        loaderResult.pRootTile->getContent().getExternalContent();
    REQUIRE(pExternal);

    const TilesetMetadata& metadata = pExternal->metadata;
    const std::optional<Cesium3DTiles::Schema>& schema = metadata.schema;
    REQUIRE(schema);
    CHECK(schema->id == "foo");
  }
}

TEST_CASE("Test loading individual tile of tileset json") {
  Cesium3DTilesContent::registerAllTileContentTypes();

  SECTION("Load tile that has render content") {
    auto loaderResult =
        createLoader(testDataPath / "ReplaceTileset" / "tileset.json");
    REQUIRE(loaderResult.pRootTile);
    REQUIRE(loaderResult.pRootTile->getChildren().size() == 1);

    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];

    const auto& tileID = std::get<std::string>(pRootTile->getTileID());
    CHECK(tileID == "parent.b3dm");

    // check tile content
    auto tileLoadResult = loadTileContent(
        testDataPath / "ReplaceTileset" / tileID,
        "parent.b3dm",
        *loaderResult.pLoader,
        *pRootTile);
    CHECK(
        std::holds_alternative<CesiumGltf::Model>(tileLoadResult.contentKind));
    CHECK(tileLoadResult.updatedBoundingVolume == std::nullopt);
    CHECK(tileLoadResult.updatedContentBoundingVolume == std::nullopt);
    CHECK(tileLoadResult.state == TileLoadResultState::Success);
    CHECK(!tileLoadResult.tileInitializer);
  }

  SECTION("Load tile that has external content") {
    auto loaderResult =
        createLoader(testDataPath / "AddTileset" / "tileset.json");

    REQUIRE(loaderResult.pRootTile);
    REQUIRE(loaderResult.pRootTile->getChildren().size() == 1);

    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];

    const auto& tileID = std::get<std::string>(pRootTile->getTileID());
    CHECK(tileID == "tileset2.json");

    // check tile content
    auto tileLoadResult = loadTileContent(
        testDataPath / "AddTileset" / tileID,
        "tileset2.json",
        *loaderResult.pLoader,
        *pRootTile);
    CHECK(tileLoadResult.updatedBoundingVolume == std::nullopt);
    CHECK(tileLoadResult.updatedContentBoundingVolume == std::nullopt);
    CHECK(std::holds_alternative<TileExternalContent>(
        tileLoadResult.contentKind));
    CHECK(tileLoadResult.state == TileLoadResultState::Success);
    CHECK(tileLoadResult.tileInitializer);

    // check tile is really an external tile
    pRootTile->getContent().setContentKind(
        std::make_unique<TileExternalContent>(
            std::get<TileExternalContent>(tileLoadResult.contentKind)));
    tileLoadResult.tileInitializer(*pRootTile);
    const auto& children = pRootTile->getChildren();
    REQUIRE(children.size() == 1);

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

  SECTION("Load tile that has external content with implicit tiling") {
    auto loaderResult =
        createLoader(testDataPath / "ImplicitTileset" / "tileset_1.1.json");

    REQUIRE(loaderResult.pRootTile);
    CHECK(loaderResult.pRootTile->isExternalContent());
    REQUIRE(loaderResult.pRootTile->getChildren().size() == 1);

    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];
    REQUIRE(pRootTile->getChildren().size() == 1);

    auto& implicitTile = pRootTile->getChildren().front();
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

      UrlResponseDataMap responseDataMap;
      pMockAssetAccessor->fillResponseDataMap(responseDataMap);

      TileLoadInput loadInput{
          implicitTile,
          {},
          asyncSystem,
          spdlog::default_logger(),
          responseDataMap};
      auto implicitContentResultFuture =
          loaderResult.pLoader->loadTileContent(loadInput);

      asyncSystem.dispatchMainThreadTasks();

      auto implicitContentResult = implicitContentResultFuture.wait();
      CHECK(implicitContentResult.state == TileLoadResultState::RetryLater);
    }

    {
      // loader will be able to load the tile the second time around
      UrlResponseDataMap responseDataMap;
      pMockAssetAccessor->fillResponseDataMap(responseDataMap);

      TileLoadInput loadInput{
          implicitTile,
          {},
          asyncSystem,
          spdlog::default_logger(),
          responseDataMap};
      auto implicitContentResultFuture =
          loaderResult.pLoader->loadTileContent(loadInput);

      asyncSystem.dispatchMainThreadTasks();

      auto implicitContentResult = implicitContentResultFuture.wait();
      CHECK(std::holds_alternative<CesiumGltf::Model>(
          implicitContentResult.contentKind));
      CHECK(!implicitContentResult.updatedBoundingVolume);
      CHECK(!implicitContentResult.updatedContentBoundingVolume);
      CHECK(implicitContentResult.state == TileLoadResultState::Success);
      CHECK(!implicitContentResult.tileInitializer);
    }
  }

  SECTION("Check that tile with legacy implicit tiling extension still works") {
    auto loaderResult =
        createLoader(testDataPath / "ImplicitTileset" / "tileset_1.0.json");

    REQUIRE(loaderResult.pRootTile);
    CHECK(loaderResult.pRootTile->isExternalContent());
    REQUIRE(loaderResult.pRootTile->getChildren().size() == 1);

    auto pRootTile = &loaderResult.pRootTile->getChildren()[0];
    REQUIRE(pRootTile->getChildren().size() == 1);

    auto& implicitTile = pRootTile->getChildren().front();
    const auto& tileID =
        std::get<CesiumGeometry::QuadtreeTileID>(implicitTile.getTileID());
    CHECK(tileID == CesiumGeometry::QuadtreeTileID(0, 0, 0));

    const auto pLoader =
        dynamic_cast<const ImplicitQuadtreeLoader*>(implicitTile.getLoader());
    CHECK(pLoader);
    CHECK(pLoader->getSubtreeLevels() == 2);
    CHECK(pLoader->getAvailableLevels() == 2);
  }
}
