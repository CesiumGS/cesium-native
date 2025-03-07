#pragma once

#include "Library.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumUtility/Result.h>

#include <optional>
#include <string>

namespace CesiumClientCommon {

struct OAuth2TokenResponse {
  std::string accessToken;
  std::optional<std::string> refreshToken;
};

class CESIUMCLIENTCOMMON_API OAuth2PKE {
public:
  static CesiumAsync::Future<CesiumUtility::Result<OAuth2TokenResponse>> authorize(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::string& friendlyApplicationName,
      const std::string& clientID,
      const std::string& redirectPath,
      const std::vector<std::string>& scopes,
      std::function<void(const std::string&)>&& openUrlCallback,
      const std::string& tokenEndpointUrl,
      const std::string& authorizeBaseUrl);

  static CesiumAsync::Future<CesiumUtility::Result<OAuth2TokenResponse>>
  completeTokenExchange(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::string& clientID,
      const std::string& tokenEndpointUrl,
      const std::string& code,
      const std::string& redirectUrl,
      const std::string& codeVerifier);
};

} // namespace CesiumClientCommon