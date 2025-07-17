#pragma once

#include "Library.h"

#include <CesiumUtility/Result.h>

#include <cstdint>
#include <string>
#include <variant>
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
   * @brief Returns the HTTP Authorization header for this token.
   *
   * Access tokens use a `Bearer` header while share tokens use a `Basic`
   * header.
   */
  std::string getTokenHeader() const;

  /**
   * @brief Creates a new `AuthenticationToken` for an access token.
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
      int64_t expires);

  /**
   * @brief Creates a new `AuthenticationToken` for a share token.
   *
   * This constructor assumes all the data in the provided token has already
   * been parsed. If not, you should call \ref parse instead.
   *
   * @param token The full token string.
   * @param iTwinId The ID of the iTwin this share token provides access to.
   * @param expires A UNIX timestamp representing the point in time that this
   * token stops being valid.
   */
  AuthenticationToken(
      const std::string& token,
      std::string&& iTwinId,
      int64_t expires)
      : _token(token), _contents(std::move(iTwinId)), _expires(expires) {}

private:
  /**
   * @brief The contents of a user's access token.
   */
  struct AccessTokenContents {
    /** @brief The name of this token. */
    std::string name;
    /** @brief The name of the user this token belongs to. */
    std::string userName;
    /** @brief The list of scopes this token is valid for. */
    std::vector<std::string> scopes;
    /** @brief The timestamp this token is not valid before. */
    int64_t notValidBefore;
  };

  /**
   * @brief The possible contents of an authentication token.
   *
   * An access token contains information about the user that produced it and
   * the scope of access. A share token only contains the iTwin ID that it is
   * for.
   */
  using AuthenticationTokenContents =
      std::variant<AccessTokenContents, std::string>;

  std::string _token;
  AuthenticationTokenContents _contents;
  int64_t _expires;
};
} // namespace CesiumITwinClient