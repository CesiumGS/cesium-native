#include "CesiumUtility/Result.h"
#include "MockITwinAssetAccessor.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumITwinClient/Connection.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>

#include <doctest/doctest.h>

#include <memory>

using namespace CesiumITwinClient;
using namespace CesiumAsync;
using namespace CesiumNativeTests;
using namespace CesiumUtility;

namespace {
const std::string& REDIRECT_PATH = "/dummy/auth/path";
const int REDIRECT_PORT = 49013;

std::shared_ptr<Connection> createConnection(const AsyncSystem& asyncSystem) {
  std::shared_ptr<MockITwinAssetAccessor> pAccessor =
      std::make_shared<MockITwinAssetAccessor>();

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
    const AuthenticationToken prevToken = pConn->getAccessToken();
    const std::optional<std::string> prevRefreshToken =
        pConn->getRefreshToken();
    REQUIRE(prevRefreshToken);

    // Set an invalid access token.
    pConn->setAccessToken(AuthenticationToken("", "", "", {}, 0, 0));

    CesiumAsync::Future<Result<UserProfile>> future = pConn->me();
    Result<UserProfile> profileResult = future.waitInMainThread();

    CHECK(profileResult.value);
    CHECK(pConn->getAccessToken().getToken() != prevToken.getToken());
    CHECK(pConn->getAccessToken().isValid());
    CHECK(pConn->getRefreshToken());
    CHECK(pConn->getRefreshToken() != prevRefreshToken);
  }
}