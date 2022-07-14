#include "ImplicitOctreeLoader.h"
#include "SimpleAssetAccessor.h"
#include "SimpleAssetRequest.h"
#include "SimpleAssetResponse.h"
#include "SimpleTaskProcessor.h"
#include "readFile.h"

#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/registerAllTileContentTypes.h>
#include <CesiumGeometry/OrientedBoundingBox.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/S2CellBoundingVolume.h>
#include <CesiumUtility/Math.h>

#include <catch2/catch.hpp>

#include <filesystem>

using namespace Cesium3DTilesSelection;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

namespace {
std::filesystem::path testDataPath = Cesium3DTilesSelection_TEST_DATA_DIR;
}

TEST_CASE("Test implicit octree loader") {
  Cesium3DTilesSelection::registerAllTileContentTypes();

  auto pMockedAssetAccessor = std::make_shared<SimpleAssetAccessor>(
      std::map<std::string, std::shared_ptr<SimpleAssetRequest>>{});

  CesiumAsync::AsyncSystem asyncSystem{std::make_shared<SimpleTaskProcessor>()};

  ImplicitOctreeLoader loader{
      "tileset.json",
      "content/{level}.{x}.{y}.{z}.b3dm",
      "subtrees/{level}.{x}.{y}.{z}.json",
      5,
      5,
      OrientedBoundingBox(glm::dvec3(0.0), glm::dmat3(20.0))};

  SECTION("Load tile that does not have quadtree ID") {
    Tile tile(&loader);
    tile.setTileID("This is a test tile");

    auto tileLoadResultFuture = loader.loadTileContent(
        tile,
        {},
        asyncSystem,
        pMockedAssetAccessor,
        spdlog::default_logger(),
        {});

    asyncSystem.dispatchMainThreadTasks();

    auto tileLoadResult = tileLoadResultFuture.wait();
    CHECK(tileLoadResult.state == TileLoadResultState::Failed);
  }

  SECTION("Load empty tile") {
    // add subtree with all empty tiles
    loader.addSubtreeAvailability(
        OctreeTileID{0, 0, 0, 0},
        SubtreeAvailability{
            3,
            SubtreeConstantAvailability{true},
            SubtreeConstantAvailability{false},
            {SubtreeConstantAvailability{false}},
            {}});

    // check that this tile will have empty content
    Tile tile(&loader);
    tile.setTileID(OctreeTileID{1, 0, 1, 1});

    auto tileLoadResultFuture = loader.loadTileContent(
        tile,
        {},
        asyncSystem,
        pMockedAssetAccessor,
        spdlog::default_logger(),
        {});

    asyncSystem.dispatchMainThreadTasks();

    auto tileLoadResult = tileLoadResultFuture.wait();
    CHECK(std::holds_alternative<TileEmptyContent>(tileLoadResult.contentKind));
    CHECK(!tileLoadResult.updatedBoundingVolume);
    CHECK(!tileLoadResult.updatedContentBoundingVolume);
    CHECK(!tileLoadResult.tileInitializer);
    CHECK(tileLoadResult.state == TileLoadResultState::Success);
  }

  SECTION("Load tile with render content") {
    // add subtree with all available tiles
    loader.addSubtreeAvailability(
        OctreeTileID{0, 0, 0, 0},
        SubtreeAvailability{
            2,
            SubtreeConstantAvailability{true},
            SubtreeConstantAvailability{false},
            {SubtreeConstantAvailability{true}},
            {}});

    // mock tile content b3dm
    auto pMockCompletedResponse = std::make_unique<SimpleAssetResponse>(
        static_cast<uint16_t>(200),
        "doesn't matter",
        CesiumAsync::HttpHeaders{},
        readFile(testDataPath / "BatchTables" / "batchedWithJson.b3dm"));

    auto pMockCompletedRequest = std::make_shared<SimpleAssetRequest>(
        "GET",
        "doesn't matter",
        CesiumAsync::HttpHeaders{},
        std::move(pMockCompletedResponse));

    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"content/3.1.0.1.b3dm", std::move(pMockCompletedRequest)});

    // check that this tile has render content
    Tile tile(&loader);
    tile.setTileID(OctreeTileID{3, 1, 0, 1});

    auto tileLoadResultFuture = loader.loadTileContent(
        tile,
        {},
        asyncSystem,
        pMockedAssetAccessor,
        spdlog::default_logger(),
        {});

    asyncSystem.dispatchMainThreadTasks();

    auto tileLoadResult = tileLoadResultFuture.wait();
    const auto& renderContent =
        std::get<TileRenderContent>(tileLoadResult.contentKind);
    CHECK(renderContent.model);

    CHECK(!tileLoadResult.updatedBoundingVolume);
    CHECK(!tileLoadResult.updatedContentBoundingVolume);
    CHECK(!tileLoadResult.tileInitializer);
    CHECK(tileLoadResult.state == TileLoadResultState::Success);
  }

  SECTION("Load unknown tile content") {
    // add subtree with all available tiles
    loader.addSubtreeAvailability(
        OctreeTileID{0, 0, 0, 0},
        SubtreeAvailability{
            2,
            SubtreeConstantAvailability{true},
            SubtreeConstantAvailability{false},
            {SubtreeConstantAvailability{true}},
            {}});

    // mock random tile content
    auto pMockCompletedResponse = std::make_unique<SimpleAssetResponse>(
        static_cast<uint16_t>(200),
        "doesn't matter",
        CesiumAsync::HttpHeaders{},
        std::vector<std::byte>(20));

    auto pMockCompletedRequest = std::make_shared<SimpleAssetRequest>(
        "GET",
        "doesn't matter",
        CesiumAsync::HttpHeaders{},
        std::move(pMockCompletedResponse));

    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"content/1.0.1.0.b3dm", std::move(pMockCompletedRequest)});

    // check that this tile has render content
    Tile tile(&loader);
    tile.setTileID(OctreeTileID{1, 0, 1, 0});

    auto tileLoadResultFuture = loader.loadTileContent(
        tile,
        {},
        asyncSystem,
        pMockedAssetAccessor,
        spdlog::default_logger(),
        {});

    asyncSystem.dispatchMainThreadTasks();

    auto tileLoadResult = tileLoadResultFuture.wait();
    CHECK(tileLoadResult.state == TileLoadResultState::Failed);

  }
}
