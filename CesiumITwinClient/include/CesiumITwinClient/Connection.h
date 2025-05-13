#pragma once

#include "AuthenticationToken.h"
#include "CesiumCuratedContent.h"
#include "IModel.h"
#include "IModelMeshExport.h"
#include "ITwin.h"
#include "ITwinRealityData.h"
#include "Library.h"
#include "Profile.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumClientCommon/OAuth2PKCE.h>
#include <CesiumITwinClient/PagedList.h>
#include <CesiumUtility/Result.h>
#include <CesiumUtility/Uri.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace CesiumITwinClient {

/**
 * @brief A common set of query parameters used across list operations in the
 * Bentley API.
 */
struct QueryParameters {
public:
  /**
   * @brief A search string to use to limit results. Not used by all endpoints.
   */
  std::optional<std::string> search;
  /**
   * @brief Used to order the results.
   *
   * Ascending or descending order can be selected with the `asc` and `desc`
   * keywords. Ordering by multiple properties at a time is supported: `name
   * desc,createdDateTime desc`.
   *
   * The set of properties that `orderBy` can reference is endpoint-specific.
   */
  std::optional<std::string> orderBy;

  /**
   * @brief Limits the number of items that can be returned.
   *
   * For example, top = 50 would return only the first 50 items. The limit is
   * 1,000.
   */
  std::optional<uint32_t> top;

  /**
   * @brief Requests that this number of items in the results will be skipped
   * and not returned.
   */
  std::optional<uint32_t> skip;

  /**
   * @brief Adds the parameters in this object to the provided URI query string.
   */
  void addToQuery(CesiumUtility::UriQuery& uriQuery) const;
  /**
   * @brief Adds the parameters in this object to the query of the provided URI.
   */
  void addToUri(CesiumUtility::Uri& uri) const;
};

/**
 * @brief Represents a connection to the Bentley iTwin API.
 */
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
   * @param redirectPort If provided, this will be the port that the internal
   * web server will attempt to bind to. If no port is specified, the server
   * will bind to a random available port.
   * @param scopes The list of scopes that the eventually-granted token should
   * allow access to.
   * @param openUrlCallback A function that is invoked to launch the user's web
   * browser with a given URL so that they can authorize access.
   * @return A future that resolves to an iTwin {@link Connection} once the
   * user authorizes the application and the token handshake completes.
   */
  static CesiumAsync::Future<CesiumUtility::Result<Connection>> authorize(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::string& friendlyApplicationName,
      const std::string& clientID,
      const std::string& redirectPath,
      const std::optional<int>& redirectPort,
      const std::vector<std::string>& scopes,
      std::function<void(const std::string&)>&& openUrlCallback);

  /**
   * @brief Obtains profile information of the currently logged in user.
   *
   * @see https://developer.bentley.com/apis/users/operations/me/
   */
  CesiumAsync::Future<CesiumUtility::Result<UserProfile>> me();

  /**
   * @brief Returns a list of iTwins the current user is a member of.
   *
   * @param params Optional parameters to filter the list results.
   * @see https://developer.bentley.com/apis/itwins/operations/get-my-itwins/
   */
  CesiumAsync::Future<CesiumUtility::Result<PagedList<ITwin>>>
  itwins(const QueryParameters& params);

  /**
   * @brief Returns a list of iModels belonging to the specified iTwin.
   *
   * @param iTwinId The ID of the iTwin to obtain the iModels of.
   * @param params Optional parameters to filter the list results.
   * @see
   * https://developer.bentley.com/apis/imodels-v2/operations/get-itwin-imodels/
   */
  CesiumAsync::Future<CesiumUtility::Result<PagedList<IModel>>>
  imodels(const std::string& iTwinId, const QueryParameters& params);

  /**
   * @brief Returns a list of mesh export tasks for the specified iModel.
   *
   * @param iModelId The ID of the iModel to obtain a list of export tasks for.
   * @param params Optional parameters to filter the list results.
   * @see https://developer.bentley.com/apis/mesh-export/operations/get-exports/
   */
  CesiumAsync::Future<CesiumUtility::Result<PagedList<IModelMeshExport>>>
  meshExports(const std::string& iModelId, const QueryParameters& params);

  /**
   * @brief Returns a list of reality data instances belonging to the specified
   * iTwin.
   *
   * @param iTwinId The ID of the iTwin to obtain a list of reality data
   * instances for.
   * @param params Optional parameters to filter the list results.
   * @see
   * https://developer.bentley.com/apis/reality-management/operations/get-all-reality-data/
   */
  CesiumAsync::Future<CesiumUtility::Result<PagedList<ITwinRealityData>>>
  realityData(const std::string& iTwinId, const QueryParameters& params);

  /**
   * @brief Returns all available iTwin Cesium Curated Content items.
   *
   * @see
   * https://developer.bentley.com/apis/cesium-curated-content/operations/list-content/
   */
  CesiumAsync::Future<
      CesiumUtility::Result<std::vector<CesiumCuratedContentAsset>>>
  cesiumCuratedContent();

  /**
   * @brief Creates a new `Connection` with the provided tokens.
   *
   * It's recommended to use the \ref Connection::authorize method to create a
   * token instead of calling this constructor directly as the `authorize`
   * method will handle the OAuth2 authentication flow with iTwin.
   *
   * @param asyncSystem The \ref CesiumAsync::AsyncSystem to use.
   * @param pAssetAccessor The \ref CesiumAsync::IAssetAccessor to use for
   * making requests to the iTwin API.
   * @param accessToken An \ref AuthenticationToken object created from parsing
   * the obtained iTwin access token.
   * @param refreshToken A refresh token to use to fetch new access tokens as
   * needed, if any.
   * @param clientOptions The set of options to use when interacting with the
   * iTwin OAuth2 API.
   */
  Connection(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const AuthenticationToken& accessToken,
      const std::optional<std::string>& refreshToken,
      const CesiumClientCommon::OAuth2ClientOptions& clientOptions)
      : _asyncSystem(asyncSystem),
        _pAssetAccessor(pAssetAccessor),
        _accessToken(accessToken),
        _refreshToken(refreshToken),
        _clientOptions(clientOptions) {}

  /**
   * @brief Returns the \ref AuthenticationToken object representing the parsed
   * JWT access token.
   */
  const AuthenticationToken& getAccessToken() const { return _accessToken; }
  /**
   * @brief Sets the access token that will be used for API calls.
   *
   * @param authToken The new auth token.
   */
  void setAccessToken(const AuthenticationToken& authToken) {
    _accessToken = authToken;
  }

  /**
   * @brief Returns the refresh token used to obtain new access tokens, if any.
   */
  const std::optional<std::string>& getRefreshToken() const {
    return _refreshToken;
  }
  /**
   * @brief Sets the refresh token used to obtain new access tokens, if any.
   */
  void setRefreshToken(const std::optional<std::string>& refreshToken) {
    _refreshToken = refreshToken;
  }

private:
  CesiumAsync::Future<CesiumUtility::Result<PagedList<ITwin>>>
  listITwins(const std::string& url);
  CesiumAsync::Future<CesiumUtility::Result<PagedList<IModel>>>
  listIModels(const std::string& url);
  CesiumAsync::Future<CesiumUtility::Result<PagedList<IModelMeshExport>>>
  listIModelMeshExports(const std::string& url);
  CesiumAsync::Future<CesiumUtility::Result<PagedList<ITwinRealityData>>>
  listITwinRealityData(const std::string& url);

  CesiumAsync::Future<CesiumUtility::Result<std::string_view>>
  ensureValidToken();

  CesiumAsync::AsyncSystem _asyncSystem;
  std::shared_ptr<CesiumAsync::IAssetAccessor> _pAssetAccessor;
  AuthenticationToken _accessToken;
  std::optional<std::string> _refreshToken;
  CesiumClientCommon::OAuth2ClientOptions _clientOptions;
};
} // namespace CesiumITwinClient