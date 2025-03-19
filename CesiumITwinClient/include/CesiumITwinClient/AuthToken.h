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
class CESIUMITWINCLIENT_API AuthToken {
public:
  static CesiumUtility::Result<AuthToken> parse(const std::string& tokenStr);

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

  const std::string& getToken() const { return _token; }

  AuthToken(
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