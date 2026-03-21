#include "MockTilesetContentManager.h"
#include "TilesetContentManager.h"

#include <Cesium3DTilesSelection/EllipsoidTilesetLoader.h>
#include <Cesium3DTilesSelection/GltfModifier.h>
#include <Cesium3DTilesSelection/GltfModifierVersionExtension.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/TileLoadRequester.h>
#include <CesiumGeometry/BoundingSphere.h>
#include <CesiumGltf/Model.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumUtility/CreditSystem.h>

#include <doctest/doctest.h>

#include <memory>

using namespace Cesium3DTilesSelection;
using namespace CesiumAsync;
using namespace CesiumNativeTests;
using namespace CesiumUtility;

namespace Cesium3DTilesSelection {

class MockTilesetContentManagerForGltfModifier {
public:
  static TileLoadRequester* getTileLoadRequester(GltfModifier& modifier) {
    return &modifier;
  }

  static const TileLoadRequester*
  getTileLoadRequester(const GltfModifier& modifier) {
    return &modifier;
  }

  static void
  onOldVersionContentLoadingComplete(GltfModifier& modifier, const Tile& tile) {
    modifier.onOldVersionContentLoadingComplete(tile);
  }

  static void
  onUnregister(GltfModifier& modifier, TilesetContentManager& contentManager) {
    return modifier.onUnregister(contentManager);
  }
};

} // namespace Cesium3DTilesSelection

namespace {

class MockGltfModifier : public GltfModifier {
public:
  CesiumAsync::Future<std::optional<GltfModifierOutput>>
  apply(GltfModifierInput&& input) override {
    ++applyCallCount;
    return input.asyncSystem
        .createResolvedFuture<std::optional<GltfModifierOutput>>(std::nullopt);
  }

  int32_t applyCallCount = 0;
};

} // namespace

TEST_CASE("GltfModifier") {
  TilesetExternals externals{
      .pAssetAccessor = std::make_shared<SimpleAssetAccessor>(
          std::map<std::string, std::shared_ptr<SimpleAssetRequest>>()),
      .pPrepareRendererResources = nullptr,
      .asyncSystem = AsyncSystem(std::make_shared<SimpleTaskProcessor>()),
      .pCreditSystem = std::make_shared<CreditSystem>(),
      .pGltfModifier = std::make_shared<MockGltfModifier>()};

  std::unique_ptr<Tileset> pTileset =
      EllipsoidTilesetLoader::createTileset(externals, TilesetOptions{});

  pTileset->getRootTileAvailableEvent().waitInMainThread();

  Tile* pTile = const_cast<Tile*>(pTileset->getRootTile());
  std::shared_ptr<GltfModifier> pModifier =
      pTileset->getExternals().pGltfModifier;

  MockTilesetContentManagerTestFixture::setTileLoadState(
      *pTile,
      TileLoadState::ContentLoaded);

  TileContent content;
  content.setContentKind(
      std::make_unique<TileRenderContent>(CesiumGltf::Model{}));
  MockTilesetContentManagerTestFixture::setTileContent(
      *pTile,
      std::move(content));

  pTile->addReference();

  SUBCASE("has empty load queues on construction") {
    const TileLoadRequester* pRequester =
        MockTilesetContentManagerForGltfModifier::getTileLoadRequester(
            *pModifier);
    CHECK_FALSE(pRequester->hasMoreTilesToLoadInWorkerThread());
    CHECK_FALSE(pRequester->hasMoreTilesToLoadInMainThread());
  }

  SUBCASE("queues tiles for worker thread loading after trigger") {
    TileLoadRequester* pRequester =
        MockTilesetContentManagerForGltfModifier::getTileLoadRequester(
            *pModifier);

    // Initially inactive, no queuing should happen
    MockTilesetContentManagerForGltfModifier::
        onOldVersionContentLoadingComplete(*pModifier, *pTile);
    CHECK_FALSE(pRequester->hasMoreTilesToLoadInWorkerThread());

    // After trigger, tile should be queued
    pModifier->trigger();
    CHECK(pRequester->hasMoreTilesToLoadInWorkerThread());

    // Get next tile should return our tile and empty the queue
    const Tile* pNext = pRequester->getNextTileToLoadInWorkerThread();
    CHECK(pNext == pTile);
    CHECK_FALSE(pRequester->hasMoreTilesToLoadInWorkerThread());

    SUBCASE("multiple tiles") {
      REQUIRE(!pTile->getChildren().empty());
      Tile* pTile2 = &pTile->getChildren()[0];

      // Make the second tile loaded, too.
      MockTilesetContentManagerTestFixture::setTileLoadState(
          *pTile2,
          TileLoadState::ContentLoaded);

      TileContent content2;
      content2.setContentKind(
          std::make_unique<TileRenderContent>(CesiumGltf::Model{}));
      MockTilesetContentManagerTestFixture::setTileContent(
          *pTile2,
          std::move(content2));

      // Add a reference for the content, and for a child with a reference.
      pTile2->addReference();
      pTile->addReference();

      pModifier->trigger();

      CHECK(pRequester->hasMoreTilesToLoadInWorkerThread());
      const Tile* pNext1 = pRequester->getNextTileToLoadInWorkerThread();
      CHECK(pRequester->hasMoreTilesToLoadInWorkerThread());
      const Tile* pNext2 = pRequester->getNextTileToLoadInWorkerThread();
      CHECK_FALSE(pRequester->hasMoreTilesToLoadInWorkerThread());

      bool bothTilesReturned = (pNext1 == pTile && pNext2 == pTile2) ||
                               (pNext1 == pTile2 && pNext2 == pTile);
      CHECK(bothTilesReturned);
    }
  }

  SUBCASE("clears load queues on unregister") {
    const TileLoadRequester* pRequester =
        MockTilesetContentManagerForGltfModifier::getTileLoadRequester(
            *pModifier);

    pModifier->trigger();
    CHECK(pRequester->hasMoreTilesToLoadInWorkerThread());

    pTileset.reset();

    CHECK_FALSE(pRequester->hasMoreTilesToLoadInWorkerThread());
    CHECK_FALSE(pRequester->hasMoreTilesToLoadInMainThread());
  }

  SUBCASE("trigger causes modifier to be reapplied and version number to be "
          "updated") {
    const TileRenderContent* pRenderContent =
        pTile->getContent().getRenderContent();
    REQUIRE(pRenderContent);

    CHECK(
        GltfModifierVersionExtension::getVersion(pRenderContent->getModel()) ==
        std::nullopt);

    pModifier->trigger();
    REQUIRE(pModifier->getCurrentVersion() == 0);

    while (GltfModifierVersionExtension::getVersion(
               pRenderContent->getModel()) != 0) {
      pTileset->loadTiles();
      externals.asyncSystem.dispatchMainThreadTasks();
    }
  }
}
