#include <CesiumClientCommon/JwtTokenUtility.h>
#include <CesiumIonClient/LoginToken.h>
#include <CesiumUtility/ErrorList.h>
#include <CesiumUtility/Result.h>

#include <rapidjson/document.h>

#include <chrono>
#include <cstdint>
#include <ctime>
#include <optional>
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

  const auto it = json.FindMember("exp");
  std::optional<std::time_t> expirationTime =
      it != json.MemberEnd() && it->value.IsInt64()
          ? std::make_optional<std::time_t>(std::time_t(it->value.GetInt64()))
          : std::nullopt;

  if (expirationTime == -1) {
    expirationTime = std::nullopt;
  }

  return Result<LoginToken>(LoginToken(tokenString, expirationTime));
}

bool LoginToken::isValid() const {
  if (!this->_expirationTime) {
    return true;
  }

  const int64_t currentTimeSinceEpoch =
      std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count();
  return currentTimeSinceEpoch < *this->_expirationTime;
}

LoginToken::LoginToken(
    const std::string& token,
    const std::optional<std::time_t>& expirationTime)
    : _token(token), _expirationTime(expirationTime) {}

std::optional<std::time_t> LoginToken::getExpirationTime() const {
  return this->_expirationTime;
}

const std::string& LoginToken::getToken() const { return this->_token; }
} // namespace CesiumIonClient