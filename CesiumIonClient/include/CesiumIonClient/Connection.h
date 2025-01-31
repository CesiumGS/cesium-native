#pragma once

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/Library.h>
#include <CesiumIonClient/ApplicationData.h>
#include <CesiumIonClient/Assets.h>
#include <CesiumIonClient/Defaults.h>
#include <CesiumIonClient/Geocoder.h>
#include <CesiumIonClient/Profile.h>
#include <CesiumIonClient/Response.h>
#include <CesiumIonClient/Token.h>
#include <CesiumIonClient/TokenList.h>

#include <cstdint>

namespace CesiumIonClient {

/**
 * @brief Whether sorted results should be ascending or descending.
 */
enum class SortOrder { Ascending, Descending };

/**
 * @brief Options to be passed to {@link Connection::tokens}.
 */
struct ListTokensOptions {
  /**
   * @brief The maximum number of tokens to return in a single page.
   *
   * Receiving fewer tokens should not be interpreted as the end of the
   * collection. The end of the collection is reached when the response does not
   * contain {@link Response::nextPageUrl}.
   */
  std::optional<int32_t> limit;

  /**
   * @brief The page number, where the first page of results is page 1 (not 0).
   */
  std::optional<int32_t> page;

  /**
   * @brief One or more keywords separated by whitespace by which to
   * filter the list of tokens. The token name will contain each keyword of the
   * search string.
   */
  std::optional<std::string> search;

  /**
   * @brief The property by which to sort results. Valid values are
   * `"NAME"` and `"LAST_USED"`.
   */
  std::optional<std::string> sortBy;

  /**
   * @brief The property by which to sort results. Valid values are
   * `"NAME"` and `"LAST_USED"`.
   */
  std::optional<SortOrder> sortOrder;
};

/**
 * @brief A connection to Cesium ion that can be used to interact with it via
 * its REST API.
 */
class CESIUMASYNC_API Connection {
public:
  /**
   * @brief Authorizes access to Cesium ion on behalf of a user, and returns a
   * {@link Connection} that can be used to interact with ion.
   *
   * Uses the "Authorization Code with PKCE" OAuth2 flow.
   *
   * See [Connecting to Cesium ion with
   * OAuth2](https://cesium.com/learn/ion/ion-oauth2/) for a description of the
   * authorization process.
   *
   * @param asyncSystem The async system used to do work in threads.
   * @param pAssetAccessor The interface used to interact with the Cesium ion
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
   * @param appData The app data retrieved from the Cesium ion server.
   * @param ionApiUrl The base URL of the Cesium ion API.
   * @param ionAuthorizeUrl The URL of the Cesium ion OAuth authorization page.
   * @return A future that resolves to a Cesium ion {@link Connection} once the
   * user authorizes the application and the token handshake completes.
   */
  static CesiumAsync::Future<Connection> authorize(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::string& friendlyApplicationName,
      int64_t clientID,
      const std::string& redirectPath,
      const std::vector<std::string>& scopes,
      std::function<void(const std::string&)>&& openUrlCallback,
      const CesiumIonClient::ApplicationData& appData,
      const std::string& ionApiUrl = "https://api.cesium.com/",
      const std::string& ionAuthorizeUrl = "https://ion.cesium.com/oauth");

  /**
   * @brief Retrieves information about the ion API server.
   *
   * @param asyncSystem The async system used to do work in threads.
   * @param pAssetAccessor The interface used to interact with the Cesium ion
   * REST API.
   * @param apiUrl The URL of the ion REST API to make requests against.
   * @return A future that resolves to the application information.
   */
  static CesiumAsync::Future<Response<ApplicationData>> appData(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::string& apiUrl = "https://api.cesium.com");

  /**
   * @brief Attempts to retrieve the ion endpoint URL by looking for a
   * `config.json` file on the server.
   *
   * This config file isn't present on `ion.cesium.com`, but will be present on
   * Cesium ion self-hosted instances to allow the user to configure the URLs of
   * their self-hosted instance as needed.
   *
   * @param asyncSystem The async system used to do work in threads.
   * @param pAssetAccessor The interface used to interact with the Cesium ion
   * REST API.
   * @param ionUrl The URL of the Cesium ion instance to make this request
   * against.
   * @returns The Cesium ion REST API url for this ion instance, or
   * `std::nullopt` if none found.
   */
  static CesiumAsync::Future<std::optional<std::string>> getApiUrl(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::string& ionUrl);

  /**
   * @brief Creates a connection to Cesium ion using the provided access token.
   *
   * @param asyncSystem The async system used to do work in threads.
   * @param pAssetAccessor The interface used to interact with the Cesium ion
   * REST API.
   * @param accessToken The access token
   * @param appData The app data retrieved from the Cesium ion server.
   * @param apiUrl The base URL of the Cesium ion API.
   */
  Connection(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::string& accessToken,
      const CesiumIonClient::ApplicationData& appData,
      const std::string& apiUrl = "https://api.cesium.com");

  /**
   * @brief Gets the async system used by this connection to do work in threads.
   */
  const CesiumAsync::AsyncSystem& getAsyncSystem() const noexcept {
    return this->_asyncSystem;
  }

  /**
   * @brief Gets the interface used by this connection to interact with the
   * Cesium ion REST API.
   */
  const std::shared_ptr<CesiumAsync::IAssetAccessor>&
  getAssetAccessor() const noexcept {
    return this->_pAssetAccessor;
  }

  /**
   * @brief Gets the access token used by this connection.
   */
  const std::string& getAccessToken() const noexcept {
    return this->_accessToken;
  }

  /**
   * @brief Gets the Cesium ion API base URL.
   */
  const std::string& getApiUrl() const noexcept { return this->_apiUrl; }

  /**
   * @brief Retrieves profile information for the access token currently being
   * used to make API calls.
   *
   * This route works with any valid token, but additional information is
   * returned if the token uses the `profile:read` scope.
   *
   * @return A future that resolves to the profile information.
   */
  CesiumAsync::Future<Response<Profile>> me() const;

  /**
   * @brief Retrieves default imagery, terrain and building assets along with
   * quick add assets that can be useful to use within other applications.
   *
   * This route will always return data, but will return user specific
   * information with any valid token.
   *
   * @return A future that resolves to the default information.
   */
  CesiumAsync::Future<Response<Defaults>> defaults() const;

  /**
   * @brief Gets the list of available assets.
   *
   * @return A future that resolves to the asset information.
   */
  CesiumAsync::Future<Response<Assets>> assets() const;

  /**
   * @brief Invokes the "List tokens" service to get the list of available
   * tokens.
   *
   * Only a single page is returned. To obtain additional pages, use
   * {@link Connection::nextPage} and {@link Connection::previousPage}.
   *
   * @param options Options to include in the "List tokens" request.
   * @return A future that resolves to a page of token information.
   */
  CesiumAsync::Future<Response<TokenList>>
  tokens(const ListTokensOptions& options = {}) const;

  /**
   * @brief Gets details of the asset with the given ID.
   *
   * @param assetID The asset ID.
   * @return A future that resolves to the asset details.
   */
  CesiumAsync::Future<Response<Asset>> asset(int64_t assetID) const;

  /**
   * @brief Gets details of the token with the given ID.
   *
   * @param tokenID The token ID.
   * @return A future that resolves to the token details.
   */
  CesiumAsync::Future<Response<Token>> token(const std::string& tokenID) const;

  /**
   * @brief Gets the next page of results from the "List tokens" service.
   *
   * To get the first page, use {@link Connection::tokens}.
   *
   * @param currentPage The current page from which to get the next page.
   * @return A future that resolves to the next page of tokens, or to a
   * response with an {@link Response::errorCode} of "NoMorePages" if the
   * currentPage is the last one.
   */
  CesiumAsync::Future<Response<TokenList>>
  nextPage(const Response<TokenList>& currentPage) const;

  /**
   * @brief Gets the previous page of results from the "List tokens" service.
   *
   * To get the first page (or a particular page), use {@link Connection::tokens}.
   *
   * @param currentPage The current page from which to get the previous page.
   * @return A future that resolves to the previous page of tokens, or to a
   * response with an {@link Response::errorCode} of "NoMorePages" if the
   * currentPage is the first one.
   */
  CesiumAsync::Future<Response<TokenList>>
  previousPage(const Response<TokenList>& currentPage) const;

  /**
   * @brief Creates a new token.
   *
   * @param name The name of the new token.
   * @param scopes The scopes allowed by this token.
   * @param assetIds The assets that may be accessed by this token. If
   * `std::nullopt`, access to all assets is allowed.
   * @param allowedUrls The URLs from which this token can be accessed. If
   * `std::nullopt`, the token can be accessed from any URL.
   * @return The new token.
   */
  CesiumAsync::Future<Response<Token>> createToken(
      const std::string& name,
      const std::vector<std::string>& scopes,
      const std::optional<std::vector<int64_t>>& assetIds = std::nullopt,
      const std::optional<std::vector<std::string>>& allowedUrls =
          std::nullopt) const;

  /**
   * @brief Modifies a token.
   *
   * @param tokenID The ID of the token to modify.
   * @param newName The new name of the token.
   * @param newAssetIDs The assets that may be accessed by this token. If
   * `std::nullopt`, access to all assets is allowed.
   * @param newScopes The new OAuth scopes allowed by this token.
   * @param newAllowedUrls The new URLs from which this token can be accessed.
   * If `std::nullopt`, the token can be accessed from any URL.
   * @return A value-less response. If the response is successful, the token has
   * been modified.
   */
  CesiumAsync::Future<Response<NoValue>> modifyToken(
      const std::string& tokenID,
      const std::string& newName,
      const std::optional<std::vector<int64_t>>& newAssetIDs,
      const std::vector<std::string>& newScopes,
      const std::optional<std::vector<std::string>>& newAllowedUrls) const;

  /**
   * @brief Makes a request to the ion geocoding service.
   *
   * A geocoding service is used to make a plain text query (like an address,
   * city name, or landmark) and obtain information about where it's located.
   *
   * @param provider The ion geocoding provider to use.
   * @param type The type of request to make. See {@link GeocoderRequestType} for more information.
   * @param query The query to make.
   */
  CesiumAsync::Future<Response<GeocoderResult>> geocode(
      GeocoderProviderType provider,
      GeocoderRequestType type,
      const std::string& query);

  /**
   * @brief Decodes a token ID from a token.
   *
   * @param token The token to decode.
   * @return The token ID, or std::nullopt if the token ID cannot be determined
   * from the token.
   */
  static std::optional<std::string> getIdFromToken(const std::string& token);

private:
  static CesiumAsync::Future<Connection> completeTokenExchange(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      int64_t clientID,
      const std::string& ionApiUrl,
      const CesiumIonClient::ApplicationData& appData,
      const std::string& code,
      const std::string& redirectUrl,
      const std::string& codeVerifier);

  CesiumAsync::Future<Response<TokenList>> tokens(const std::string& url) const;

  CesiumAsync::AsyncSystem _asyncSystem;
  std::shared_ptr<CesiumAsync::IAssetAccessor> _pAssetAccessor;
  std::string _accessToken;
  std::string _apiUrl;
  CesiumIonClient::ApplicationData _appData;
};
} // namespace CesiumIonClient
