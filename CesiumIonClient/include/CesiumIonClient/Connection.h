#pragma once

#include "CesiumAsync/AsyncSystem.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumAsync/Library.h"
#include "CesiumIonClient/Assets.h"
#include "CesiumIonClient/Profile.h"
#include "CesiumIonClient/Response.h"
#include "CesiumIonClient/Token.h"
#include <cstdint>

namespace CesiumIonClient {
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
   * See [Connection to Cesium ion with OAuth2](https://cesium.com/docs/oauth/)
   * for a description of the authorization process.
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
      const std::string& ionApiUrl = "https://api.cesium.com/",
      const std::string& ionAuthorizeUrl = "https://cesium.com/ion/oauth");

  /**
   * @brief Creates a connection to Cesium ion using the provided access token.
   *
   * @param asyncSystem The async system used to do work in threads.
   * @param pAssetAccessor The interface used to interact with the Cesium ion
   * REST API.
   * @param accessToken The access token
   * @param apiUrl The base URL of the Cesium ion API.
   */
  Connection(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::string& accessToken,
      const std::string& apiUrl = "https://api.cesium.com");

  /**
   * @brief Gets the async system used by this connection to do work in threads.
   */
  const CesiumAsync::AsyncSystem& getAsyncSystem() const {
    return this->_asyncSystem;
  }

  /**
   * @brief Gets the interface used by this connection to interact with the
   * Cesium ion REST API.
   */
  const std::shared_ptr<CesiumAsync::IAssetAccessor>& getAssetAccessor() const {
    return this->_pAssetAccessor;
  }

  /**
   * @brief Gets the access token used by this connection.
   */
  const std::string& getAccessToken() const { return this->_accessToken; }

  /**
   * @brief Gets the Cesium ion API base URL.
   */
  const std::string& getApiUrl() const { return this->_apiUrl; }

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
   * @brief Gets the list of available assets.
   *
   * @return A future that resolves to the asset information.
   */
  CesiumAsync::Future<Response<Assets>> assets() const;

  /**
   * @brief Gets the list of available tokens.
   *
   * @return A future that resolves to the token information.
   */
  CesiumAsync::Future<Response<std::vector<Token>>> tokens() const;

  /**
   * @brief Creates a new token.
   *
   * @param name The name of the new token.
   * @param scopes The scopes allowed by this token.
   * @param assets The assets that may be accessed by this token. If
   * `std::nullopt`, access to all assets is allowed.
   * @return The new token.
   */
  CesiumAsync::Future<Response<Token>> createToken(
      const std::string& name,
      const std::vector<std::string>& scopes,
      const std::optional<std::vector<int64_t>>& assets = std::nullopt) const;

private:
  static CesiumAsync::Future<Connection> completeTokenExchange(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      int64_t clientID,
      const std::string& ionApiUrl,
      const std::string& code,
      const std::string& redirectUrl,
      const std::string& codeVerifier);

  CesiumAsync::AsyncSystem _asyncSystem;
  std::shared_ptr<CesiumAsync::IAssetAccessor> _pAssetAccessor;
  std::string _accessToken;
  std::string _apiUrl;
};
} // namespace CesiumIonClient
