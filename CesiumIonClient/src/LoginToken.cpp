#include <CesiumClientCommon/JwtTokenUtility.h>
#include <CesiumIonClient/LoginToken.h>
#include <CesiumUtility/ErrorList.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Result.h>

#include <rapidjson/document.h>

#include <chrono>
#include <cstdint>
#include <string>
#include <utility>

using namespace CesiumUtility;

namespace CesiumIonClient {
CesiumUtility::Result<LoginToken>
LoginToken::parse(const std::string& tokenString) {
  CesiumUtility::Result<rapidjson::Document> payloadResult =
      CesiumClientCommon::JwtTokenUtility::parseTokenPayload(tokenString);
  if (!payloadResult.value) {
    return Result<LoginToken>(std::move(payloadResult.errors));
  }

  rapidjson::Document& json = *payloadResult.value;
  int64_t expires = JsonHelpers::getInt64OrDefault(json, "exp", -1);

  return Result<LoginToken>(LoginToken(tokenString, expires));
}

bool LoginToken::isValid() const {
  if (this->_expires == -1) {
    // If the value is -1, that suggests we have no expiration date, so the
    // token is always valid.
    return true;
  }

  const int64_t currentTimeSinceEpoch =
      std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count();
  return currentTimeSinceEpoch < this->_expires;
}

LoginToken::LoginToken(const std::string& token, int64_t expires)
    : _token(token), _expires(expires) {}

int64_t LoginToken::getExpirationTime() const { return this->_expires; }

const std::string& LoginToken::getToken() const { return this->_token; }
} // namespace CesiumIonClient