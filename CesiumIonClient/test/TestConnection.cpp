#include <CesiumAsync/AsyncSystem.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumIonClient/ApplicationData.h>
#include <CesiumIonClient/Connection.h>
#include <CesiumIonClient/Defaults.h>
#include <CesiumIonClient/Geocoder.h>
#include <CesiumIonClient/Profile.h>
#include <CesiumIonClient/Response.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumNativeTests/SimpleAssetResponse.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/readFile.h>
#include <CesiumNativeTests/waitForFuture.h>

#include <doctest/doctest.h>

#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <utility>

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
      CesiumIonClient::ApplicationData(),
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

TEST_CASE("CesiumIonClient::Connection on single-user mode") {

  std::shared_ptr<SimpleAssetAccessor> pAssetAccessor =
      std::make_shared<SimpleAssetAccessor>(
          std::map<std::string, std::shared_ptr<SimpleAssetRequest>>());

  ApplicationData data;
  data.authenticationMode = AuthenticationMode::SingleUser;

  AsyncSystem asyncSystem(std::make_shared<SimpleTaskProcessor>());
  Connection connection(
      asyncSystem,
      pAssetAccessor,
      "my access token",
      data,
      "https://example.com/");

  Future<Response<Profile>> futureMe = connection.me();
  Response<Profile> me = waitForFuture(asyncSystem, std::move(futureMe));

  CHECK(me.value);
  CHECK(me.value->id == 0);
  CHECK(me.value->username == "ion-user");
}

TEST_CASE("CesiumIonClient::Connection::geocode") {
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
              "geocode.json"));

  std::shared_ptr<SimpleAssetRequest> pRequest =
      std::make_shared<SimpleAssetRequest>(
          "GET",
          "doesn't matter",
          CesiumAsync::HttpHeaders{},
          std::move(pResponse));

  pAssetAccessor->mockCompletedRequests.insert(
      {"https://example.com/v1/geocode/search?text=antarctica&geocoder=bing",
       std::move(pRequest)});

  AsyncSystem asyncSystem(std::make_shared<SimpleTaskProcessor>());
  Connection connection(
      asyncSystem,
      pAssetAccessor,
      "my access token",
      CesiumIonClient::ApplicationData(),
      "https://example.com/");

  Future<Response<GeocoderResult>> futureGeocode = connection.geocode(
      GeocoderProviderType::Bing,
      GeocoderRequestType::Search,
      "antarctica");
  Response<GeocoderResult> geocode =
      waitForFuture(asyncSystem, std::move(futureGeocode));

  REQUIRE(geocode.value);

  CHECK(geocode.value->attributions.size() == 2);
  CHECK(geocode.value->attributions[0].showOnScreen);
  CHECK(!geocode.value->attributions[1].showOnScreen);

  CHECK(geocode.value->features.size() == 5);
  CHECK(geocode.value->features[0].displayName == "Antarctica");
  CHECK(
      geocode.value->features[0].getGlobeRectangle().getNorth() ==
      -1.05716816000174529);

  CHECK(geocode.value->features[1].displayName == "Antarctica, FL");
  CesiumGeospatial::Cartographic center(
      -1.4217365374220714,
      0.4958794631292909);
  CHECK(geocode.value->features[1].getCartographic() == center);

  CHECK(geocode.value->features[2].displayName == "Point Value");
  CesiumGeospatial::Cartographic point =
      CesiumGeospatial::Cartographic::fromDegrees(-180, -90);
  CHECK(geocode.value->features[2].getCartographic() == point);
  CHECK(
      geocode.value->features[2].getGlobeRectangle().getNorth() ==
      point.latitude);
  CHECK(
      geocode.value->features[2].getGlobeRectangle().getSouth() ==
      point.latitude);
  CHECK(
      geocode.value->features[2].getGlobeRectangle().getEast() ==
      point.longitude);
  CHECK(
      geocode.value->features[2].getGlobeRectangle().getWest() ==
      point.longitude);
}
