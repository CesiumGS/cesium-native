#pragma once

#include "Library.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumUtility/Result.h>

#include <optional>
#include <string>

namespace CesiumClientCommon {

/**
 * @brief Tokens obtained from a successful OAuth2 authentication operation.
 */
struct OAuth2TokenResponse {
  /**
   * @brief The access token returned. This can be used to authenticate
   * requests.
   */
  std::string accessToken;
  /**
   * @brief The refresh token returned, if any.
   *
   * If provided, this can be used with \ref OAuth2PKCE::refresh to obtain a new
   * access token.
   */
  std::optional<std::string> refreshToken;
};

/**
 * @brief Options used to configure the OAuth2 authentication process.
 */
struct OAuth2ClientOptions {
  /**
   * @brief The OAuth2 client ID.
   */
  std::string clientID;
  /**
   * @brief The URL path that will be used to create the redirect URI.
   *
   * The final redirect URI will be
   * `http://127.0.0.1:<redirectPort>/<redirectPath>`. This should match what is
   * configured in the developer settings for the service you are authenticating
   * with.
   */
  std::string redirectPath;
  /**
   * @brief The port that the internal HTTP server will listen on.
   *
   * If this is `std::nullopt`, a random available port will be chosen. You may
   * need to explicitly specify the port if the API you are authenticating with
   * requires a specific port to be provided in the developer settings.
   */
  std::optional<int> redirectPort = std::nullopt;
  /**
   * @brief Whether requests against the token and refresh endpoints should use
   * a JSON body to the POST request.
   *
   * If false, the body will be specified in the
   * `application/x-www-form-urlencoded` format.
   */
  bool useJsonBody;
};

/**
 * @brief Class for authenticating with an API that uses OAuth2 Proof of Key
 * Code Exchange (PKCE).
 *
 * For more information:
 * - [Authorization code flow with Proof Key for Code Exchange
 * (PKCE)](https://developer.bentley.com/apis/overview/authorization/native-spa/#authorization-code-flow-with-proof-key-for-code-exchange-pkce)
 * - [Connecting to Cesium ion with
 * OAuth2](https://cesium.com/learn/ion/ion-oauth2/)
 */
class CESIUMCLIENTCOMMON_API OAuth2PKCE {
public:
  /**
   * @brief Initiates the OAuth2 PKCE authentication process.
   *
   * This will start an internal HTTP server to listen on the redirect URI for a
   * response from the authorization endpoint. This HTTP is not currently
   * shutdown until the user visits this redirect URI.
   *
   * @param asyncSystem The \ref CesiumAsync::AsyncSystem to use.
   * @param pAssetAccessor The \ref CesiumAsync::IAssetAccessor to use for API
   * requests.
   * @param friendlyApplicationName The name of the application that will be
   * displayed to the user in status messages from the internal HTTP server.
   * @param clientOptions Options to configure the OAuth2 authentication
   * process.
   * @param scopes A set of scopes that the token obtained from the
   * authentication process will have access to.
   * @param openUrlCallback A callback that will be called with the created
   * redirect URI after the internal server has been created. This callback
   * should either present the URL to the user or open the URL in a web browser.
   * @param tokenEndpointUrl The endpoint that will be requested to attempt to
   * obtain an access token after the code has been acquired from the redirect
   * URI.
   * @param authorizeBaseUrl The base URL of the page the user will be prompted
   * to open to confirm the authorization attempt.
   *
   * @returns A `Future` returning a `Result` containing either a
   * `OAuth2TokenResponse` or error messages.
   */
  static CesiumAsync::Future<CesiumUtility::Result<OAuth2TokenResponse>>
  authorize(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::string& friendlyApplicationName,
      const OAuth2ClientOptions& clientOptions,
      const std::vector<std::string>& scopes,
      std::function<void(const std::string&)>&& openUrlCallback,
      const std::string& tokenEndpointUrl,
      const std::string& authorizeBaseUrl);

  /**
   * @brief Attempts to obtain new access and refresh tokens using a refresh
   * token obtained from a previous call to `authorize`.
   *
   * @param asyncSystem The \ref CesiumAsync::AsyncSystem to use.
   * @param pAssetAccessor The \ref CesiumAsync::IAssetAccessor to use for API
   * requests.
   * @param clientOptions Options to configure the OAuth2 authentication
   * process.
   * @param refreshBaseUrl The base URL of the endpoint that will be requested
   * to attempt to obtain new access and refresh tokens.
   * @param refreshToken The refresh token.
   *
   * @returns A `Future` returning a `Result` containing either a
   * `OAuth2TokenResponse` or error messages.
   */
  static CesiumAsync::Future<CesiumUtility::Result<OAuth2TokenResponse>>
  refresh(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const OAuth2ClientOptions& clientOptions,
      const std::string& refreshBaseUrl,
      const std::string& refreshToken);

private:
  static CesiumAsync::Future<CesiumUtility::Result<OAuth2TokenResponse>>
  completeTokenExchange(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const OAuth2ClientOptions& clientOptions,
      const std::string& tokenEndpointUrl,
      const std::string& code,
      const std::string& redirectUrl,
      const std::string& codeVerifier);
};

} // namespace CesiumClientCommon