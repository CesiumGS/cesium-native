#include "SimpleAssetAccessor.h"
#include "SimpleAssetRequest.h"
#include "SimpleAssetResponse.h"
#include "SimplePrepareRendererResource.h"
#include "SimpleTaskProcessor.h"
#include "TilesetContentManager.h"
#include "readFile.h"

#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/registerAllTileContentTypes.h>

#include <catch2/catch.hpp>

using namespace Cesium3DTilesSelection;

namespace {
class SimpleTilesetContentLoader : public TilesetContentLoader {
public:
  CesiumAsync::Future<TileLoadResult>
  loadTileContent(const TileLoadInput& input) {
    return input.asyncSystem.createResolvedFuture(
        std::move(mockLoadTileContent));
  }

  TileChildrenResult createTileChildren([[maybe_unused]] const Tile& tile) {
    return std::move(mockCreateTileChildren);
  }

  TileLoadResult mockLoadTileContent;
  TileChildrenResult mockCreateTileChildren;
};

CesiumGltf::Model createCubeMesh() { return {}; }
} // namespace

TEST_CASE("Test tile state machine") {
  Cesium3DTilesSelection::registerAllTileContentTypes();

  // create mock tileset externals
  auto pMockedAssetAccessor = std::make_shared<SimpleAssetAccessor>(
      std::map<std::string, std::shared_ptr<SimpleAssetRequest>>{});
  auto pMockedPrepareRendererResources =
      std::make_shared<SimplePrepareRendererResource>();
  CesiumAsync::AsyncSystem asyncSystem{std::make_shared<SimpleTaskProcessor>()};
  auto pMockedCreditSystem = std::make_shared<CreditSystem>();

  TilesetExternals externals{
      pMockedAssetAccessor,
      pMockedPrepareRendererResources,
      asyncSystem,
      pMockedCreditSystem};

  // create mock raster overlay collection
  Tile::LoadedLinkedList loadedTiles;
  RasterOverlayCollection rasterOverlayCollection{loadedTiles, externals};

  SECTION("Load render content successfully") {
    // create mock loader
    bool initializerCall = false;
    auto pMockedLoader = std::make_unique<SimpleTilesetContentLoader>();
    pMockedLoader->mockLoadTileContent = {
        TileRenderContent{createCubeMesh()},
        std::nullopt,
        std::nullopt,
        std::nullopt,
        nullptr,
        [&](Tile&) { initializerCall = true; },
        TileLoadResultState::Success};
    pMockedLoader->mockCreateTileChildren = {{}, TileLoadResultState::Success};
    pMockedLoader->mockCreateTileChildren.children.emplace_back(
        pMockedLoader.get(),
        TileEmptyContent());

    // create tile
    Tile tile(pMockedLoader.get());

    // create manager
    TilesetContentManager manager{
        externals,
        {},
        std::move(pMockedLoader),
        rasterOverlayCollection};

    // test manager loading
    TilesetOptions options{};
    options.contentOptions.generateMissingNormalsSmooth = true;
    manager.loadTileContent(tile, options);

    // ensure that tile move from ContentLoading to ContentLoaded
    // check the state of the tile before main thread get called
    CHECK(manager.getNumOfTilesLoading() == 1);
    CHECK(tile.getState() == TileLoadState::ContentLoading);
    CHECK(!tile.getContent().isRenderContent());
    CHECK(!tile.getContent().isExternalContent());
    CHECK(!tile.getContent().isEmptyContent());
    CHECK(!tile.getContent().getRenderResources());
    CHECK(!tile.getContent().getTileInitializerCallback());
    CHECK(!initializerCall);

    // check the state of the tile after main thread get called
    asyncSystem.dispatchMainThreadTasks();
    CHECK(manager.getNumOfTilesLoading() == 0);
    CHECK(tile.getState() == TileLoadState::ContentLoaded);
    CHECK(tile.getContent().isRenderContent());
    CHECK(tile.getContent().getRenderResources());
    CHECK(tile.getContent().getTileInitializerCallback());
    CHECK(!initializerCall);

    // update tile content to move from ContentLoaded -> Done
    manager.updateTileContent(tile, options);
    CHECK(tile.getState() == TileLoadState::Done);
    CHECK(tile.getChildren().size() == 1);
    CHECK(tile.getChildren().front().getContent().isEmptyContent());
    CHECK(tile.getContent().isRenderContent());
    CHECK(tile.getContent().getRenderResources());
    CHECK(!tile.getContent().getTileInitializerCallback());
    CHECK(initializerCall);

    // unload tile to move from Done -> Unload
    manager.unloadTileContent(tile);
    CHECK(tile.getState() == TileLoadState::Unloaded);
    CHECK(!tile.getContent().isRenderContent());
    CHECK(!tile.getContent().getRenderResources());
    CHECK(!tile.getContent().getTileInitializerCallback());
  }

  SECTION("") {}
}
