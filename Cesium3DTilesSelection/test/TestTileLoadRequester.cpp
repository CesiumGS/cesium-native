#include <Cesium3DTilesSelection/EllipsoidTilesetLoader.h>
#include <Cesium3DTilesSelection/TileLoadRequester.h>
#include <Cesium3DTilesSelection/Tileset.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>

#include <doctest/doctest.h>

using namespace Cesium3DTilesSelection;
using namespace CesiumAsync;
using namespace CesiumGeospatial;
using namespace CesiumNativeTests;
using namespace CesiumUtility;

namespace {

class TestTileLoadRequester : public TileLoadRequester {
public:
  // TileLoadRequester methods
  double getWeight() const override { return this->_weight; }

  bool hasMoreTilesToLoadInWorkerThread() const override {
    return !this->_workerThread.empty();
  }

  Tile* getNextTileToLoadInWorkerThread() override {
    CESIUM_ASSERT(!this->_workerThread.empty());
    Tile* pResult = this->_workerThread.back();
    this->_workerThread.pop_back();
    return pResult;
  }

  bool hasMoreTilesToLoadInMainThread() const override {
    return !this->_mainThread.empty();
  }

  Tile* getNextTileToLoadInMainThread() override {
    CESIUM_ASSERT(!this->_mainThread.empty());
    Tile* pResult = this->_mainThread.back();
    this->_mainThread.pop_back();
    return pResult;
  }

  // Extra methods for testing
  void setWeight(double weight) { this->_weight = weight; }
  void setWorkerThreadQueue(const std::vector<Tile*>& newQueue) {
    this->_keepAlive.insert(
        this->_keepAlive.end(),
        newQueue.begin(),
        newQueue.end());
    this->_workerThread = newQueue;
  }
  void setMainThreadQueue(const std::vector<Tile*>& newQueue) {
    this->_keepAlive.insert(
        this->_keepAlive.end(),
        newQueue.begin(),
        newQueue.end());
    this->_mainThread = newQueue;
  }

private:
  double _weight = 1.0;
  std::vector<Tile*> _workerThread;
  std::vector<Tile*> _mainThread;
  std::vector<Tile::Pointer> _keepAlive;
};

class TestCustomLoader : public TilesetContentLoader {
public:
  Future<TileLoadResult> loadTileContent(const TileLoadInput& input) override {
    return input.asyncSystem.runInMainThread([input]() {
      return TileLoadResult{
          .contentKind = TileEmptyContent(),
          .glTFUpAxis = CesiumGeometry::Axis::Z,
          .updatedBoundingVolume = std::nullopt,
          .updatedContentBoundingVolume = std::nullopt,
          .rasterOverlayDetails = std::nullopt,
          .pAssetAccessor = input.pAssetAccessor,
          .pCompletedRequest = nullptr,
          .tileInitializer = {},
          .state = TileLoadResultState::Success,
          .ellipsoid = CesiumGeospatial::Ellipsoid::WGS84};
    });
  }

  TileChildrenResult createTileChildren(
      const Tile& /* tile */,
      const Ellipsoid& /* ellipsoid */) override {
    return TileChildrenResult{{}, TileLoadResultState::Success};
  }
};

} // namespace

TEST_CASE("TileLoadRequester") {
  SUBCASE("with a real tileset") {
    TilesetExternals externals{
        .pAssetAccessor = std::make_shared<SimpleAssetAccessor>(
            std::map<std::string, std::shared_ptr<SimpleAssetRequest>>()),
        .pPrepareRendererResources = nullptr,
        .asyncSystem = AsyncSystem(std::make_shared<SimpleTaskProcessor>()),
        .pCreditSystem = std::make_shared<CreditSystem>(),
    };

    auto pTileset = EllipsoidTilesetLoader::createTileset(externals);

    Tile* pRoot = pTileset->getRootTile();
    REQUIRE(pRoot != nullptr);
    REQUIRE(pRoot->getChildren().size() == 2);
    REQUIRE(pRoot->getState() == TileLoadState::ContentLoaded);

    SUBCASE("triggers tile loads") {
      TestTileLoadRequester requester;
      pTileset->registerLoadRequester(requester);

      Tile* pToLoad = &pRoot->getChildren()[1];
      CHECK(pToLoad->getState() == TileLoadState::Unloaded);

      // loadTiles won't load the tile because nothing has requested it yet.
      pTileset->loadTiles();
      CHECK(pToLoad->getState() == TileLoadState::Unloaded);

      // Request this tile for worker thread loading and verify it happens.
      requester.setWorkerThreadQueue({pToLoad});
      pTileset->loadTiles();
      CHECK(pToLoad->getState() == TileLoadState::ContentLoading);

      // Wait until the tile finishes worker thread loading.
      while (pToLoad->getState() == TileLoadState::ContentLoading) {
        externals.asyncSystem.dispatchMainThreadTasks();
      }

      // Calling loadTiles again won't do main thread loading yet.
      CHECK(pToLoad->getState() == TileLoadState::ContentLoaded);
      pTileset->loadTiles();
      CHECK(pToLoad->getState() == TileLoadState::ContentLoaded);

      // Request this tile for main thread loading and verify it happens.
      requester.setMainThreadQueue({pToLoad});
      pTileset->loadTiles();
      CHECK(pToLoad->getState() == TileLoadState::Done);
    }
  }

  SUBCASE("with a simple flat tileset") {
    TilesetExternals externals{
        .pAssetAccessor = std::make_shared<SimpleAssetAccessor>(
            std::map<std::string, std::shared_ptr<SimpleAssetRequest>>()),
        .pPrepareRendererResources = nullptr,
        .asyncSystem = AsyncSystem(std::make_shared<SimpleTaskProcessor>()),
        .pCreditSystem = std::make_shared<CreditSystem>(),
    };

    auto pCustomLoader = std::make_unique<TestCustomLoader>();
    auto pRootTile = std::make_unique<Tile>(pCustomLoader.get());

    std::vector<Tile> children;
    children.reserve(100);

    for (size_t i = 0; i < 100; ++i) {
      children.emplace_back(pCustomLoader.get());
    }

    pRootTile->createChildTiles(std::move(children));

    TilesetOptions options;
    options.maximumSimultaneousTileLoads = 5;
    auto pTileset = std::make_unique<Tileset>(
        externals,
        std::move(pCustomLoader),
        std::move(pRootTile),
        options);

    Tile* pRoot = pTileset->getRootTile();
    REQUIRE(pRoot != nullptr);
    REQUIRE(pRoot->getChildren().size() == 100);

    SUBCASE("prioritizes different load requesters by weight") {
      TestTileLoadRequester reqNormal, reqExtra, reqVeryLow, reqVeryHigh;
      reqNormal.setWeight(1.0);
      reqExtra.setWeight(2.0);
      reqVeryLow.setWeight(1e-100);
      reqVeryHigh.setWeight(1e100);

      pTileset->registerLoadRequester(reqNormal);
      pTileset->registerLoadRequester(reqExtra);
      pTileset->registerLoadRequester(reqVeryLow);
      pTileset->registerLoadRequester(reqVeryHigh);

      std::vector<Tile*> pointers(pRoot->getChildren().size());
      std::transform(
          pRoot->getChildren().begin(),
          pRoot->getChildren().end(),
          pointers.begin(),
          [](Tile& tile) { return &tile; });

      reqNormal.setWorkerThreadQueue(
          std::vector<Tile*>(pointers.begin(), pointers.begin() + 20));
      reqExtra.setWorkerThreadQueue(
          std::vector<Tile*>(pointers.begin() + 20, pointers.begin() + 40));
      reqVeryLow.setWorkerThreadQueue(
          std::vector<Tile*>(pointers.begin() + 40, pointers.begin() + 60));
      reqVeryHigh.setWorkerThreadQueue(
          std::vector<Tile*>(pointers.begin() + 80, pointers.end()));

      std::vector<const TestTileLoadRequester*> requestersOutOfTiles;

      auto checkAndAdd = [&requestersOutOfTiles](
                             const TestTileLoadRequester& requester) mutable {
        if (!requester.hasMoreTilesToLoadInWorkerThread() &&
            std::find(
                requestersOutOfTiles.begin(),
                requestersOutOfTiles.end(),
                &requester) == requestersOutOfTiles.end()) {
          requestersOutOfTiles.emplace_back(&requester);
        }
      };

      while (requestersOutOfTiles.size() < 4) {
        pTileset->loadTiles();
        externals.asyncSystem.dispatchMainThreadTasks();

        checkAndAdd(reqNormal);
        checkAndAdd(reqExtra);
        checkAndAdd(reqVeryLow);
        checkAndAdd(reqVeryHigh);
      }

      // The requesters should finish their items in the order expected based on
      // their weight.
      CHECK(
          requestersOutOfTiles == std::vector<const TestTileLoadRequester*>{
                                      &reqVeryHigh,
                                      &reqExtra,
                                      &reqNormal,
                                      &reqVeryLow});
    }
  }
}
