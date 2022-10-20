
#include "SimpleAssetAccessor.h"
#include "SimpleAssetRequest.h"
#include "SimpleAssetResponse.h"
#include "SimplePrepareRendererResource.h"
#include "SimpleTaskProcessor.h"
#include "TilesetContentManager.h"

#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileContentCache.h>
#include <Cesium3DTilesSelection/TileLoadResult.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/registerAllTileContentTypes.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumUtility/IntrusivePointer.h>

#include <catch2/catch.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <variant>

using namespace Cesium3DTilesSelection;
using namespace CesiumAsync;

namespace {
class MockTileLoader : public TilesetContentLoader {
public:
  bool loadTileContentCalled = false;
  std::vector<Tile> mockChildren{};

  virtual Future<TileLoadResult>
  loadTileContent(const TileLoadInput& loadInput) override {
    return loadInput.pTileContentCache->getOrLoad(
        loadInput.asyncSystem,
        *std::get_if<std::string>(&loadInput.tile.getTileID()),
        loadInput.requestHeaders,
        [&loadTileContentCalled = this->loadTileContentCalled](
            std::shared_ptr<IAssetRequest>&& pCompletedRequest) mutable
        -> TileLoadResult {
          loadTileContentCalled = true;
          TileLoadResult result;
          result.contentKind = CesiumGltf::Model();
          result.pCompletedRequest = std::move(pCompletedRequest);
          result.state = TileLoadResultState::Success;
          return result;
        });
  }

  virtual TileChildrenResult createTileChildren(const Tile& /*tile*/) override {
    TileChildrenResult result{
        std::move(this->mockChildren),
        TileLoadResultState::Success};
    this->mockChildren.clear();

    return result;
  }
};

std::shared_ptr<SimpleAssetRequest> createMockRequest(
    const std::string& url,
    const std::vector<std::byte>& responseData) {
  auto pMockCompletedResponse = std::make_unique<SimpleAssetResponse>(
      static_cast<uint16_t>(200),
      "doesn't matter",
      CesiumAsync::HttpHeaders{},
      responseData);

  return std::make_shared<SimpleAssetRequest>(
      "GET",
      url,
      CesiumAsync::HttpHeaders{},
      std::move(pMockCompletedResponse));
}

void addMockRequest(
    std::map<std::string, std::shared_ptr<SimpleAssetRequest>>& mockedRequests,
    const std::string& url,
    const std::vector<std::byte>& responseData) {
  mockedRequests.emplace(url, createMockRequest(url, responseData));
}

std::vector<std::byte> int_to_buffer(uint32_t i) {
  return {
      static_cast<std::byte>(i >> 24),
      static_cast<std::byte>(255 & (i >> 16)),
      static_cast<std::byte>(255 & (i >> 8)),
      static_cast<std::byte>(255 & i)};
}
} // namespace

TEST_CASE("Test tile content cache") {
  Cesium3DTilesSelection::registerAllTileContentTypes();

  // Mock HTTP requests.
  std::map<std::string, std::shared_ptr<SimpleAssetRequest>> mockedRequests;
  std::vector<std::byte> a_data = int_to_buffer(0x00112233);
  addMockRequest(mockedRequests, "a.com", a_data);

  auto pMockedAssetAccessor =
      std::make_shared<SimpleAssetAccessor>(std::move(mockedRequests));

  // The cache starts empty.
  auto pMockedCacheAssetAccessor = std::make_shared<SimpleCachedAssetAccessor>(
      pMockedAssetAccessor,
      std::map<std::string, std::shared_ptr<IAssetRequest>>{});
  CesiumAsync::AsyncSystem asyncSystem{std::make_shared<SimpleTaskProcessor>()};

  auto pMockedPrepareRendererResources =
      std::make_shared<SimplePrepareRendererResource>();
  auto pMockedCreditSystem = std::make_shared<CreditSystem>();

  TilesetExternals externals{
      pMockedCacheAssetAccessor,
      pMockedPrepareRendererResources,
      asyncSystem,
      pMockedCreditSystem};

  // Create mock loader
  auto pMockedLoader_ = std::make_unique<MockTileLoader>();
  MockTileLoader* pMockedLoader = pMockedLoader_.get();

  // create root tile
  auto tileA = std::make_unique<Tile>(pMockedLoader);
  tileA->setTileID("a.com");

  // create manager
  TilesetOptions options{};
  Tile::LoadedLinkedList loadedTiles;
  CesiumUtility::IntrusivePointer<TilesetContentManager> pManager =
      new TilesetContentManager{
          externals,
          options,
          RasterOverlayCollection{loadedTiles, externals},
          {},
          std::move(pMockedLoader_),
          std::move(tileA)};

  Tile& tile = *pManager->getRootTile();

  SECTION("Tile cache miss causes load") {
    // Load tile.
    pManager->loadTileContent(tile, options);
    pManager->waitUntilIdle();
    pManager->updateTileContent(tile, 0.0, options);

    // A cache miss means the tile loader needs to process the response.
    REQUIRE(pMockedLoader->loadTileContentCalled);
    pMockedLoader->loadTileContentCalled = false;
  }

  SECTION("Loading, unloading, and reloading causes cache hit.") {
    // Mock the client creating derived data from the loading.
    auto clientData = int_to_buffer(0xaabbccdd);
    pMockedPrepareRendererResources->mockClientData = clientData;
    pMockedPrepareRendererResources->mockShouldCacheResponseData = true;

    // Load tile.
    pManager->loadTileContent(tile, options);
    pManager->waitUntilIdle();
    pManager->updateTileContent(tile, 0.0, options);

    REQUIRE(pMockedPrepareRendererResources->totalAllocation == 1);

    // Cache miss causes load.
    REQUIRE(pMockedLoader->loadTileContentCalled);
    pMockedLoader->loadTileContentCalled = false;

    // Unload the tile.
    pManager->unloadTileContent(tile);

    REQUIRE(pMockedPrepareRendererResources->totalAllocation == 0);

    // Tile should now be in cache.
    REQUIRE(pMockedCacheAssetAccessor->mockCache.size() == 1);
    REQUIRE(
        pMockedCacheAssetAccessor->mockCache.find("a.com") !=
        pMockedCacheAssetAccessor->mockCache.end());

    pMockedPrepareRendererResources->mockClientData.clear();
    pMockedPrepareRendererResources->mockShouldCacheResponseData = true;

    // Load the tile again.
    pManager->loadTileContent(tile, options);
    pManager->waitUntilIdle();
    pManager->updateTileContent(tile, 0.0, options);

    // Since there was derived data in the cache, loading should have been
    // skipped.
    REQUIRE(pMockedLoader->loadTileContentCalled == false);

    // TODO: Should ClientTileLoadResult, particularly the client-written buffer
    // be kept available somewhere after loading completes? Would be useful to
    // inspect.
  }
}

// TODO: Remove this before merging, this is just testing a mocked structure!
TEST_CASE("Test Mocked SimpleCachedAssetAccessor") {
  CesiumAsync::AsyncSystem asyncSystem{std::make_shared<SimpleTaskProcessor>()};

  // Mock HTTP requests.
  std::map<std::string, std::shared_ptr<SimpleAssetRequest>> mockedRequests;
  std::vector<std::byte> a = int_to_buffer(0x00112233);
  addMockRequest(mockedRequests, "a.com", a);
  std::vector<std::byte> b = int_to_buffer(0x44556677);
  addMockRequest(mockedRequests, "b.com", b);
  std::vector<std::byte> c = int_to_buffer(0x8899aabb);
  addMockRequest(mockedRequests, "c.com", c);

  // Start with empty cache
  SimpleCachedAssetAccessor cachedAssetAccessor(
      std::make_shared<SimpleAssetAccessor>(std::move(mockedRequests)),
      {});

  SECTION("Test write-through") {
    auto pRequestA =
        cachedAssetAccessor.get(asyncSystem, "a.com", {}, true).wait();
    auto pRequestB =
        cachedAssetAccessor.get(asyncSystem, "b.com", {}, true).wait();

    std::vector<std::byte> a_responseData(
        pRequestA->response()->data().begin(),
        pRequestA->response()->data().end());
    std::vector<std::byte> b_responseData(
        pRequestB->response()->data().begin(),
        pRequestB->response()->data().end());

    // Check the correct responses were retrieved.
    REQUIRE(a_responseData == a);
    REQUIRE(b_responseData == b);

    // Check that both requests were cached.
    REQUIRE(cachedAssetAccessor.mockCache.size() == 2);
    REQUIRE(
        cachedAssetAccessor.mockCache.find("a.com") !=
        cachedAssetAccessor.mockCache.end());
    REQUIRE(
        cachedAssetAccessor.mockCache.find("b.com") !=
        cachedAssetAccessor.mockCache.end());

    // Break underlying asset accessor, check that responses still
    // load from cache.
    cachedAssetAccessor.underlyingAssetAccessor->mockCompletedRequests.clear();

    pRequestA = cachedAssetAccessor.get(asyncSystem, "a.com", {}, true).wait();
    pRequestB = cachedAssetAccessor.get(asyncSystem, "b.com", {}, true).wait();

    a_responseData = std::vector<std::byte>(
        pRequestA->response()->data().begin(),
        pRequestA->response()->data().end());
    b_responseData = std::vector<std::byte>(
        pRequestB->response()->data().begin(),
        pRequestB->response()->data().end());

    // Check the correct responses were retrieved.
    REQUIRE(a_responseData == a);
    REQUIRE(b_responseData == b);
  }

  SECTION("Test write-back") {
    auto pRequestA =
        cachedAssetAccessor.get(asyncSystem, "a.com", {}, false).wait();
    auto pRequestB =
        cachedAssetAccessor.get(asyncSystem, "b.com", {}, false).wait();
    auto pRequestC =
        cachedAssetAccessor.get(asyncSystem, "c.com", {}, true).wait();

    std::vector<std::byte> a_responseData(
        pRequestA->response()->data().begin(),
        pRequestA->response()->data().end());
    std::vector<std::byte> b_responseData(
        pRequestB->response()->data().begin(),
        pRequestB->response()->data().end());
    std::vector<std::byte> c_responseData(
        pRequestC->response()->data().begin(),
        pRequestC->response()->data().end());

    // Check the correct responses were retrieved.
    REQUIRE(a_responseData == a);
    REQUIRE(b_responseData == b);
    REQUIRE(c_responseData == c);

    // Only c should be cached.
    REQUIRE(cachedAssetAccessor.mockCache.size() == 1);
    REQUIRE(
        cachedAssetAccessor.mockCache.find("c.com") !=
        cachedAssetAccessor.mockCache.end());

    // Write back custom client data for a and b.
    std::vector<std::byte> a_clientData = int_to_buffer(0xaaaaaaaa);
    std::vector<std::byte> b_clientData = int_to_buffer(0xbbbbbbbb);

    cachedAssetAccessor.writeBack(
        asyncSystem,
        pRequestA,
        false,
        std::vector<std::byte>(a_clientData));
    cachedAssetAccessor.writeBack(
        asyncSystem,
        pRequestB,
        true,
        std::vector<std::byte>(b_clientData));

    // All a, b, c should be in cache.
    REQUIRE(cachedAssetAccessor.mockCache.size() == 3);

    pRequestA = cachedAssetAccessor.get(asyncSystem, "a.com", {}, true).wait();
    pRequestB = cachedAssetAccessor.get(asyncSystem, "b.com", {}, true).wait();
    pRequestC = cachedAssetAccessor.get(asyncSystem, "c.com", {}, true).wait();

    a_responseData = std::vector<std::byte>(
        pRequestA->response()->data().begin(),
        pRequestA->response()->data().end());
    b_responseData = std::vector<std::byte>(
        pRequestB->response()->data().begin(),
        pRequestB->response()->data().end());
    c_responseData = std::vector<std::byte>(
        pRequestC->response()->data().begin(),
        pRequestC->response()->data().end());

    REQUIRE(a_responseData.empty());
    REQUIRE(b_responseData == b);
    REQUIRE(c_responseData == c);

    std::vector<std::byte> a_responseClientData(
        pRequestA->response()->clientData().begin(),
        pRequestA->response()->clientData().end());
    std::vector<std::byte> b_responseClientData(
        pRequestB->response()->clientData().begin(),
        pRequestB->response()->clientData().end());
    std::vector<std::byte> c_responseClientData(
        pRequestC->response()->clientData().begin(),
        pRequestC->response()->clientData().end());

    REQUIRE(a_responseClientData == a_clientData);
    REQUIRE(b_responseClientData == b_clientData);
    REQUIRE(c_responseClientData.empty());
  }
}