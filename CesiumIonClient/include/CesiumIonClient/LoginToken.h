#pragma once

#include <CesiumIonClient/Library.h>
#include <CesiumUtility/Result.h>

#include <cstdint>
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
   * @brief Is this token currently valid?
   *
   * For the token to be valid, it must be before its expiration date.
   */
  bool isValid() const;

  /**
   * @brief Returns the number of seconds since the Unix epoch representing the
   * time that this token expires.
   */
  int64_t getExpirationTime() const;

  /**
   * @brief Returns the contained token string.
   */
  const std::string& getToken() const;

  /**
   * @brief Creates a new `LoginToken`.
   *
   * @param token The full token string.
   * @param expires A UNIX timestamp representing the point in time that this
   * token stops being valid.
   */
  LoginToken(const std::string& token, int64_t expires);

private:
  std::string _token;
  int64_t _expires;
};
} // namespace CesiumIonClient