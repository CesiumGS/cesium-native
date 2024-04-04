#pragma once

#include <cstdint>
#include <string>

namespace CesiumIonClient {

/**
 *  @brief An enumeration representing the values of {@link ApplicationData::applicationMode}.
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
 * @brief Data about the Cesium ion server itself.
 */
struct ApplicationData {
  /**
   * The authentication mode that the Ion server is running in.
   */
  AuthenticationMode applicationMode;

  /**
   * The type of store used by this ion server to hold files.
   * Known values: FILE_SYSTEM, S3
   */
  std::string dataStoreType;

  /**
   * The attribution HTML for this ion server.
   */
  std::string attribution;

  bool needsOauthAuthentication() const {
    return this->applicationMode != AuthenticationMode::SingleUser;
  }
};

} // namespace CesiumIonClient
