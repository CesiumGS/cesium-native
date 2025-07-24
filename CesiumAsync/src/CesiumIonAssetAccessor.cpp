#include <CesiumAsync/CesiumIonAssetAccessor.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Log.h>

#include <rapidjson/document.h>

namespace CesiumAsync {

CesiumIonAssetAccessor::CesiumIonAssetAccessor(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::shared_ptr<IAssetAccessor>& pAggregatedAccessor,
    const std::string& assetEndpointUrl,
    std::function<void(const UpdatedToken&)> updatedTokenCallback)
    : _pLogger(pLogger),
      _pAggregatedAccessor(pAggregatedAccessor),
      _assetEndpointUrl(assetEndpointUrl),
      _updatedTokenCallback(std::move(updatedTokenCallback)),
      _tokenRefreshInProgress() {}

CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
CesiumIonAssetAccessor::get(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& url,
    const std::vector<THeader>& headers) {
  // If token refresh is needed, this lambda will be called in the main thread
  // so that it can safely use the tileset loader.
  auto refreshToken = [pThis = this->shared_from_this()](
                          const CesiumAsync::AsyncSystem& asyncSystem,
                          std::shared_ptr<CesiumAsync::IAssetRequest>&&
                              pRequest) {
    if (!pThis->_updatedTokenCallback) {
      // The owner has been destroyed, so just return the
      // original (failed) request.
      return asyncSystem.createResolvedFuture(std::move(pRequest));
    }

    const CesiumAsync::HttpHeaders& headers = pRequest->headers();
    auto authIt = headers.find("authorization");
    std::string currentAuthorizationHeaderValue =
        authIt != headers.end() ? authIt->second : std::string();

    return pThis
        ->refreshTokenInMainThread(asyncSystem, currentAuthorizationHeaderValue)
        .thenImmediately(
            [pThis, asyncSystem, pRequest = std::move(pRequest)](
                const std::string& newAuthorizationHeader) mutable {
              if (newAuthorizationHeader.empty()) {
                // Could not refresh the token, so just return the
                // original (failed) request.
                return asyncSystem.createResolvedFuture(std::move(pRequest));
              }

              // Repeat the request using the new token.
              CesiumAsync::HttpHeaders headers = pRequest->headers();
              headers["Authorization"] = newAuthorizationHeader;
              std::vector<THeader> vecHeaders(
                  std::make_move_iterator(headers.begin()),
                  std::make_move_iterator(headers.end()));
              return pThis->get(asyncSystem, pRequest->url(), vecHeaders);
            });
  };

  return this->_pAggregatedAccessor->get(asyncSystem, url, headers)
      .thenImmediately(
          [asyncSystem, refreshToken = std::move(refreshToken)](
              std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) mutable {
            const CesiumAsync::IAssetResponse* pResponse = pRequest->response();
            if (!pResponse) {
              return asyncSystem.createResolvedFuture(std::move(pRequest));
            }

            if (pResponse->statusCode() == 401) {
              // We need to refresh the Cesium ion token.
              return asyncSystem.runInMainThread(
                  [asyncSystem,
                   pRequest = std::move(pRequest),
                   refreshToken = std::move(refreshToken)]() mutable {
                    return refreshToken(asyncSystem, std::move(pRequest));
                  });
            }

            return asyncSystem.createResolvedFuture(std::move(pRequest));
          });
}

CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
CesiumIonAssetAccessor::request(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& verb,
    const std::string& url,
    const std::vector<THeader>& headers,
    const std::span<const std::byte>& contentPayload) {
  return this->_pAggregatedAccessor
      ->request(asyncSystem, verb, url, headers, contentPayload);
}

void CesiumIonAssetAccessor::tick() noexcept {
  this->_pAggregatedAccessor->tick();
}

void CesiumIonAssetAccessor::notifyOwnerIsBeingDestroyed() {
  this->_updatedTokenCallback.reset();
}

namespace {
/**
 * @brief Tries to obtain the `accessToken` from the JSON of the
 * given response.
 *
 * @param pIonResponse The response
 * @return The access token if successful
 */
std::optional<std::string> getNewAccessToken(
    const CesiumAsync::IAssetResponse* pIonResponse,
    const std::shared_ptr<spdlog::logger>& pLogger) {
  const std::span<const std::byte> data = pIonResponse->data();
  rapidjson::Document ionResponse;
  ionResponse.Parse(reinterpret_cast<const char*>(data.data()), data.size());
  if (ionResponse.HasParseError()) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "A JSON parsing error occurred while attempting to refresh the Cesium "
        "ion token. Error code {} at byte offset {}.",
        ionResponse.GetParseError(),
        ionResponse.GetErrorOffset());
    return std::nullopt;
  }

  std::string accessToken = CesiumUtility::JsonHelpers::getStringOrDefault(
      ionResponse,
      "accessToken",
      "");
  if (accessToken.empty()) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "Could not refresh Cesium ion token because the `accessToken` field in "
        "the JSON response is missing or blank.");
    return std::nullopt;
  }

  return accessToken;
}
} // namespace

CesiumAsync::SharedFuture<std::string>
CesiumIonAssetAccessor::refreshTokenInMainThread(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& currentAuthorizationHeaderValue) {
  if (this->_tokenRefreshInProgress) {
    if (!this->_tokenRefreshInProgress->isReady() ||
        this->_tokenRefreshInProgress->wait() !=
            currentAuthorizationHeaderValue) {
      // Only use this refreshed token if it's different from the one we're
      // currently using. Otherwise, fall through and get a new token.
      return *this->_tokenRefreshInProgress;
    }
  }

  SPDLOG_LOGGER_INFO(
      this->_pLogger,
      "Refreshing Cesium ion token for url {}.",
      this->_assetEndpointUrl);

  this->_tokenRefreshInProgress =
      this->_pAggregatedAccessor->get(asyncSystem, this->_assetEndpointUrl)
          .thenInMainThread(
              [this](
                  std::shared_ptr<CesiumAsync::IAssetRequest>&& pIonRequest) {
                if (!this->_updatedTokenCallback) {
                  // Owner is already destroyed.
                  return std::string();
                }

                const CesiumAsync::IAssetResponse* pIonResponse =
                    pIonRequest->response();

                if (!pIonResponse) {
                  // Token refresh failed.
                  SPDLOG_LOGGER_ERROR(
                      this->_pLogger,
                      "Request failed while attempting to refresh the Cesium "
                      "ion token.");
                  return std::string();
                }

                uint16_t statusCode = pIonResponse->statusCode();
                if (statusCode >= 200 && statusCode < 300) {
                  auto accessToken =
                      getNewAccessToken(pIonResponse, this->_pLogger);
                  if (accessToken) {
                    std::string authorizationHeader = "Bearer " + *accessToken;
                    this->_updatedTokenCallback.value()(
                        UpdatedToken{*accessToken, authorizationHeader});

                    SPDLOG_LOGGER_INFO(
                        this->_pLogger,
                        "Successfuly refreshed Cesium ion token for url {}.",
                        this->_assetEndpointUrl);

                    return authorizationHeader;
                  } else {
                    // This error is logged from within `getNewAccessToken`.
                    return std::string();
                  }
                } else {
                  // Token refresh failed.
                  SPDLOG_LOGGER_ERROR(
                      this->_pLogger,
                      "Request failed with status code {} while attempting to "
                      "refresh the Cesium ion token.",
                      statusCode);
                  return std::string();
                }
              })
          .share();

  return *this->_tokenRefreshInProgress;
}

} // namespace CesiumAsync
