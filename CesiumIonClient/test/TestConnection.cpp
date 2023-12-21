#include <CesiumAsync/AsyncSystem.h>
#include <CesiumIonClient/Connection.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/readFile.h>
#include <CesiumNativeTests/waitForFuture.h>

#include <catch2/catch.hpp>

using namespace CesiumAsync;
using namespace CesiumIonClient;
using namespace CesiumNativeTests;

TEST_CASE("CesiumIonClient::Connection") {
  std::shared_ptr<SimpleAssetAccessor> pAssetAccessor =
      std::make_shared<SimpleAssetAccessor>(
          std::map<std::string, std::shared_ptr<SimpleAssetRequest>>());

  std::unique_ptr<SimpleAssetResponse> pResponse =
      std::make_unique<SimpleAssetResponse>(
          static_cast<uint16_t>(200),
          "doesn't matter",
          CesiumAsync::HttpHeaders{},
          readFile(
              std::filesystem::path(CesiumIonClient_TEST_DATA_DIR) /
              "defaults.json"));

  std::shared_ptr<SimpleAssetRequest> pRequest =
      std::make_shared<SimpleAssetRequest>(
          "GET",
          "doesn't matter",
          CesiumAsync::HttpHeaders{},
          std::move(pResponse));

  pAssetAccessor->mockCompletedRequests.insert(
      {"https://example.com/v1/defaults", std::move(pRequest)});

  AsyncSystem asyncSystem(std::make_shared<SimpleTaskProcessor>());
  Connection connection(
      asyncSystem,
      pAssetAccessor,
      "my access token",
      "https://example.com/");

  Future<Response<Defaults>> futureDefaults = connection.defaults();
  Response<Defaults> defaults =
      waitForFuture(asyncSystem, std::move(futureDefaults));

  REQUIRE(defaults.value);

  CHECK(defaults.value->defaultAssets.imagery == 2);
  CHECK(defaults.value->defaultAssets.terrain == 1);
  CHECK(defaults.value->defaultAssets.buildings == 624);

  REQUIRE(defaults.value->quickAddAssets.size() == 9);
  const QuickAddAsset& cwtAndBing = defaults.value->quickAddAssets[6];
  CHECK(cwtAndBing.name == "Cesium World Terrain + Bing Maps Aerial");
  CHECK(cwtAndBing.objectName == "Cesium World Terrain");
  CHECK(
      cwtAndBing.description ==
      "High-resolution global terrain tileset curated from several data "
      "sources.  See the official [Cesium World "
      "Terrain](https://cesium.com/content/cesium-world-terrain/) page for "
      "details. textured with Aerial imagery.");
  CHECK(cwtAndBing.assetId == 1);
  CHECK(cwtAndBing.type == "TERRAIN");
  CHECK(cwtAndBing.subscribed == true);

  REQUIRE(cwtAndBing.rasterOverlays.size() == 1);
  const QuickAddRasterOverlay& bing = cwtAndBing.rasterOverlays[0];
  CHECK(bing.name == "Bing Maps Aerial");
  CHECK(bing.assetId == 2);
  CHECK(bing.subscribed == true);
}
