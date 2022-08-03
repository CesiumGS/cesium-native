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
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGltfReader/GltfReader.h>

#include <catch2/catch.hpp>
#include <glm/glm.hpp>

#include <filesystem>
#include <vector>

using namespace Cesium3DTilesSelection;
using namespace CesiumGeometry;

namespace {
std::filesystem::path testDataPath = Cesium3DTilesSelection_TEST_DATA_DIR;

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

std::shared_ptr<SimpleAssetRequest>
createMockRequest(const std::filesystem::path& path) {
  auto pMockCompletedResponse = std::make_unique<SimpleAssetResponse>(
      static_cast<uint16_t>(200),
      "doesn't matter",
      CesiumAsync::HttpHeaders{},
      readFile(path));

  auto pMockCompletedRequest = std::make_shared<SimpleAssetRequest>(
      "GET",
      "doesn't matter",
      CesiumAsync::HttpHeaders{},
      std::move(pMockCompletedResponse));

  return pMockCompletedRequest;
}
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

  SECTION("Load content successfully") {
    // create mock loader
    bool initializerCall = false;
    auto pMockedLoader = std::make_unique<SimpleTilesetContentLoader>();
    pMockedLoader->mockLoadTileContent = {
        TileRenderContent{CesiumGltf::Model()},
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

    SECTION("Load tile from ContentLoading -> Done") {
      // Unloaded -> ContentLoading
      // check the state of the tile before main thread get called
      CHECK(manager.getNumOfTilesLoading() == 1);
      CHECK(tile.getState() == TileLoadState::ContentLoading);
      CHECK(!tile.getContent().isRenderContent());
      CHECK(!tile.getContent().isExternalContent());
      CHECK(!tile.getContent().isEmptyContent());
      CHECK(!tile.getContent().getRenderResources());
      CHECK(!tile.getContent().getTileInitializerCallback());
      CHECK(!initializerCall);

      // ContentLoading -> ContentLoaded
      // check the state of the tile after main thread get called
      manager.waitIdle();
      CHECK(manager.getNumOfTilesLoading() == 0);
      CHECK(tile.getState() == TileLoadState::ContentLoaded);
      CHECK(tile.getContent().isRenderContent());
      CHECK(tile.getContent().getRenderResources());
      CHECK(tile.getContent().getTileInitializerCallback());
      CHECK(!initializerCall);

      // ContentLoaded -> Done
      // update tile content to move from ContentLoaded -> Done
      manager.updateTileContent(tile, options);
      CHECK(tile.getState() == TileLoadState::Done);
      CHECK(tile.getChildren().size() == 1);
      CHECK(tile.getChildren().front().getContent().isEmptyContent());
      CHECK(tile.getContent().isRenderContent());
      CHECK(tile.getContent().getRenderResources());
      CHECK(!tile.getContent().getTileInitializerCallback());
      CHECK(initializerCall);

      // Done -> Unloaded
      manager.unloadTileContent(tile);
      CHECK(tile.getState() == TileLoadState::Unloaded);
      CHECK(!tile.getContent().isRenderContent());
      CHECK(!tile.getContent().getRenderResources());
      CHECK(!tile.getContent().getTileInitializerCallback());
    }

    SECTION("Try to unload tile when it's still loading") {
      // unload tile to move from Done -> Unload
      manager.unloadTileContent(tile);
      CHECK(manager.getNumOfTilesLoading() == 1);
      CHECK(tile.getState() == TileLoadState::ContentLoading);
      CHECK(!tile.getContent().isRenderContent());
      CHECK(!tile.getContent().isExternalContent());
      CHECK(!tile.getContent().isEmptyContent());
      CHECK(!tile.getContent().getRenderResources());
      CHECK(!tile.getContent().getTileInitializerCallback());

      manager.waitIdle();
      CHECK(manager.getNumOfTilesLoading() == 0);
      CHECK(tile.getState() == TileLoadState::ContentLoaded);
      CHECK(tile.getContent().isRenderContent());
      CHECK(tile.getContent().getRenderResources());
      CHECK(tile.getContent().getTileInitializerCallback());

      manager.unloadTileContent(tile);
      CHECK(manager.getNumOfTilesLoading() == 0);
      CHECK(tile.getState() == TileLoadState::Unloaded);
      CHECK(!tile.getContent().isRenderContent());
      CHECK(!tile.getContent().getRenderResources());
      CHECK(!tile.getContent().getTileInitializerCallback());
    }
  }

  SECTION("Loader requests retry later") {
    // create mock loader
    bool initializerCall = false;
    auto pMockedLoader = std::make_unique<SimpleTilesetContentLoader>();
    pMockedLoader->mockLoadTileContent = {
        TileRenderContent{CesiumGltf::Model()},
        std::nullopt,
        std::nullopt,
        std::nullopt,
        nullptr,
        [&](Tile&) { initializerCall = true; },
        TileLoadResultState::RetryLater};
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

    // Unloaded -> ContentLoading
    CHECK(manager.getNumOfTilesLoading() == 1);
    CHECK(tile.getState() == TileLoadState::ContentLoading);
    CHECK(tile.getChildren().empty());
    CHECK(!tile.getContent().isRenderContent());
    CHECK(!tile.getContent().getTileInitializerCallback());
    CHECK(!tile.getContent().getRenderResources());

    // ContentLoading -> FailedTemporarily
    manager.waitIdle();
    CHECK(manager.getNumOfTilesLoading() == 0);
    CHECK(tile.getChildren().empty());
    CHECK(tile.getState() == TileLoadState::FailedTemporarily);
    CHECK(!tile.getContent().isRenderContent());
    CHECK(!tile.getContent().getTileInitializerCallback());
    CHECK(!tile.getContent().getRenderResources());
    CHECK(!initializerCall);

    // FailedTemporarily -> FailedTemporarily
    // tile is failed temporarily but the loader can still add children to it
    manager.updateTileContent(tile, options);
    CHECK(manager.getNumOfTilesLoading() == 0);
    CHECK(tile.getChildren().size() == 1);
    CHECK(tile.getChildren().front().isEmptyContent());
    CHECK(tile.getState() == TileLoadState::FailedTemporarily);
    CHECK(!tile.getContent().isRenderContent());
    CHECK(!tile.getContent().getTileInitializerCallback());
    CHECK(!tile.getContent().getRenderResources());
    CHECK(!initializerCall);

    // FailedTemporarily -> ContentLoading
    manager.loadTileContent(tile, options);
    CHECK(manager.getNumOfTilesLoading() == 1);
    CHECK(tile.getState() == TileLoadState::ContentLoading);
  }

  SECTION("Loader requests failed") {
    // create mock loader
    bool initializerCall = false;
    auto pMockedLoader = std::make_unique<SimpleTilesetContentLoader>();
    pMockedLoader->mockLoadTileContent = {
        TileRenderContent{CesiumGltf::Model()},
        std::nullopt,
        std::nullopt,
        std::nullopt,
        nullptr,
        [&](Tile&) { initializerCall = true; },
        TileLoadResultState::Failed};
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

    // Unloaded -> ContentLoading
    CHECK(manager.getNumOfTilesLoading() == 1);
    CHECK(tile.getState() == TileLoadState::ContentLoading);
    CHECK(tile.getChildren().empty());
    CHECK(!tile.getContent().isRenderContent());
    CHECK(!tile.getContent().getTileInitializerCallback());
    CHECK(!tile.getContent().getRenderResources());

    // ContentLoading -> Failed
    manager.waitIdle();
    CHECK(manager.getNumOfTilesLoading() == 0);
    CHECK(tile.getChildren().empty());
    CHECK(tile.getState() == TileLoadState::Failed);
    CHECK(!tile.getContent().isRenderContent());
    CHECK(!tile.getContent().getTileInitializerCallback());
    CHECK(!tile.getContent().getRenderResources());
    CHECK(!initializerCall);

    // Failed -> Failed
    // tile is failed but the loader can still add children to it
    manager.updateTileContent(tile, options);
    CHECK(manager.getNumOfTilesLoading() == 0);
    CHECK(tile.getChildren().size() == 1);
    CHECK(tile.getChildren().front().isEmptyContent());
    CHECK(tile.getState() == TileLoadState::Failed);
    CHECK(!tile.getContent().isRenderContent());
    CHECK(!tile.getContent().getTileInitializerCallback());
    CHECK(!tile.getContent().getRenderResources());
    CHECK(!initializerCall);

    // cannot transition from Failed -> ContentLoading
    manager.loadTileContent(tile, options);
    CHECK(manager.getNumOfTilesLoading() == 0);
    CHECK(tile.getState() == TileLoadState::Failed);
    CHECK(!tile.getContent().isRenderContent());
    CHECK(!tile.getContent().isExternalContent());
    CHECK(!tile.getContent().isEmptyContent());
    CHECK(!tile.getContent().getTileInitializerCallback());
    CHECK(!tile.getContent().getRenderResources());

    // Failed -> Unloaded
    manager.unloadTileContent(tile);
    CHECK(tile.getState() == TileLoadState::Unloaded);
    CHECK(!tile.getContent().isRenderContent());
    CHECK(!tile.getContent().isExternalContent());
    CHECK(!tile.getContent().isEmptyContent());
    CHECK(!tile.getContent().getTileInitializerCallback());
    CHECK(!tile.getContent().getRenderResources());
  }

  SECTION("Make sure the manager loads parent first before loading upsampled "
          "child") {
    // create mock loader
    bool initializerCall = false;
    auto pMockedLoader = std::make_unique<SimpleTilesetContentLoader>();
    auto pMockedLoaderRaw = pMockedLoader.get();

    pMockedLoader->mockLoadTileContent = {
        TileRenderContent{CesiumGltf::Model()},
        std::nullopt,
        std::nullopt,
        std::nullopt,
        nullptr,
        [&](Tile&) { initializerCall = true; },
        TileLoadResultState::Success};
    pMockedLoader->mockCreateTileChildren = {{}, TileLoadResultState::Failed};

    // create manager
    TilesetContentManager manager{
        externals,
        {},
        std::move(pMockedLoader),
        rasterOverlayCollection};

    // create tile
    Tile tile(pMockedLoaderRaw);
    tile.setTileID(QuadtreeTileID(0, 0, 0));

    // create upsampled children. We put it in the scope since upsampledTile
    // will be moved to the parent afterward, and we don't let the tests below
    // to use it accidentally
    {
      std::vector<Tile> children;
      children.emplace_back(pMockedLoaderRaw);
      Tile& upsampledTile = children.back();
      upsampledTile.setTileID(UpsampledQuadtreeNode{QuadtreeTileID(1, 1, 1)});
      tile.createChildTiles(std::move(children));
    }

    Tile& upsampledTile = tile.getChildren().back();

    // test manager loading upsample tile
    TilesetOptions options{};
    options.contentOptions.generateMissingNormalsSmooth = true;
    manager.loadTileContent(upsampledTile, options);

    // since parent is not yet loaded, it will load the parent first.
    // The upsampled tile will not be loaded at the moment
    CHECK(upsampledTile.getState() == TileLoadState::Unloaded);
    CHECK(tile.getState() == TileLoadState::ContentLoading);

    // parent moves from ContentLoading -> ContentLoaded
    manager.waitIdle();
    CHECK(tile.getState() == TileLoadState::ContentLoaded);
    CHECK(tile.isRenderContent());
    CHECK(tile.getContent().getTileInitializerCallback());
    CHECK(!initializerCall);

    // try again with upsample tile, but still not able to load it
    // because parent is not done yet
    manager.loadTileContent(upsampledTile, options);
    CHECK(upsampledTile.getState() == TileLoadState::Unloaded);

    // parent moves from ContentLoaded -> Done
    manager.updateTileContent(tile, options);
    CHECK(tile.getState() == TileLoadState::Done);
    CHECK(tile.getChildren().size() == 1);
    CHECK(&tile.getChildren().back() == &upsampledTile);
    CHECK(tile.isRenderContent());
    CHECK(!tile.getContent().getTileInitializerCallback());
    CHECK(initializerCall);

    // load the upsampled tile again: Unloaded -> ContentLoading
    initializerCall = false;
    pMockedLoaderRaw->mockLoadTileContent = {
        TileRenderContent{CesiumGltf::Model()},
        std::nullopt,
        std::nullopt,
        std::nullopt,
        nullptr,
        [&](Tile&) { initializerCall = true; },
        TileLoadResultState::Success};
    pMockedLoaderRaw->mockCreateTileChildren = {
        {},
        TileLoadResultState::Failed};
    manager.loadTileContent(upsampledTile, options);
    CHECK(upsampledTile.getState() == TileLoadState::ContentLoading);

    // trying to unload parent while upsampled children is loading won't work
    CHECK(!manager.unloadTileContent(tile));
    CHECK(tile.getState() == TileLoadState::Done);
    CHECK(tile.isRenderContent());

    // upsampled tile: ContentLoading -> ContentLoaded
    manager.waitIdle();
    CHECK(upsampledTile.getState() == TileLoadState::ContentLoaded);
    CHECK(upsampledTile.isRenderContent());
    CHECK(upsampledTile.getContent().getTileInitializerCallback());

    // trying to unload parent will work now since the upsampled tile is already
    // in the main thread
    CHECK(manager.unloadTileContent(tile));
    CHECK(tile.getState() == TileLoadState::Unloaded);
    CHECK(!tile.isRenderContent());
    CHECK(!tile.getContent().getRenderResources());

    // unload upsampled tile: ContentLoaded -> Done
    CHECK(manager.unloadTileContent(upsampledTile));
    CHECK(upsampledTile.getState() == TileLoadState::Unloaded);
    CHECK(!upsampledTile.isRenderContent());
    CHECK(!upsampledTile.getContent().getRenderResources());
  }
}

TEST_CASE("Test the tileset content manager's post processing for gltf") {
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

  SECTION("Resolve external buffers") {
    // create mock loader
    CesiumGltfReader::GltfReader gltfReader;
    std::vector<std::byte> gltfBoxFile =
        readFile(testDataPath / "gltf" / "box" / "Box.gltf");
    auto modelReadResult = gltfReader.readGltf(gltfBoxFile);

    // check that this model has external buffer and it's not loaded
    {
      CHECK(modelReadResult.errors.empty());
      CHECK(modelReadResult.warnings.empty());
      const auto& buffers = modelReadResult.model->buffers;
      CHECK(buffers.size() == 1);

      const auto& buffer = buffers.front();
      CHECK(buffer.uri == "Box0.bin");
      CHECK(buffer.byteLength == 648);
      CHECK(buffer.cesium.data.size() == 0);
    }

    auto pMockedLoader = std::make_unique<SimpleTilesetContentLoader>();
    pMockedLoader->mockLoadTileContent = {
        TileRenderContent{std::move(modelReadResult.model)},
        std::nullopt,
        std::nullopt,
        std::nullopt,
        nullptr,
        {},
        TileLoadResultState::Success};
    pMockedLoader->mockCreateTileChildren = {{}, TileLoadResultState::Failed};

    // add external buffer to the completed request
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"Box0.bin",
         createMockRequest(testDataPath / "gltf" / "box" / "Box0.bin")});

    // create manager
    TilesetContentManager manager{
        externals,
        {},
        std::move(pMockedLoader),
        rasterOverlayCollection};

    // test the gltf model
    Tile tile(pMockedLoader.get());
    manager.loadTileContent(tile, {});
    manager.waitIdle();

    // check the buffer is already loaded
    {
      CHECK(tile.getState() == TileLoadState::ContentLoaded);
      CHECK(tile.isRenderContent());

      const auto& renderContent = tile.getContent().getRenderContent();
      CHECK(renderContent->model);

      const auto& buffers = renderContent->model->buffers;
      CHECK(buffers.size() == 1);

      const auto& buffer = buffers.front();
      CHECK(buffer.uri == std::nullopt);
      CHECK(buffer.cesium.data.size() == 648);
    }

    // unload the tile content
    manager.unloadTileContent(tile);
  }

  SECTION("Ensure the loader generate smooth normal when the mesh doesn't have "
          "normal") {}

  SECTION("Generate raster overlay projections") {}
}
