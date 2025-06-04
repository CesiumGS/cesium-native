#include "MockITwinAssetAccessor.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumGeometry/Axis.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumITwinClient/AuthenticationToken.h>
#include <CesiumITwinClient/Connection.h>
#include <CesiumITwinClient/GeospatialFeatureCollection.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumUtility/Result.h>
#include <CesiumVectorData/GeoJsonObject.h>

#include <doctest/doctest.h>

#include <memory>
#include <optional>

using namespace CesiumITwinClient;
using namespace CesiumAsync;
using namespace CesiumNativeTests;
using namespace CesiumUtility;

namespace {
const std::string& REDIRECT_PATH = "/dummy/auth/path";
const int REDIRECT_PORT = 49013;

std::shared_ptr<Connection>
createConnection(const AsyncSystem& asyncSystem, bool isAccessToken = true) {
  std::shared_ptr<MockITwinAssetAccessor> pAccessor =
      std::make_shared<MockITwinAssetAccessor>(isAccessToken);

  Result<AuthenticationToken> tokenResult =
      AuthenticationToken::parse(pAccessor->authToken);
  REQUIRE(tokenResult.value);

  return std::make_shared<Connection>(
      asyncSystem,
      pAccessor,
      *tokenResult.value,
      pAccessor->refreshToken,
      CesiumClientCommon::OAuth2ClientOptions{
          "ClientID",
          REDIRECT_PATH,
          REDIRECT_PORT,
          false});
}
} // namespace

TEST_CASE("CesiumITwinClient::Connection::me") {
  AsyncSystem asyncSystem(std::make_shared<SimpleTaskProcessor>());
  std::shared_ptr<Connection> pConn = createConnection(asyncSystem);

  SUBCASE("Returns correct results") {
    CesiumAsync::Future<Result<UserProfile>> future = pConn->me();
    Result<UserProfile> profileResult = future.waitInMainThread();

    REQUIRE(profileResult.value);
    CHECK(profileResult.value->id == "00000000-0000-0000-0000-000000000000");
    CHECK(profileResult.value->displayName == "John.Smith@example.com");
    CHECK(profileResult.value->givenName == "John");
    CHECK(profileResult.value->surname == "Smith");
    CHECK(profileResult.value->email == "John.Smith@example.com");
  }

  SUBCASE("Handles refreshing token") {
    const AuthenticationToken prevToken = pConn->getAuthenticationToken();
    const std::optional<std::string> prevRefreshToken =
        pConn->getRefreshToken();
    REQUIRE(prevRefreshToken);

    // Set an invalid access token.
    pConn->setAuthenticationToken(AuthenticationToken("", "", "", {}, 0, 0));

    CesiumAsync::Future<Result<UserProfile>> future = pConn->me();
    Result<UserProfile> profileResult = future.waitInMainThread();

    CHECK(profileResult.value);
    CHECK(pConn->getAuthenticationToken().getToken() != prevToken.getToken());
    CHECK(pConn->getAuthenticationToken().isValid());
    CHECK(pConn->getRefreshToken());
    CHECK(pConn->getRefreshToken() != prevRefreshToken);
  }
}

TEST_CASE("CesiumITwinClient::Connection::geospatialFeatures") {
  AsyncSystem asyncSystem(std::make_shared<SimpleTaskProcessor>());
  std::shared_ptr<Connection> pConn = createConnection(asyncSystem, false);

  SUBCASE("Returns correct results") {
    CesiumAsync::Future<Result<PagedList<CesiumVectorData::GeoJsonFeature>>>
        future = pConn->geospatialFeatures(
            "00000000-0000-0000-0000-000000000000",
            "00000000-0000-0000-0000-000000000000",
            10);
    Result<PagedList<CesiumVectorData::GeoJsonFeature>> featuresResult =
        future.waitInMainThread();

    REQUIRE(featuresResult.value);
    CHECK(!featuresResult.errors.hasErrors());

    PagedList<CesiumVectorData::GeoJsonFeature>& list = *featuresResult.value;
    CHECK(list.size() == 10);
    const int64_t* pId = std::get_if<int64_t>(&list[5].id);
    REQUIRE(pId);
    CHECK(*pId == 133);

    REQUIRE(list[5].properties);
    CHECK((*list[5].properties)["type"].isString());
    CHECK((*list[5].properties)["type"].getString() == "Lamp_post");

    REQUIRE(list[5].geometry);
    const CesiumVectorData::GeoJsonPoint* pPoint =
        list[5].geometry->getIf<CesiumVectorData::GeoJsonPoint>();
    REQUIRE(pPoint);
    CHECK(
        pPoint->coordinates ==
        glm::dvec3(103.839238468, 1.348559984, 7.813700195));
  }
}

TEST_CASE("CesiumITwinClient::Connection::geospatialFeatureCollections") {
  AsyncSystem asyncSystem(std::make_shared<SimpleTaskProcessor>());
  std::shared_ptr<Connection> pConn = createConnection(asyncSystem, false);

  SUBCASE("Returns correct results") {
    CesiumAsync::Future<Result<std::vector<GeospatialFeatureCollection>>>
        future = pConn->geospatialFeatureCollections(
            "00000000-0000-0000-0000-000000000000");
    Result<std::vector<GeospatialFeatureCollection>> collectionsResult =
        future.waitInMainThread();

    REQUIRE(collectionsResult.value);
    CHECK(!collectionsResult.errors.hasErrors());

    const std::vector<GeospatialFeatureCollection>& collections =
        *collectionsResult.value;
    REQUIRE(!collections.empty());

    const GeospatialFeatureCollection& collection = collections[0];
    CHECK(collection.id == "90442b2b-a7e6-4471-b093-cb002a37762a");
    CHECK(collection.title == "Title");
    CHECK(collection.description == "Description");

    REQUIRE(!collection.extents.spatial.empty());
    const CesiumGeometry::AxisAlignedBox& spatialExtents =
        collection.extents.spatial[0];
    CHECK(spatialExtents.minimumX == -50.08876885548398);
    CHECK(spatialExtents.minimumY == 50.94487570541774);
    CHECK(spatialExtents.maximumX == -50.08830149142197);
    CHECK(spatialExtents.maximumY == 50.94521538951092);
    CHECK(spatialExtents.minimumZ == 0.0003396840931770839);
    CHECK(spatialExtents.maximumZ == 0.0004673640620040942);
    CHECK(
        collection.extents.coordinateReferenceSystem ==
        "https://www.opengis.net/def/crs/OGC/1.3/CRS84");

    REQUIRE(!collection.extents.temporal.empty());
    CHECK(collection.extents.temporal[0].first == "2011-11-11T12:22:11Z");
    CHECK(collection.extents.temporal[0].second == "");
    CHECK(
        collection.extents.temporalReferenceSystem ==
        "http://www.opengis.net/def/uom/ISO-8601/0/Gregorian");

    REQUIRE(!collection.crs.empty());
    CHECK(collection.crs[0] == "https://www.opengis.net/def/crs/EPSG/0/32615");
    CHECK(
        collection.storageCrs ==
        "https://www.opengis.net/def/crs/EPSG/0/32615");
  }
}