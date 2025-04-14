#include "MockITwinAssetAccessor.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumITwinClient/AuthenticationToken.h>
#include <CesiumITwinClient/Connection.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumUtility/Result.h>
#include <CesiumVectorData/VectorNode.h>

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
    const AuthenticationToken prevToken = pConn->getAuthToken();
    const std::optional<std::string> prevRefreshToken =
        pConn->getRefreshToken();
    REQUIRE(prevRefreshToken);

    // Set an invalid access token.
    pConn->setAuthToken(
        AuthenticationToken("", AccessTokenContents{"", "", {}, 0}, 0));

    CesiumAsync::Future<Result<UserProfile>> future = pConn->me();
    Result<UserProfile> profileResult = future.waitInMainThread();

    CHECK(profileResult.value);
    CHECK(pConn->getAuthToken().getToken() != prevToken.getToken());
    CHECK(pConn->getAuthToken().isValid());
    CHECK(pConn->getRefreshToken());
    CHECK(pConn->getRefreshToken() != prevRefreshToken);
  }
}

TEST_CASE("CesiumITwinClient::Connection::geospatialFeatures") {
  AsyncSystem asyncSystem(std::make_shared<SimpleTaskProcessor>());
  std::shared_ptr<Connection> pConn = createConnection(asyncSystem, false);

  SUBCASE("Returns correct results") {
    CesiumAsync::Future<Result<PagedList<CesiumVectorData::VectorNode>>>
        future = pConn->geospatialFeatures(
            "00000000-0000-0000-0000-000000000000",
            "00000000-0000-0000-0000-000000000000",
            10);
    Result<PagedList<CesiumVectorData::VectorNode>> featuresResult =
        future.waitInMainThread();

    REQUIRE(featuresResult.value);
    CHECK(!featuresResult.errors.hasErrors());

    PagedList<CesiumVectorData::VectorNode>& list = *featuresResult.value;
    CHECK(list.size() == 10);
    const int64_t* pId = std::get_if<int64_t>(&list[5].id);
    REQUIRE(pId);
    CHECK(*pId == 133);

    REQUIRE(list[5].properties);
    CHECK((*list[5].properties)["type"].isString());
    CHECK((*list[5].properties)["type"].getString() == "Lamp_post");

    REQUIRE(!list[5].children.empty());
    CHECK(!list[5].children[0].primitives.empty());
    const CesiumGeospatial::Cartographic* pCartographic =
        std::get_if<CesiumGeospatial::Cartographic>(
            &list[5].children[0].primitives[0]);
    REQUIRE(pCartographic);
    CHECK(
        *pCartographic == CesiumGeospatial::Cartographic::fromDegrees(
                              103.839238468,
                              1.348559984,
                              7.813700195));
  }
}