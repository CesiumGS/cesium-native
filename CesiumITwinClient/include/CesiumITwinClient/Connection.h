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
  static CesiumAsync::Future<Connection> authorize(
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
  listITwins(const QueryParameters& params);

  /**
   * @brief Returns a list of iModels belonging to the specified iTwin.
   *
   * @param iTwinId The ID of the iTwin to obtain the iModels of.
   * @param params Optional parameters to filter the list results.
   * @see
   * https://developer.bentley.com/apis/imodels-v2/operations/get-itwin-imodels/
   */
  CesiumAsync::Future<CesiumUtility::Result<PagedList<IModel>>>
  listIModels(const std::string& iTwinId, const QueryParameters& params);

  /**
   * @brief Returns a list of mesh export tasks for the specified iModel.
   *
   * @param iModelId The ID of the iModel to obtain a list of export tasks for.
   * @param params Optional parameters to filter the list results.
   * @see https://developer.bentley.com/apis/mesh-export/operations/get-exports/
   */
  CesiumAsync::Future<CesiumUtility::Result<PagedList<IModelMeshExport>>>
  listIModelMeshExports(
      const std::string& iModelId,
      const QueryParameters& params);

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
  listITwinRealityData(
      const std::string& iTwinId,
      const QueryParameters& params);

  /**
   * @brief Returns all available iTwin Cesium Curated Content items.
   *
   * @see
   * https://developer.bentley.com/apis/cesium-curated-content/operations/list-content/
   */
  CesiumAsync::Future<
      CesiumUtility::Result<std::vector<ITwinCesiumCuratedContentItem>>>
  listCesiumCuratedContent();

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

  const AuthToken& getAccessToken() const { return _authToken; }
  void setAccessToken(const AuthToken& authToken) { _authToken = authToken; }

  const std::optional<std::string>& getRefreshToken() const {
    return _refreshToken;
  }
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
  AuthToken _authToken;
  std::optional<std::string> _refreshToken;
  CesiumClientCommon::OAuth2ClientOptions _clientOptions;
};
} // namespace CesiumITwinClient