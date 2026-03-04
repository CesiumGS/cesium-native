#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/CesiumIonAssetAccessor.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/HttpHeaders.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumAsync/SharedFuture.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Log.h> // NOLINT
#include <CesiumUtility/Uri.h>

#include <rapidjson/document.h>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace CesiumAsync {

CesiumIonAssetAccessor::CesiumIonAssetAccessor(
    const std::shared_ptr<spdlog::logger>& pLogger,
    const std::shared_ptr<IAssetAccessor>& pAggregatedAccessor,
    const std::string& assetEndpointUrl,
    const std::vector<IAssetAccessor::THeader>& assetEndpointHeaders,
    std::function<Future<void>(const UpdatedToken&)> updatedTokenCallback)
    : _pLogger(pLogger),
      _pAggregatedAccessor(pAggregatedAccessor),
      _assetEndpointUrl(assetEndpointUrl),
      _assetEndpointHeaders(assetEndpointHeaders),
      _maybeUpdatedTokenCallback(std::move(updatedTokenCallback)),
      _tokenRefreshInProgress() {}

CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
CesiumIonAssetAccessor::get(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& url,
    const std::vector<THeader>& headers) {
  // If token refresh is needed, this lambda will be called in the main thread
  // so that it can safely use the tileset loader.
  auto refreshToken =
      [pThis = this->shared_from_this()](
          const CesiumAsync::AsyncSystem& asyncSystem,
          std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
        if (!pThis->_maybeUpdatedTokenCallback) {
          // The owner has been destroyed, so just return the
          // original (failed) request.
          return asyncSystem.createResolvedFuture(std::move(pRequest));
        }

        const CesiumAsync::HttpHeaders& headers = pRequest->headers();
        auto authIt = headers.find("authorization");
        std::string currentAuthorizationHeaderValue =
            authIt != headers.end() ? authIt->second : std::string();

        std::string currentAccessTokenQueryParameterValue;
        if (pRequest->url().find("access_token") != std::string::npos) {
          CesiumUtility::Uri uri(pRequest->url());
          CesiumUtility::UriQuery query(uri);
          std::optional<std::string_view> maybeToken =
              query.getValue("access_token");
          if (maybeToken) {
            currentAccessTokenQueryParameterValue = *maybeToken;
          }
        }

        return pThis
            ->refreshTokenInMainThread(
                asyncSystem,
                currentAuthorizationHeaderValue,
                currentAccessTokenQueryParameterValue)
            .thenImmediately([pThis,
                              asyncSystem,
                              pRequest = std::move(pRequest)](
                                 const UpdatedToken& updatedToken) mutable {
              if (updatedToken.authorizationHeader.empty() &&
                  updatedToken.token.empty()) {
                // Could not refresh the token, so just return the
                // original (failed) request.
                return asyncSystem.createResolvedFuture(std::move(pRequest));
              }

              // Repeat the request using the new token.
              CesiumAsync::HttpHeaders headers = pRequest->headers();
              if (!updatedToken.authorizationHeader.empty()) {
                headers["Authorization"] = updatedToken.authorizationHeader;
              }

              std::string url = pRequest->url();
              if (pRequest->url().find("access_token") != std::string::npos) {
                CesiumUtility::Uri uri(pRequest->url());
                CesiumUtility::UriQuery query(uri);
                std::optional<std::string_view> maybeToken =
                    query.getValue("access_token");
                if (maybeToken) {
                  query.setValue("access_token", updatedToken.token);
                  uri.setQuery(query.toQueryString());
                  url = uri.toString();
                }
              }

              std::vector<THeader> vecHeaders(
                  std::make_move_iterator(headers.begin()),
                  std::make_move_iterator(headers.end()));
              return pThis->get(asyncSystem, url, vecHeaders);
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
  this->_maybeUpdatedTokenCallback.reset();
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
    // Some assets, like Bing Maps, put the token in the "options" -> "key"
    // field.
    auto optionsIt = ionResponse.FindMember("options");
    if (optionsIt != ionResponse.MemberEnd() && optionsIt->value.IsObject()) {
      accessToken = CesiumUtility::JsonHelpers::getStringOrDefault(
          optionsIt->value,
          "key",
          "");
    }
  }

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

CesiumAsync::SharedFuture<CesiumIonAssetAccessor::UpdatedToken>
CesiumIonAssetAccessor::refreshTokenInMainThread(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& currentAuthorizationHeaderValue,
    const std::string& currentAccessTokenQueryParameterValue) {
  if (this->_tokenRefreshInProgress) {
    if (!this->_tokenRefreshInProgress->isReady()) {
      // Still waiting for an in-progress token refresh.
      return *this->_tokenRefreshInProgress;
    }

    const UpdatedToken& refreshedToken = this->_tokenRefreshInProgress->wait();
    bool hasHeader = !currentAuthorizationHeaderValue.empty();
    bool hasParameter = !currentAccessTokenQueryParameterValue.empty();
    bool headerChanged = hasHeader && refreshedToken.authorizationHeader !=
                                          currentAuthorizationHeaderValue;
    bool parameterChanged =
        hasParameter &&
        refreshedToken.token != currentAccessTokenQueryParameterValue;
    if (headerChanged || parameterChanged) {
      // Only use this refreshed token if it's different from the one we're
      // currently using. Otherwise, fall through and get a new token.
      return *this->_tokenRefreshInProgress;
    }
  }

  SPDLOG_LOGGER_INFO(
      this->_pLogger,
      "Refreshing Cesium ion token for URL {}.",
      this->_assetEndpointUrl);

  this->_tokenRefreshInProgress =
      this->_pAggregatedAccessor
          ->get(
              asyncSystem,
              this->_assetEndpointUrl,
              this->_assetEndpointHeaders)
          .thenInMainThread([this, asyncSystem](
                                std::shared_ptr<CesiumAsync::IAssetRequest>&&
                                    pIonRequest) {
            if (!this->_maybeUpdatedTokenCallback) {
              // Owner is already destroyed.
              return asyncSystem.createResolvedFuture(UpdatedToken());
            }

            const CesiumAsync::IAssetResponse* pIonResponse =
                pIonRequest->response();

            if (!pIonResponse) {
              // Token refresh failed.
              SPDLOG_LOGGER_ERROR(
                  this->_pLogger,
                  "Request failed while attempting to refresh the Cesium "
                  "ion token.");
              return asyncSystem.createResolvedFuture(UpdatedToken());
            }

            uint16_t statusCode = pIonResponse->statusCode();
            if (statusCode >= 200 && statusCode < 300) {
              std::optional<std::string> maybeAccessToken =
                  getNewAccessToken(pIonResponse, this->_pLogger);
              if (maybeAccessToken) {
                std::string authorizationHeader = "Bearer " + *maybeAccessToken;
                UpdatedToken update{*maybeAccessToken, authorizationHeader};
                return (*this->_maybeUpdatedTokenCallback)(update)
                    .thenImmediately([update,
                                      pLogger = this->_pLogger,
                                      url = this->_assetEndpointUrl]() {
                      SPDLOG_LOGGER_INFO(
                          pLogger,
                          "Successfully refreshed Cesium ion token for URL {}.",
                          url);

                      return update;
                    });
              } else {
                // This error is logged from within `getNewAccessToken`.
                return asyncSystem.createResolvedFuture(UpdatedToken());
              }
            } else {
              // Token refresh failed.
              SPDLOG_LOGGER_ERROR(
                  this->_pLogger,
                  "Request failed with status code {} while attempting to "
                  "refresh the Cesium ion token.",
                  statusCode);
              return asyncSystem.createResolvedFuture(UpdatedToken());
            }
          })
          .share();

  return *this->_tokenRefreshInProgress;
}

} // namespace CesiumAsync
