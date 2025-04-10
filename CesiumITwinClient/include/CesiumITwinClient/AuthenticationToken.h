#pragma once

#include "Library.h"

#include <CesiumUtility/Result.h>

#include <cstdint>
#include <string>
#include <vector>

namespace CesiumITwinClient {
/**
 * @brief An authentication token obtained from the iTwin OAuth2 flow.
 */
class CESIUMITWINCLIENT_API AuthenticationToken {
public:
  /**
   * @brief Creates a new \ref AuthenticationToken by parsing the provided JWT
   * authentication token.
   *
   * @param tokenStr The JWT authentication token.
   * @returns A `Result` containing either the parsed \ref AuthenticationToken
   * or error messages.
   */
  static CesiumUtility::Result<AuthenticationToken>
  parse(const std::string& tokenStr);

  /**
   * @brief Is this token currently valid?
   *
   * For the token to be valid, it must currently be after its "not valid
   * before" date but before its expiration date.
   */
  bool isValid() const;

  /**
   * @brief Returns the number of seconds since the Unix epoch representing the
   * time that this token expires.
   */
  int64_t getExpirationTime() const { return _expires; }

  /**
   * @brief Returns the contained token string.
   */
  const std::string& getToken() const { return _token; }

  /**
   * @brief Creates a new `AuthenticationToken`.
   *
   * This constructor assumes all the data in the provided token has already
   * been parsed. If not, you should call \ref parse instead.
   *
   * @param token The full token string.
   * @param name The name of the token.
   * @param userName The name of the user this token belongs to.
   * @param scopes The set of scopes this token is valid for.
   * @param notValidBefore A UNIX timestamp representing the point in time that
   * this token starts to be valid.
   * @param expires A UNIX timestamp representing the point in time that this
   * token stops being valid.
   */
  AuthenticationToken(
      const std::string& token,
      std::string&& name,
      std::string&& userName,
      std::vector<std::string>&& scopes,
      int64_t notValidBefore,
      int64_t expires)
      : _token(token),
        _name(std::move(name)),
        _userName(std::move(userName)),
        _scopes(std::move(scopes)),
        _notValidBefore(notValidBefore),
        _expires(expires) {}

private:
  std::string _token;
  std::string _name;
  std::string _userName;
  std::vector<std::string> _scopes;
  int64_t _notValidBefore;
  int64_t _expires;
};
} // namespace CesiumITwinClient