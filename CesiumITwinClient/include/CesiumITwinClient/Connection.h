#pragma once

#include "AuthToken.h"
#include "CesiumClientCommon/OAuth2PKE.h"
#include "Library.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumITwinClient/PagedList.h>
#include <CesiumITwinClient/Resources.h>
#include <CesiumUtility/Result.h>
#include <CesiumUtility/Uri.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace CesiumITwinClient {
struct QueryParameters {
public:
  std::optional<std::string> search;
  std::optional<std::string> orderBy;
  std::optional<uint32_t> top;
  std::optional<uint32_t> skip;

  void addToQuery(CesiumUtility::UriQuery& uriQuery) const;
  void addToUri(CesiumUtility::Uri& uri) const;
};

class CESIUMITWINCLIENT_API Connection {
public:
  /**
   * @brief Authorizes access to iTwin on behalf of a user, and returns a
   * {@link Connection} that can be used to interact with the iTwin API.
   *
   * Uses the "Authorization Code with PKCE" OAuth2 flow.
   *
   * See [Authorize Native
   * Application](https://developer.bentley.com/tutorials/authorize-native/) for
   * a description of the authorization process.
   *
   * @param asyncSystem The async system used to do work in threads.
   * @param pAssetAccessor The interface used to interact with the iTwin
   * REST API.
   * @param friendlyApplicationName A friendly name for the application
   * requesting access. It will be displayed to the user when authorization is
   * complete, informing them that they can return to the original application.
   * @param clientID The client ID that was assigned to your application when
   * you registered it.
   * @param redirectPath The path on http://127.0.0.1 that a user will be
   * redirected to once they authorize your application. This must match the URI
   * provided when you registered your application, without the protocol,
   * hostname, or port.
   * @param scopes The list of scopes that the eventually-granted token should
   * allow access to.
   * @param openUrlCallback A function that is invoked to launch the user's web
   * browser with a given URL so that they can authorize access.
   * @return A future that resolves to an iTwin {@link Connection} once the
   * user authorizes the application and the token handshake completes.
   */
  static CesiumAsync::Future<Connection> authorize(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::string& friendlyApplicationName,
      const std::string& clientID,
      const std::string& redirectPath,
      const std::vector<std::string>& scopes,
      std::function<void(const std::string&)>&& openUrlCallback);

  CesiumAsync::Future<CesiumUtility::Result<PagedList<ITwin>>> listITwins(const QueryParameters& params);

private:
  CesiumAsync::Future<CesiumUtility::Result<PagedList<ITwin>>> listITwins(const std::string& url);

  CesiumAsync::Future<
      CesiumUtility::Result<std::vector<ITwinCesiumCuratedContentItem>>>
  listCesiumCuratedContent();

  CesiumAsync::Future<CesiumUtility::Result<std::string_view>>
  ensureValidToken();

  Connection(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const AuthToken& authToken,
      const std::optional<std::string>& refreshToken,
      const CesiumClientCommon::OAuth2ClientOptions& clientOptions)
      : _asyncSystem(asyncSystem),
        _pAssetAccessor(pAssetAccessor),
        _authToken(authToken),
        _refreshToken(refreshToken),
        _clientOptions(clientOptions) {}

  CesiumAsync::AsyncSystem _asyncSystem;
  std::shared_ptr<CesiumAsync::IAssetAccessor> _pAssetAccessor;
  AuthToken _authToken;
  std::optional<std::string> _refreshToken;
  CesiumClientCommon::OAuth2ClientOptions _clientOptions;
};
} // namespace CesiumITwinClient