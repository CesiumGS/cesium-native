#include <CesiumClientCommon/JwtTokenUtility.h>
#include <CesiumITwinClient/AuthenticationToken.h>
#include <CesiumUtility/ErrorList.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Result.h>

#include <fmt/format.h>
#include <modp_b64.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <chrono>
#include <cstdint>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace CesiumUtility;

namespace CesiumITwinClient {
Result<AuthenticationToken>
AuthenticationToken::parse(const std::string& tokenStr) {
  CesiumUtility::Result<rapidjson::Document> payloadResult =
      CesiumClientCommon::JwtTokenUtility::parseTokenPayload(tokenStr);
  if (!payloadResult.value) {
    return Result<AuthenticationToken>(std::move(payloadResult.errors));
  }

  rapidjson::Document& json = *payloadResult.value;

  int64_t expired = JsonHelpers::getInt64OrDefault(json, "exp", 0);

  const auto& iTwinIdValue = json.FindMember("iTwinId");
  if (iTwinIdValue != json.MemberEnd()) {
    return Result<AuthenticationToken>(AuthenticationToken(
        tokenStr,
        iTwinIdValue->value.GetString(),
        expired));
  }

  std::string name = JsonHelpers::getStringOrDefault(json, "name", "");
  std::string userName =
      JsonHelpers::getStringOrDefault(json, "preferred_username", "");
  std::vector<std::string> scopes = JsonHelpers::getStrings(json, "scope");
  int64_t notValidBefore = JsonHelpers::getInt64OrDefault(json, "nbf", 0);

  return Result<AuthenticationToken>(AuthenticationToken(
      tokenStr,
      std::move(name),
      std::move(userName),
      std::move(scopes),
      notValidBefore,
      expired));
}

bool AuthenticationToken::isValid() const {
  const int64_t currentTimeSinceEpoch =
      std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count();

  const AccessTokenContents* pAccessTokenContents =
      std::get_if<AccessTokenContents>(&this->_contents);
  if (pAccessTokenContents) {
    return currentTimeSinceEpoch >= pAccessTokenContents->notValidBefore &&
           currentTimeSinceEpoch < this->_expires;
  }

  return currentTimeSinceEpoch < this->_expires;
}

std::string AuthenticationToken::getTokenHeader() const {
  struct TokenHeaderVisitor {
    const std::string& tokenStr;
    std::string operator()(const AccessTokenContents&) {
      return fmt::format("Bearer {}", tokenStr);
    }
    std::string operator()(const std::string&) {
      return fmt::format("Basic {}", tokenStr);
    }
  };

  return std::visit(TokenHeaderVisitor{this->_token}, this->_contents);
}
AuthenticationToken::AuthenticationToken(
    const std::string& token,
    std::string&& name,
    std::string&& userName,
    std::vector<std::string>&& scopes,
    int64_t notValidBefore,
    int64_t expires)
    : _token(token),
      _contents(AccessTokenContents{
          std::move(name),
          std::move(userName),
          std::move(scopes),
          notValidBefore}),
      _expires(expires) {}
} // namespace CesiumITwinClient