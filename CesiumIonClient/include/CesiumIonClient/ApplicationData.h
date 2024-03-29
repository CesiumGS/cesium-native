#pragma once

#include <cstdint>
#include <string>

namespace CesiumIonClient {

/**
 *  @brief An enumeration representing the values of {@link ApplicationData::applicationMode}.
 */
enum ApplicationMode { CesiumIon, Saml, SingleUser };

/**
 * @brief Data about the Cesium ion server itself.
 */
struct ApplicationData {
  /**
   * The mode that the Ion server is running in.
   * This value can be cesium-ion, saml, or single-user.
   */
  ApplicationMode applicationMode;

  /**
   * The type of store used by this ion server to hold files.
   * Known values: FILE_SYSTEM, S3
   */
  std::string dataStoreType;

  /**
   * The attribution HTML for this ion server.
   */
  std::string attribution;

  bool needsOauthAuthentication() {
    return this->applicationMode != ApplicationMode::SingleUser;
  }
};

} // namespace CesiumIonClient
