#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/readFile.h>
#include <CesiumNativeTests/waitForFuture.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumRasterOverlays/TileMapServiceRasterOverlay.h>

#include <catch2/catch.hpp>

#include <filesystem>

using namespace CesiumGltf;
using namespace CesiumNativeTests;
using namespace CesiumRasterOverlays;
using namespace CesiumUtility;

TEST_CASE("TileMapServiceRasterOverlay") {
  // Set up some mock resources for the raster overlay.
  std::filesystem::path dataDir(CesiumRasterOverlays_TEST_DATA_DIR);
  auto pMockTaskProcessor = std::make_shared<SimpleTaskProcessor>();
  CesiumAsync::AsyncSystem asyncSystem{pMockTaskProcessor};

  std::map<std::string, std::shared_ptr<SimpleAssetRequest>> mapUrlToRequest;
  for (const auto& entry : std::filesystem::recursive_directory_iterator(
           dataDir / "Cesium_Logo_Color")) {
    if (!entry.is_regular_file())
      continue;
    auto pResponse = std::make_unique<SimpleAssetResponse>(
        uint16_t(200),
        "application/binary",
        CesiumAsync::HttpHeaders{},
        readFile(entry.path()));
    std::string url = "file:///" + entry.path().generic_u8string();
    auto pRequest = std::make_unique<SimpleAssetRequest>(
        "GET",
        url,
        CesiumAsync::HttpHeaders{},
        std::move(pResponse));
    mapUrlToRequest[url] = std::move(pRequest);
  }

  auto pMockAssetAccessor =
      std::make_shared<SimpleAssetAccessor>(std::move(mapUrlToRequest));

  std::string tmr =
      "file:///" + std::filesystem::directory_entry(
                       dataDir / "Cesium_Logo_Color" / "tilemapresource.xml")
                       .path()
                       .generic_u8string();
  IntrusivePointer<TileMapServiceRasterOverlay> pRasterOverlay =
      new TileMapServiceRasterOverlay("test", tmr);

  SECTION("can load images") {
    RasterOverlay::CreateTileProviderResult result = waitForFuture(
        asyncSystem,
        pRasterOverlay->createTileProvider(
            asyncSystem,
            pMockAssetAccessor,
            nullptr,
            nullptr,
            spdlog::default_logger(),
            nullptr));

    REQUIRE(result);

    CesiumUtility::IntrusivePointer<RasterOverlayTileProvider> pTileProvider =
        *result;
    IntrusivePointer<RasterOverlayTile> pTile = pTileProvider->getTile(
        pTileProvider->getCoverageRectangle(),
        glm::dvec2(256.0, 256.0));
    REQUIRE(pTile);
    waitForFuture(asyncSystem, pTileProvider->loadTile(*pTile));

    ImageCesium& image = pTile->getImage();
    CHECK(image.width > 0);
    CHECK(image.height > 0);
  }

  SECTION("appends tilemapresource.xml to URL if not already present and "
          "direct request fails") {
    std::string url = "file:///" + std::filesystem::directory_entry(
                                       dataDir / "Cesium_Logo_Color")
                                       .path()
                                       .generic_u8string();
    pMockAssetAccessor->mockCompletedRequests[url] =
        std::make_shared<SimpleAssetRequest>(
            "GET",
            url,
            CesiumAsync::HttpHeaders(),
            std::make_unique<SimpleAssetResponse>(
                404,
                "",
                CesiumAsync::HttpHeaders(),
                std::vector<std::byte>()));
    pRasterOverlay = new TileMapServiceRasterOverlay("test", url);

    RasterOverlay::CreateTileProviderResult result = waitForFuture(
        asyncSystem,
        pRasterOverlay->createTileProvider(
            asyncSystem,
            pMockAssetAccessor,
            nullptr,
            nullptr,
            spdlog::default_logger(),
            nullptr));

    REQUIRE(result);
  }

  SECTION("does not add another slash when URL has query parameters") {
    // Add a request handler for `.../tilemapresource.xml?some=parameter` but
    // _not_ `.../tilemapresource.xml?some=parameter/`or
    // `.../tilemapresource.xml/?some=parameter`, in order to verify that the
    // TMS overlay class sees the tilemapresource.xml at the end of the URL and
    // is not confused by the query parameter.
    std::string xmlUrlWithParameter = tmr + "?some=parameter";
    std::shared_ptr<SimpleAssetRequest> pExistingRequest =
        pMockAssetAccessor->mockCompletedRequests[tmr];
    REQUIRE(pExistingRequest);
    pMockAssetAccessor->mockCompletedRequests[xmlUrlWithParameter] =
        std::make_unique<SimpleAssetRequest>(
            "GET",
            xmlUrlWithParameter,
            CesiumAsync::HttpHeaders{},
            std::make_unique<SimpleAssetResponse>(
                *static_cast<const SimpleAssetResponse*>(
                    pExistingRequest->response())));

    pRasterOverlay =
        new TileMapServiceRasterOverlay("test", xmlUrlWithParameter);

    RasterOverlay::CreateTileProviderResult result = waitForFuture(
        asyncSystem,
        pRasterOverlay->createTileProvider(
            asyncSystem,
            pMockAssetAccessor,
            nullptr,
            nullptr,
            spdlog::default_logger(),
            nullptr));

    REQUIRE(result);
  }

  SECTION("adds tilemapresource.xml in the correct place even with query "
          "parameters") {
    // The initial URL does not include tilemapresource.xml and will fail
    std::string url =
        "file:///" +
        std::filesystem::directory_entry(dataDir / "Cesium_Logo_Color")
            .path()
            .generic_u8string() +
        "?some=parameter";

    pMockAssetAccessor->mockCompletedRequests[url] =
        std::make_shared<SimpleAssetRequest>(
            "GET",
            url,
            CesiumAsync::HttpHeaders(),
            std::make_unique<SimpleAssetResponse>(
                404,
                "",
                CesiumAsync::HttpHeaders(),
                std::vector<std::byte>()));

    // Add a request handler for `.../tilemapresource.xml?some=parameter` but
    // _not_ `.../tilemapresource.xml?some=parameter/`or
    // `.../tilemapresource.xml/?some=parameter`, in order to verify that the
    // TMS overlay class sees the tilemapresource.xml at the end of the URL and
    // is not confused by the query parameter.
    std::string xmlUrlWithParameter = tmr + "?some=parameter";
    std::shared_ptr<SimpleAssetRequest> pExistingRequest =
        pMockAssetAccessor->mockCompletedRequests[tmr];
    REQUIRE(pExistingRequest);
    pMockAssetAccessor->mockCompletedRequests[xmlUrlWithParameter] =
        std::make_unique<SimpleAssetRequest>(
            "GET",
            xmlUrlWithParameter,
            CesiumAsync::HttpHeaders{},
            std::make_unique<SimpleAssetResponse>(
                *static_cast<const SimpleAssetResponse*>(
                    pExistingRequest->response())));

    pRasterOverlay = new TileMapServiceRasterOverlay("test", url);

    RasterOverlay::CreateTileProviderResult result = waitForFuture(
        asyncSystem,
        pRasterOverlay->createTileProvider(
            asyncSystem,
            pMockAssetAccessor,
            nullptr,
            nullptr,
            spdlog::default_logger(),
            nullptr));

    REQUIRE(result);
  }
}
