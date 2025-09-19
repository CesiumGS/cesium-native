#include "LayerJsonTerrainLoader.h"
#include "MockTilesetContentManager.h"
#include "SimplePrepareRendererResource.h"

#include <Cesium3DTilesContent/registerAllTileContentTypes.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileLoadResult.h>
#include <Cesium3DTilesSelection/TileRefine.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/TilesetOptions.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumGltf/Model.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumNativeTests/SimpleAssetResponse.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/readFile.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>
#include <spdlog/sinks/ringbuffer_sink.h>
#include <spdlog/spdlog.h>

#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace doctest;
using namespace Cesium3DTilesSelection;
using namespace CesiumGeospatial;
using namespace CesiumGeometry;
using namespace CesiumAsync;
using namespace CesiumUtility;
using namespace CesiumNativeTests;

namespace {
std::filesystem::path testDataPath = Cesium3DTilesSelection_TEST_DATA_DIR;

std::shared_ptr<SimpleAssetRequest>
createMockAssetRequest(const std::filesystem::path& requestContentPath) {
  std::unique_ptr<SimpleAssetResponse> pMockCompletedResponse;
  if (std::filesystem::exists(requestContentPath)) {
    pMockCompletedResponse = std::make_unique<SimpleAssetResponse>(
        static_cast<uint16_t>(200),
        "doesn't matter",
        CesiumAsync::HttpHeaders{},
        readFile(requestContentPath));
  } else {
    pMockCompletedResponse = std::make_unique<SimpleAssetResponse>(
        static_cast<uint16_t>(404),
        "doesn't matter",
        CesiumAsync::HttpHeaders{},
        std::vector<std::byte>{});
  }
  auto pMockCompletedRequest = std::make_shared<SimpleAssetRequest>(
      "GET",
      requestContentPath.filename().string(),
      CesiumAsync::HttpHeaders{},
      std::move(pMockCompletedResponse));
  return pMockCompletedRequest;
}

Future<TileLoadResult> loadTile(
    const QuadtreeTileID& tileID,
    LayerJsonTerrainLoader& loader,
    AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor) {
  Tile tile(&loader);
  tile.setTileID(tileID);
  tile.setBoundingVolume(BoundingRegionWithLooseFittingHeights{
      {GlobeRectangle(-Math::OnePi, -Math::PiOverTwo, 0.0, Math::PiOverTwo),
       -1000.0,
       9000.0,
       Ellipsoid::WGS84}});

  TileLoadInput loadInput{
      tile,
      {},
      asyncSystem,
      pAssetAccessor,
      spdlog::default_logger(),
      {}};

  auto tileLoadResultFuture = loader.loadTileContent(loadInput);

  asyncSystem.dispatchMainThreadTasks();

  return tileLoadResultFuture;
}
} // namespace

TEST_CASE("Test create layer json terrain loader") {
  Cesium3DTilesContent::registerAllTileContentTypes();

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

  SUBCASE("Create layer json loader") {
    auto layerJsonPath =
        testDataPath / "CesiumTerrainTileJson" / "QuantizedMesh.tile.json";
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"layer.json", createMockAssetRequest(layerJsonPath)});

    auto loaderFuture =
        LayerJsonTerrainLoader::createLoader(externals, {}, "layer.json", {});

    asyncSystem.dispatchMainThreadTasks();

    auto loaderResult = loaderFuture.wait();
    CHECK(loaderResult.pLoader);
    CHECK(loaderResult.pRootTile);

    // check tiling scheme
    const auto& tilingScheme = loaderResult.pLoader->getTilingScheme();
    CHECK(tilingScheme.getRootTilesX() == 2);
    CHECK(tilingScheme.getRootTilesY() == 1);

    // check projection
    const auto& projection = loaderResult.pLoader->getProjection();
    CHECK(std::holds_alternative<GeographicProjection>(projection));

    // check layer
    const auto& layers = loaderResult.pLoader->getLayers();
    CHECK(layers.size() == 1);
    CHECK(layers[0].version == "1.0.0");
    CHECK(layers[0].tileTemplateUrls.size() == 1);
    CHECK(layers[0].tileTemplateUrls[0] == "{z}/{x}/{y}.terrain?v={version}");
    CHECK(layers[0].availabilityLevels == -1);

    // check root tile
    const Tile& rootTile = *loaderResult.pRootTile;
    const auto& rootLooseRegion =
        std::get<BoundingRegionWithLooseFittingHeights>(
            rootTile.getBoundingVolume());
    const auto& rootRegion = rootLooseRegion.getBoundingRegion();
    CHECK(rootTile.isEmptyContent());
    CHECK(rootTile.getUnconditionallyRefine());
    CHECK(rootTile.getRefine() == TileRefine::Replace);
    CHECK(rootRegion.getRectangle().getWest() == Approx(-Math::OnePi));
    CHECK(rootRegion.getRectangle().getEast() == Approx(Math::OnePi));
    CHECK(rootRegion.getRectangle().getSouth() == Approx(-Math::PiOverTwo));
    CHECK(rootRegion.getRectangle().getNorth() == Approx(Math::PiOverTwo));
    CHECK(rootRegion.getMinimumHeight() == -1000.0);
    CHECK(rootRegion.getMaximumHeight() == 9000.0);

    // check children
    const auto& tileChildren = rootTile.getChildren();
    CHECK(tileChildren.size() == 2);

    const Tile& tile_0_0_0 = tileChildren[0];
    const auto& looseRegion_0_0_0 =
        std::get<BoundingRegionWithLooseFittingHeights>(
            tile_0_0_0.getBoundingVolume());
    const auto& region_0_0_0 = looseRegion_0_0_0.getBoundingRegion();
    CHECK(
        std::get<QuadtreeTileID>(tile_0_0_0.getTileID()) ==
        QuadtreeTileID(0, 0, 0));
    CHECK(tile_0_0_0.getGeometricError() == Approx(616538.71824));
    CHECK(region_0_0_0.getRectangle().getWest() == Approx(-Math::OnePi));
    CHECK(region_0_0_0.getRectangle().getEast() == Approx(0.0));
    CHECK(region_0_0_0.getRectangle().getSouth() == Approx(-Math::PiOverTwo));
    CHECK(region_0_0_0.getRectangle().getNorth() == Approx(Math::PiOverTwo));
    CHECK(region_0_0_0.getMinimumHeight() == -1000.0);
    CHECK(region_0_0_0.getMaximumHeight() == 9000.0);

    const Tile& tile_0_1_0 = tileChildren[1];
    const auto& looseRegion_0_1_0 =
        std::get<BoundingRegionWithLooseFittingHeights>(
            tile_0_1_0.getBoundingVolume());
    const auto& region_0_1_0 = looseRegion_0_1_0.getBoundingRegion();
    CHECK(
        std::get<QuadtreeTileID>(tile_0_1_0.getTileID()) ==
        QuadtreeTileID(0, 1, 0));
    CHECK(tile_0_1_0.getGeometricError() == Approx(616538.71824));
    CHECK(region_0_1_0.getRectangle().getWest() == Approx(0.0));
    CHECK(region_0_1_0.getRectangle().getEast() == Approx(Math::OnePi));
    CHECK(region_0_1_0.getRectangle().getSouth() == Approx(-Math::PiOverTwo));
    CHECK(region_0_1_0.getRectangle().getNorth() == Approx(Math::PiOverTwo));
    CHECK(region_0_1_0.getMinimumHeight() == -1000.0);
    CHECK(region_0_1_0.getMaximumHeight() == 9000.0);
  }

  SUBCASE("Load error layer json with empty tiles array") {
    auto layerJsonPath =
        testDataPath / "CesiumTerrainTileJson" / "EmptyTilesArray.tile.json";
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"layer.json", createMockAssetRequest(layerJsonPath)});

    auto loaderFuture =
        LayerJsonTerrainLoader::createLoader(externals, {}, "layer.json", {});

    asyncSystem.dispatchMainThreadTasks();

    auto loaderResult = loaderFuture.wait();
    CHECK(!loaderResult.pLoader);
    CHECK(!loaderResult.pRootTile);
    CHECK(loaderResult.errors.errors.size() == 1);
    CHECK(
        loaderResult.errors.errors[0] ==
        "Layer Json does not specify any tile URL templates");
  }

  SUBCASE("Load error layer json with no tiles field") {
    auto layerJsonPath =
        testDataPath / "CesiumTerrainTileJson" / "NoTiles.tile.json";
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"layer.json", createMockAssetRequest(layerJsonPath)});

    auto loaderFuture =
        LayerJsonTerrainLoader::createLoader(externals, {}, "layer.json", {});

    asyncSystem.dispatchMainThreadTasks();

    auto loaderResult = loaderFuture.wait();
    CHECK(!loaderResult.pLoader);
    CHECK(!loaderResult.pRootTile);
    CHECK(loaderResult.errors.errors.size() == 1);
    CHECK(
        loaderResult.errors.errors[0] ==
        "Layer Json does not specify any tile URL templates");
  }

  SUBCASE("Load layer json with metadataAvailability field") {
    auto layerJsonPath = testDataPath / "CesiumTerrainTileJson" /
                         "MetadataAvailability.tile.json";
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"layer.json", createMockAssetRequest(layerJsonPath)});

    auto loaderFuture =
        LayerJsonTerrainLoader::createLoader(externals, {}, "layer.json", {});

    asyncSystem.dispatchMainThreadTasks();

    auto loaderResult = loaderFuture.wait();
    CHECK(loaderResult.pLoader);
    CHECK(loaderResult.pRootTile);
    CHECK(!loaderResult.errors);

    CHECK(std::holds_alternative<GeographicProjection>(
        loaderResult.pLoader->getProjection()));

    const auto& layers = loaderResult.pLoader->getLayers();
    CHECK(layers.size() == 1);
    CHECK(layers[0].version == "1.33.0");
    CHECK(
        layers[0].tileTemplateUrls.front() ==
        "{z}/{x}/{y}.terrain?v={version}");
    CHECK(layers[0].extensionsToRequest == "octvertexnormals-metadata");
    CHECK(layers[0].loadedSubtrees.size() == 2);
    CHECK(layers[0].availabilityLevels == 10);
  }

  SUBCASE("Load layer json with OctVertexNormals extension") {
    auto layerJsonPath =
        testDataPath / "CesiumTerrainTileJson" / "OctVertexNormals.tile.json";
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"layer.json", createMockAssetRequest(layerJsonPath)});

    auto loaderFuture =
        LayerJsonTerrainLoader::createLoader(externals, {}, "layer.json", {});

    asyncSystem.dispatchMainThreadTasks();

    auto loaderResult = loaderFuture.wait();
    CHECK(loaderResult.pLoader);
    CHECK(loaderResult.pRootTile);
    CHECK(!loaderResult.errors);

    CHECK(std::holds_alternative<GeographicProjection>(
        loaderResult.pLoader->getProjection()));

    const auto& layers = loaderResult.pLoader->getLayers();
    CHECK(layers.size() == 1);
    CHECK(layers[0].version == "1.0.0");
    CHECK(
        layers[0].tileTemplateUrls.front() ==
        "{z}/{x}/{y}.terrain?v={version}");
    CHECK(layers[0].extensionsToRequest == "octvertexnormals");
    CHECK(layers[0].loadedSubtrees.empty());
    CHECK(layers[0].availabilityLevels == -1);

    CHECK(
        layers[0].contentAvailability.isTileAvailable(QuadtreeTileID(0, 0, 0)));
    CHECK(
        layers[0].contentAvailability.isTileAvailable(QuadtreeTileID(0, 1, 0)));
    CHECK(
        layers[0].contentAvailability.isTileAvailable(QuadtreeTileID(1, 1, 0)));
    CHECK(
        layers[0].contentAvailability.isTileAvailable(QuadtreeTileID(1, 3, 1)));
  }

  SUBCASE("Load multiple layers") {
    auto layerJsonPath =
        testDataPath / "CesiumTerrainTileJson" / "ParentUrl.tile.json";
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"layer.json", createMockAssetRequest(layerJsonPath)});

    auto parentJsonPath =
        testDataPath / "CesiumTerrainTileJson" / "Parent.tile.json";
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"Parent/layer.json", createMockAssetRequest(parentJsonPath)});

    auto loaderFuture =
        LayerJsonTerrainLoader::createLoader(externals, {}, "layer.json", {});

    asyncSystem.dispatchMainThreadTasks();

    auto loaderResult = loaderFuture.wait();
    CHECK(loaderResult.pLoader);
    CHECK(loaderResult.pRootTile);
    CHECK(!loaderResult.errors);

    const auto& layers = loaderResult.pLoader->getLayers();
    CHECK(layers.size() == 2);

    CHECK(layers[0].baseUrl == "ParentUrl.tile.json");
    CHECK(layers[0].version == "1.0.0");
    CHECK(layers[0].tileTemplateUrls.size() == 1);
    CHECK(
        layers[0].tileTemplateUrls.front() ==
        "{z}/{x}/{y}.terrain?v={version}");

    CHECK(layers[1].baseUrl == "Parent.tile.json");
    CHECK(layers[1].version == "1.1.0");
    CHECK(layers[1].tileTemplateUrls.size() == 1);
    CHECK(
        layers[1].tileTemplateUrls.front() ==
        "{z}/{x}/{y}.terrain?v={version}");
  }

  SUBCASE("Load layer json with partial availability") {
    auto layerJsonPath = testDataPath / "CesiumTerrainTileJson" /
                         "PartialAvailability.tile.json";
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"layer.json", createMockAssetRequest(layerJsonPath)});

    auto loaderFuture =
        LayerJsonTerrainLoader::createLoader(externals, {}, "layer.json", {});

    asyncSystem.dispatchMainThreadTasks();

    auto loaderResult = loaderFuture.wait();
    CHECK(loaderResult.pLoader);
    CHECK(loaderResult.pRootTile);

    const auto& layers = loaderResult.pLoader->getLayers();
    CHECK(layers.size() == 1);
    CHECK(
        layers[0].contentAvailability.isTileAvailable(QuadtreeTileID(2, 1, 0)));
    CHECK(!layers[0].contentAvailability.isTileAvailable(
        QuadtreeTileID(2, 0, 0)));
  }

  SUBCASE("Load layer json with attribution") {
    auto layerJsonPath =
        testDataPath / "CesiumTerrainTileJson" / "WithAttribution.tile.json";
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"layer.json", createMockAssetRequest(layerJsonPath)});

    auto loaderFuture =
        LayerJsonTerrainLoader::createLoader(externals, {}, "layer.json", {});

    asyncSystem.dispatchMainThreadTasks();

    auto loaderResult = loaderFuture.wait();
    CHECK(loaderResult.pLoader);
    CHECK(loaderResult.pRootTile);
    CHECK(loaderResult.credits.size() == 1);
    CHECK(
        loaderResult.credits.front().creditText ==
        "This amazing data is courtesy The Amazing Data Source!");
  }

  SUBCASE("Load layer json with watermask") {
    auto layerJsonPath =
        testDataPath / "CesiumTerrainTileJson" / "WaterMask.tile.json";
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"layer.json", createMockAssetRequest(layerJsonPath)});

    TilesetContentOptions options;
    options.enableWaterMask = true;
    auto loaderFuture = LayerJsonTerrainLoader::createLoader(
        externals,
        options,
        "layer.json",
        {});

    asyncSystem.dispatchMainThreadTasks();

    auto loaderResult = loaderFuture.wait();
    CHECK(loaderResult.pLoader);
    CHECK(loaderResult.pRootTile);

    const auto& layers = loaderResult.pLoader->getLayers();
    CHECK(layers.size() == 1);
    CHECK(layers[0].tileTemplateUrls.size() == 1);
    CHECK(layers[0].tileTemplateUrls[0] == "{z}/{x}/{y}.terrain?v={version}");
    CHECK(layers[0].extensionsToRequest == "octvertexnormals-watermask");
  }

  SUBCASE("Verify custom headers are passed to all HTTP requests") {
    // Create a custom asset accessor that tracks headers for each request
    class HeaderTrackingAssetAccessor : public CesiumAsync::IAssetAccessor {
    public:
      HeaderTrackingAssetAccessor(
          std::map<std::string, std::shared_ptr<SimpleAssetRequest>>&& mockRequests)
          : mockCompletedRequests{std::move(mockRequests)} {}

      CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
      get(const CesiumAsync::AsyncSystem& asyncSystem,
          const std::string& url,
          const std::vector<THeader>& headers) override {
        // Temporary logging for debugging
        spdlog::info("HTTP Request to: {}", url);
        spdlog::info("Headers count: {}", headers.size());
        for (const auto& header : headers) {
          spdlog::info("  {} = {}", header.first, header.second);
        }
        
        // Store the headers for this URL
        requestHeaders[url] = headers;
        
        // Return the mock request if available
        auto it = mockCompletedRequests.find(url);
        if (it != mockCompletedRequests.end()) {
          return asyncSystem.createResolvedFuture<std::shared_ptr<CesiumAsync::IAssetRequest>>(it->second);
        }
        
        // Return a 404 response if no mock is available
        auto pMock404Response = std::make_unique<SimpleAssetResponse>(
            static_cast<uint16_t>(404),
            "doesn't matter",
            CesiumAsync::HttpHeaders{},
            std::vector<std::byte>{});
        auto pMock404Request = std::make_shared<SimpleAssetRequest>(
            "GET",
            url,
            CesiumAsync::HttpHeaders{},
            std::move(pMock404Response));
        return asyncSystem.createResolvedFuture<std::shared_ptr<CesiumAsync::IAssetRequest>>(pMock404Request);
      }

      CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
      request(
          const CesiumAsync::AsyncSystem& asyncSystem,
          const std::string& /* verb */,
          const std::string& url,
          const std::vector<THeader>& headers,
          const std::span<const std::byte>&) override {
        return get(asyncSystem, url, headers);
      }

      void tick() noexcept override {}

      std::map<std::string, std::shared_ptr<SimpleAssetRequest>> mockCompletedRequests;
      std::map<std::string, std::vector<THeader>> requestHeaders;
    };

    // Setup mock files
    auto layerJsonPath =
        testDataPath / "CesiumTerrainTileJson" / "ParentUrl.tile.json";
    auto parentJsonPath =
        testDataPath / "CesiumTerrainTileJson" / "Parent.tile.json";
    auto tileContentPath =
        testDataPath / "CesiumTerrainTileJson" / "tile.terrain";

    auto pHeaderTrackingAccessor = std::make_shared<HeaderTrackingAssetAccessor>(
        std::map<std::string, std::shared_ptr<SimpleAssetRequest>>{
            {"layer.json", createMockAssetRequest(layerJsonPath)},
            {"Parent/layer.json", createMockAssetRequest(parentJsonPath)},
            {"0/0/0.terrain?v=1.0.0", createMockAssetRequest(tileContentPath)}
        });

    TilesetExternals customExternals{
        pHeaderTrackingAccessor,
        pMockedPrepareRendererResources,
        asyncSystem,
        pMockedCreditSystem};

    // Define custom headers
    std::vector<CesiumAsync::IAssetAccessor::THeader> customHeaders = {
        {"Authorization", "Bearer test-token-123"},
        {"X-Custom-Header", "custom-value"},
        {"User-Agent", "CesiumNative-Test/1.0"}
    };

    // **DEBUG POINT 2**: Set breakpoint here to inspect custom headers before loader creation
    auto loaderFuture = LayerJsonTerrainLoader::createLoader(
        customExternals,
        {},
        "layer.json",
        customHeaders);

    asyncSystem.dispatchMainThreadTasks();

    auto loaderResult = loaderFuture.wait();
    // **DEBUG POINT 3**: Set breakpoint here to inspect loader creation results
    CHECK(loaderResult.pLoader);
    CHECK(loaderResult.pRootTile);
    CHECK(!loaderResult.errors);

    // Verify that custom headers were passed to all requests
    CHECK(pHeaderTrackingAccessor->requestHeaders.size() >= 2);

    // **DEBUG POINT 4**: Set breakpoint here to inspect captured headers
    auto mainLayerHeaders = pHeaderTrackingAccessor->requestHeaders.find("layer.json");
    REQUIRE(mainLayerHeaders != pHeaderTrackingAccessor->requestHeaders.end());
    CHECK(mainLayerHeaders->second.size() == customHeaders.size());
    for (size_t i = 0; i < customHeaders.size(); ++i) {
      CHECK(mainLayerHeaders->second[i].first == customHeaders[i].first);
      CHECK(mainLayerHeaders->second[i].second == customHeaders[i].second);
    }

    // Check headers for parent layer.json request
    auto parentLayerHeaders = pHeaderTrackingAccessor->requestHeaders.find("Parent/layer.json");
    REQUIRE(parentLayerHeaders != pHeaderTrackingAccessor->requestHeaders.end());
    CHECK(parentLayerHeaders->second.size() == customHeaders.size());
    for (size_t i = 0; i < customHeaders.size(); ++i) {
      CHECK(parentLayerHeaders->second[i].first == customHeaders[i].first);
      CHECK(parentLayerHeaders->second[i].second == customHeaders[i].second);
    }

    // Now test tile content loading with custom headers
    // Create a tile and load it with custom headers
    Tile tile(loaderResult.pLoader.get());
    tile.setTileID(QuadtreeTileID(0, 0, 0));
    tile.setBoundingVolume(BoundingRegionWithLooseFittingHeights{
        {GlobeRectangle(-Math::OnePi, -Math::PiOverTwo, 0.0, Math::PiOverTwo),
         -1000.0,
         9000.0,
         Ellipsoid::WGS84}});

    // Create TileLoadInput with correct constructor parameters
    TileLoadInput loadInput(
        tile,
        {},  // TilesetContentOptions
        asyncSystem,
        pHeaderTrackingAccessor,
        spdlog::default_logger(),
        customHeaders,
        Ellipsoid::WGS84);

    auto tileLoadResultFuture = loaderResult.pLoader->loadTileContent(loadInput);
    asyncSystem.dispatchMainThreadTasks();
    auto tileLoadResult = tileLoadResultFuture.wait();

    // Verify tile content request used custom headers
    auto tileContentHeaders = pHeaderTrackingAccessor->requestHeaders.find("0/0/0.terrain?v=1.0.0");
    REQUIRE(tileContentHeaders != pHeaderTrackingAccessor->requestHeaders.end());
    CHECK(tileContentHeaders->second.size() == customHeaders.size());
    for (size_t i = 0; i < customHeaders.size(); ++i) {
      CHECK(tileContentHeaders->second[i].first == customHeaders[i].first);
      CHECK(tileContentHeaders->second[i].second == customHeaders[i].second);
    }
  }
}
