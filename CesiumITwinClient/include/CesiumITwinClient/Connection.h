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
#include <CesiumITwinClient/GeospatialFeatureCollection.h>
#include <CesiumITwinClient/PagedList.h>
#include <CesiumUtility/Result.h>
#include <CesiumUtility/Uri.h>
#include <CesiumVectorData/GeoJsonObjectTypes.h>

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
   * @brief Retrieve all Collections (Feature Classes) that contain features
   * within an iTwin.
   *
   * @param iTwinId The ID of the iTwin to retrieve all collections from.
   * @see
   * https://developer.bentley.com/apis/geospatial-features/operations/get-collections/
   */
  CesiumAsync::Future<
      CesiumUtility::Result<std::vector<GeospatialFeatureCollection>>>
  geospatialFeatureCollections(const std::string& iTwinId);

  /**
   * @brief Returns one or more pages of GeoJSON features in this iTwin.
   *
   * @param iTwinId The ID of the iTwin to load data from.
   * @param collectionId The ID of the data collection to load.
   * @param limit The maximum number of items per page, between 1 and 10,000.
   */
  CesiumAsync::Future<
      CesiumUtility::Result<PagedList<CesiumVectorData::GeoJsonFeature>>>
  geospatialFeatures(
      const std::string& iTwinId,
      const std::string& collectionId,
      uint32_t limit = 10000);

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
   * @param authenticationToken An \ref AuthenticationToken object created from
   * parsing the obtained iTwin access or share token.
   * @param refreshToken A refresh token to use to fetch new access tokens as
   * needed, if any.
   * @param clientOptions The set of options to use when interacting with the
   * iTwin OAuth2 API.
   */
  Connection(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const AuthenticationToken& authenticationToken,
      const std::optional<std::string>& refreshToken,
      const CesiumClientCommon::OAuth2ClientOptions& clientOptions)
      : _asyncSystem(asyncSystem),
        _pAssetAccessor(pAssetAccessor),
        _authenticationToken(authenticationToken),
        _refreshToken(refreshToken),
        _clientOptions(clientOptions) {}

  /**
   * @brief Returns the \ref AuthenticationToken object representing the parsed
   * JWT access or share token.
   */
  const AuthenticationToken& getAuthenticationToken() const {
    return this->_authenticationToken;
  }

  /**
   * @brief Sets the access or share token that will be used for API calls.
   *
   * @param authToken The new auth token.
   */
  void setAuthenticationToken(const AuthenticationToken& authToken) {
    this->_authenticationToken = authToken;
  }

  /**
   * @brief Returns the refresh token used to obtain new access tokens, if any.
   */
  const std::optional<std::string>& getRefreshToken() const {
    return this->_refreshToken;
  }

  /**
   * @brief Sets the refresh token used to obtain new access tokens, if any.
   */
  void setRefreshToken(const std::optional<std::string>& refreshToken) {
    this->_refreshToken = refreshToken;
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
  CesiumAsync::Future<
      CesiumUtility::Result<PagedList<CesiumVectorData::GeoJsonFeature>>>
  listGeospatialFeatures(const std::string& url);

  CesiumAsync::Future<CesiumUtility::Result<std::string>> ensureValidToken();

  CesiumAsync::AsyncSystem _asyncSystem;
  std::shared_ptr<CesiumAsync::IAssetAccessor> _pAssetAccessor;
  AuthenticationToken _authenticationToken;
  std::optional<std::string> _refreshToken;
  CesiumClientCommon::OAuth2ClientOptions _clientOptions;
};
} // namespace CesiumITwinClient