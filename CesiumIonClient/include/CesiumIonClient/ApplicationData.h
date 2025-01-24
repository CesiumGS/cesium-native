#pragma once

#include <string>

namespace CesiumIonClient {

/**
 *  @brief An enumeration representing the values of the `authenticationMode`
 * property in the `appData` response.
 */
enum AuthenticationMode {
  /**
   * Authentication using OAuth with an ion.cesium.com account.
   */
  CesiumIon,
  /**
   * Authentication using OAuth with Cesium ion Self-Hosted.
   * On the server, this uses the Security Assertion Markup Language (SAML) to
   * communicate with another authentication server.
   * From our perspective, we can treat this the same as {@link AuthenticationMode::CesiumIon}.
   */
  Saml,
  /**
   * A Cesium ion Self-Hosted server without authentication.
   * In single-user mode, any application that can reach the server has
   * permissions to use its endpoints. In this mode, some endpoints (like /me
   * and /tokens) are unavailable.
   */
  SingleUser
};

/**
 * @brief Data retrieved from the Cesium ion server via an "appData" request
 * from Cesium ion. This actually represents information about the server
 * itself.
 */
struct ApplicationData {
  /**
   * The authentication mode that the ion server is running in.
   */
  AuthenticationMode authenticationMode;

  /**
   * The type of store used by this ion server to hold files.
   * Known values: FILE_SYSTEM, S3
   */
  std::string dataStoreType;

  /**
   * The attribution HTML for this ion server.
   */
  std::string attribution;

  /**
   * @brief Does the `authenticationMode` require OAuth authentication?
   */
  bool needsOauthAuthentication() const {
    return this->authenticationMode != AuthenticationMode::SingleUser;
  }
};

} // namespace CesiumIonClient
