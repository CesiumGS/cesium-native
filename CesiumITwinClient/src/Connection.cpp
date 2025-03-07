#include "CesiumITwinClient/AuthToken.h"

#include <CesiumAsync/Future.h>
#include <CesiumAsync/Promise.h>
#include <CesiumClientCommon/OAuth2PKE.h>
#include <CesiumITwinClient/Connection.h>
#include <CesiumUtility/Uri.h>

#include <string>

using namespace CesiumAsync;
using namespace CesiumUtility;

const std::string ITWIN_AUTHORIZE_URL =
    "https://ims.bentley.com/connect/authorize";
const std::string ITWIN_TOKEN_URL = "https://ims.bentley.com/connect/token";

namespace CesiumITwinClient {
CesiumAsync::Future<Connection> Connection::authorize(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::string& friendlyApplicationName,
    const std::string& clientID,
    const std::string& redirectPath,
    const std::vector<std::string>& scopes,
    std::function<void(const std::string&)>&& openUrlCallback) {
  Promise<Connection> connectionPromise =
      asyncSystem.createPromise<Connection>();

  CesiumClientCommon::OAuth2PKE::authorize(
      asyncSystem,
      pAssetAccessor,
      friendlyApplicationName,
      clientID,
      redirectPath,
      scopes,
      std::move(openUrlCallback),
      ITWIN_TOKEN_URL,
      ITWIN_AUTHORIZE_URL)
      .thenImmediately(
          [asyncSystem, pAssetAccessor, connectionPromise](
              const Result<CesiumClientCommon::OAuth2TokenResponse>& result) {
            if (!result.value.has_value()) {
              connectionPromise.reject(std::runtime_error(fmt::format(
                  "Failed to complete authorization: {}",
                  joinToString(result.errors.errors, ", "))));
            } else {
              Result<AuthToken> authTokenResult =
                  AuthToken::parse(result.value->accessToken);
              if (!authTokenResult.value.has_value() || !authTokenResult.value->isValid()) {
                connectionPromise.reject(std::runtime_error(fmt::format(
                    "Invalid auth token obtained: {}",
                    joinToString(authTokenResult.errors.errors, ", "))));
              } else {
                connectionPromise.resolve(Connection(
                    asyncSystem,
                    pAssetAccessor,
                    *authTokenResult.value,
                    result.value->refreshToken));
              }
            }
          });

  return connectionPromise.getFuture();
}
} // namespace CesiumITwinClient