#pragma once

#include <CesiumIonClient/Library.h>
#include <CesiumUtility/Result.h>

#include <cstdint>
#include <ctime>
#include <optional>
#include <string>

namespace CesiumIonClient {

/**
 * @brief A login token for interacting with the Cesium ion REST API, obtained
 * from the OAuth authentication flow.
 *
 * These tokens have a limited duration and are scoped to a user's account. This
 * is distinct from a Cesium ion \ref Token, which is valid until revoked by the
 * user and is scoped to specific assets and endpoints.
 */
class CESIUMIONCLIENT_API LoginToken {
public:
  /**
   * @brief Creates a new \ref LoginToken by parsing the provided JWT
   * authentication token.
   *
   * @param tokenString The JWT authentication token.
   * @returns A `Result` containing either the parsed \ref LoginToken
   * or error messages.
   */
  static CesiumUtility::Result<LoginToken>
  parse(const std::string& tokenString);

  /**
   * @brief Returns whether this token is currently valid.
   *
   * The token is valid up until its expiration time. If the token does not have
   * an expiration time, this method returns true.
   */
  bool isValid() const;

  /**
   * @brief Returns the time that this token expires, represented as
   * a number of seconds since the Unix epoch.
   *
   * If the token does not expire, this method returns `std::nullopt`.
   */
  std::optional<std::time_t> getExpirationTime() const;

  /**
   * @brief Returns the contained token string.
   */
  const std::string& getToken() const;

  /**
   * @brief Creates a new `LoginToken`.
   *
   * @param token The full token string.
   * @param expirationTime A UNIX timestamp representing the point in time that
   * this token stops being valid. If this parameter is `std::nullopt`, the
   * token is assumed to never expire.
   */
  LoginToken(
      const std::string& token,
      const std::optional<std::time_t>& expirationTime);

private:
  std::string _token;
  std::optional<std::time_t> _expirationTime;
};
} // namespace CesiumIonClient