#include "CesiumIonTilesetLoader.h"

#include "LayerJsonTerrainLoader.h"
#include "TilesetContentLoaderResult.h"
#include "TilesetJsonLoader.h"

#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileLoadResult.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/TilesetOptions.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/HttpHeaders.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumAsync/SharedFuture.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Uri.h>

#include <fmt/format.h>
#include <rapidjson/document.h>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Cesium3DTilesSelection {

// This is an IAssetAccessor decorator that handles token refresh for any asset
// that returns a 401 error.
class CesiumIonAssetAccessor
    : public std::enable_shared_from_this<CesiumIonAssetAccessor>,
      public CesiumAsync::IAssetAccessor {
public:
  CesiumIonAssetAccessor(
      CesiumIonTilesetLoader& tilesetLoader,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAggregatedAccessor)
      : _pTilesetLoader(&tilesetLoader),
        _pAggregatedAccessor(pAggregatedAccessor) {}

  CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
  get(const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& url,
      const std::vector<THeader>& headers = {}) override {
    // If token refresh is needed, this lambda will be called in the main thread
    // so that it can safely use the tileset loader.
    auto refreshToken =
        [pThis = this->shared_from_this()](
            const CesiumAsync::AsyncSystem& asyncSystem,
            std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
          if (!pThis->_pTilesetLoader) {
            // The tileset loader has been destroyed, so just return the
            // original (failed) request.
            return asyncSystem.createResolvedFuture(std::move(pRequest));
          }

          const CesiumAsync::HttpHeaders& headers = pRequest->headers();
          auto authIt = headers.find("authorization");
          std::string currentAuthorizationHeaderValue =
              authIt != headers.end() ? authIt->second : std::string();

          return pThis->_pTilesetLoader
              ->refreshTokenInMainThread(
                  asyncSystem,
                  currentAuthorizationHeaderValue)
              .thenImmediately(
                  [pThis, asyncSystem, pRequest = std::move(pRequest)](
                      const std::string& newAuthorizationHeader) mutable {
                    if (newAuthorizationHeader.empty()) {
                      // Could not refresh the token, so just return the
                      // original (failed) request.
                      return asyncSystem.createResolvedFuture(
                          std::move(pRequest));
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
        .thenImmediately([asyncSystem, refreshToken = std::move(refreshToken)](
                             std::shared_ptr<CesiumAsync::IAssetRequest>&&
                                 pRequest) mutable {
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

  CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>> request(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& verb,
      const std::string& url,
      const std::vector<THeader>& headers = std::vector<THeader>(),
      const std::span<const std::byte>& contentPayload = {}) override {
    return this->_pAggregatedAccessor
        ->request(asyncSystem, verb, url, headers, contentPayload);
  }

  void tick() noexcept override { this->_pAggregatedAccessor->tick(); }

  void notifyLoaderIsBeingDestroyed() { this->_pTilesetLoader = nullptr; }

private:
  CesiumIonTilesetLoader* _pTilesetLoader;
  std::shared_ptr<CesiumAsync::IAssetAccessor> _pAggregatedAccessor;
};

namespace {
struct AssetEndpointAttribution {
  std::string html;
  bool collapsible = true;
};

struct AssetEndpoint {
  std::string type;
  std::string url;
  std::string accessToken;
  std::vector<AssetEndpointAttribution> attributions;
};

std::unordered_map<std::string, AssetEndpoint> endpointCache;

std::string createEndpointResource(
    int64_t ionAssetID,
    const std::string& ionAccessToken,
    const std::string& ionAssetEndpointUrl) {
  std::string ionUrl = fmt::format(
      "{}v1/assets/{}/endpoint?access_token={}",
      ionAssetEndpointUrl,
      ionAssetID,
      ionAccessToken);
  return ionUrl;
}

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

CesiumAsync::Future<TilesetContentLoaderResult<CesiumIonTilesetLoader>>
mainThreadLoadTilesetJsonFromAssetEndpoint(
    const TilesetExternals& externals,
    const AssetEndpoint& endpoint,
    int64_t ionAssetID,
    std::string ionAccessToken,
    std::string ionAssetEndpointUrl,
    CesiumIonTilesetLoader::AuthorizationHeaderChangeListener
        headerChangeListener,
    bool showCreditsOnScreen,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  std::vector<LoaderCreditResult> credits;
  if (externals.pCreditSystem) {
    credits.reserve(endpoint.attributions.size());
    for (auto& endpointAttribution : endpoint.attributions) {
      bool showOnScreen =
          showCreditsOnScreen || !endpointAttribution.collapsible;
      credits.push_back(
          LoaderCreditResult{endpointAttribution.html, showOnScreen});
    }
  }

  std::vector<CesiumAsync::IAssetAccessor::THeader> requestHeaders;
  requestHeaders.emplace_back(
      "Authorization",
      "Bearer " + endpoint.accessToken);

  return TilesetJsonLoader::createLoader(
             externals,
             endpoint.url,
             requestHeaders,
             ellipsoid)
      .thenImmediately(
          [credits = std::move(credits),
           requestHeaders,
           ionAssetID,
           ionAccessToken = std::move(ionAccessToken),
           ionAssetEndpointUrl = std::move(ionAssetEndpointUrl),
           headerChangeListener = std::move(headerChangeListener),
           ellipsoid](TilesetContentLoaderResult<TilesetJsonLoader>&&
                          tilesetJsonResult) mutable {
            if (tilesetJsonResult.credits.empty()) {
              tilesetJsonResult.credits = std::move(credits);
            } else {
              tilesetJsonResult.credits.insert(
                  tilesetJsonResult.credits.end(),
                  credits.begin(),
                  credits.end());
            }

            TilesetContentLoaderResult<CesiumIonTilesetLoader> result;
            if (!tilesetJsonResult.errors) {
              result.pLoader = std::make_unique<CesiumIonTilesetLoader>(
                  ionAssetID,
                  std::move(ionAccessToken),
                  std::move(ionAssetEndpointUrl),
                  std::move(tilesetJsonResult.pLoader),
                  std::move(headerChangeListener),
                  ellipsoid);
              result.pRootTile = std::move(tilesetJsonResult.pRootTile);
              result.credits = std::move(tilesetJsonResult.credits);
              result.requestHeaders = std::move(requestHeaders);
            }
            result.errors = std::move(tilesetJsonResult.errors);
            result.statusCode = tilesetJsonResult.statusCode;
            return result;
          });
}

CesiumAsync::Future<TilesetContentLoaderResult<CesiumIonTilesetLoader>>
mainThreadLoadLayerJsonFromAssetEndpoint(
    const TilesetExternals& externals,
    const TilesetContentOptions& contentOptions,
    const AssetEndpoint& endpoint,
    int64_t ionAssetID,
    std::string ionAccessToken,
    std::string ionAssetEndpointUrl,
    CesiumIonTilesetLoader::AuthorizationHeaderChangeListener
        headerChangeListener,
    bool showCreditsOnScreen,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  std::vector<LoaderCreditResult> credits;
  if (externals.pCreditSystem) {
    credits.reserve(endpoint.attributions.size());
    for (auto& endpointAttribution : endpoint.attributions) {
      bool showOnScreen =
          showCreditsOnScreen || !endpointAttribution.collapsible;
      credits.push_back(
          LoaderCreditResult{endpointAttribution.html, showOnScreen});
    }
  }

  std::vector<CesiumAsync::IAssetAccessor::THeader> requestHeaders;
  requestHeaders.emplace_back(
      "Authorization",
      "Bearer " + endpoint.accessToken);

  std::string url =
      CesiumUtility::Uri::resolve(endpoint.url, "layer.json", true);

  return LayerJsonTerrainLoader::createLoader(
             externals,
             contentOptions,
             url,
             requestHeaders,
             ellipsoid)
      .thenImmediately([ellipsoid,
                        credits = std::move(credits),
                        requestHeaders,
                        ionAssetID,
                        ionAccessToken = std::move(ionAccessToken),
                        ionAssetEndpointUrl = std::move(ionAssetEndpointUrl),
                        headerChangeListener = std::move(headerChangeListener)](
                           TilesetContentLoaderResult<LayerJsonTerrainLoader>&&
                               tilesetJsonResult) mutable {
        if (tilesetJsonResult.credits.empty()) {
          tilesetJsonResult.credits = std::move(credits);
        } else {
          tilesetJsonResult.credits.insert(
              tilesetJsonResult.credits.end(),
              credits.begin(),
              credits.end());
        }

        TilesetContentLoaderResult<CesiumIonTilesetLoader> result;
        if (!tilesetJsonResult.errors) {
          result.pLoader = std::make_unique<CesiumIonTilesetLoader>(
              ionAssetID,
              std::move(ionAccessToken),
              std::move(ionAssetEndpointUrl),
              std::move(tilesetJsonResult.pLoader),
              std::move(headerChangeListener),
              ellipsoid);
          result.pRootTile = std::move(tilesetJsonResult.pRootTile);
          result.credits = std::move(tilesetJsonResult.credits);
          result.requestHeaders = std::move(requestHeaders);
        }
        result.errors = std::move(tilesetJsonResult.errors);
        result.statusCode = tilesetJsonResult.statusCode;
        return result;
      });
}

CesiumAsync::Future<TilesetContentLoaderResult<CesiumIonTilesetLoader>>
mainThreadHandleEndpointResponse(
    const TilesetExternals& externals,
    std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest,
    int64_t ionAssetID,
    std::string&& ionAccessToken,
    std::string&& ionAssetEndpointUrl,
    const TilesetContentOptions& contentOptions,
    CesiumIonTilesetLoader::AuthorizationHeaderChangeListener&&
        headerChangeListener,
    bool showCreditsOnScreen,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  const CesiumAsync::IAssetResponse* pResponse = pRequest->response();
  const std::string& requestUrl = pRequest->url();
  if (!pResponse) {
    TilesetContentLoaderResult<CesiumIonTilesetLoader> result;
    result.errors.emplaceError(
        fmt::format("No response received for asset request {}", requestUrl));
    return externals.asyncSystem.createResolvedFuture(std::move(result));
  }

  uint16_t statusCode = pResponse->statusCode();
  if (statusCode < 200 || statusCode >= 300) {
    TilesetContentLoaderResult<CesiumIonTilesetLoader> result;
    result.errors.emplaceError(fmt::format(
        "Received status code {} for asset response {}",
        statusCode,
        requestUrl));
    result.statusCode = statusCode;
    return externals.asyncSystem.createResolvedFuture(std::move(result));
  }

  const std::span<const std::byte> data = pResponse->data();

  rapidjson::Document ionResponse;
  ionResponse.Parse(reinterpret_cast<const char*>(data.data()), data.size());

  if (ionResponse.HasParseError()) {
    TilesetContentLoaderResult<CesiumIonTilesetLoader> result;
    result.errors.emplaceError(fmt::format(
        "Error when parsing Cesium ion response JSON, error code {} at byte "
        "offset {}",
        ionResponse.GetParseError(),
        ionResponse.GetErrorOffset()));
    return externals.asyncSystem.createResolvedFuture(std::move(result));
  }

  AssetEndpoint endpoint;
  if (externals.pCreditSystem) {
    const auto attributionsIt = ionResponse.FindMember("attributions");
    if (attributionsIt != ionResponse.MemberEnd() &&
        attributionsIt->value.IsArray()) {

      for (const rapidjson::Value& attribution :
           attributionsIt->value.GetArray()) {
        AssetEndpointAttribution& endpointAttribution =
            endpoint.attributions.emplace_back();
        const auto html = attribution.FindMember("html");
        if (html != attribution.MemberEnd() && html->value.IsString()) {
          endpointAttribution.html = html->value.GetString();
        }
        auto collapsible = attribution.FindMember("collapsible");
        if (collapsible != attribution.MemberEnd() &&
            collapsible->value.IsBool()) {
          endpointAttribution.collapsible = collapsible->value.GetBool();
        }
      }
    }
  }

  std::string type =
      CesiumUtility::JsonHelpers::getStringOrDefault(ionResponse, "type", "");
  std::string url =
      CesiumUtility::JsonHelpers::getStringOrDefault(ionResponse, "url", "");
  std::string accessToken = CesiumUtility::JsonHelpers::getStringOrDefault(
      ionResponse,
      "accessToken",
      "");
  std::string externalType = CesiumUtility::JsonHelpers::getStringOrDefault(
      ionResponse,
      "externalType",
      "");

  if (!externalType.empty()) {
    type = externalType;
    const auto optionsIt = ionResponse.FindMember("options");
    if (optionsIt != ionResponse.MemberEnd() && optionsIt->value.IsObject()) {
      url = CesiumUtility::JsonHelpers::getStringOrDefault(
          optionsIt->value,
          "url",
          url);
    }
  }

  if (type == "TERRAIN") {
    // For terrain resources, we need to append `/layer.json` to the end of
    // the URL.
    url = CesiumUtility::Uri::resolve(url, "layer.json", true);
    endpoint.type = type;
    endpoint.url = url;
    endpoint.accessToken = accessToken;
    endpointCache[requestUrl] = endpoint;
    return mainThreadLoadLayerJsonFromAssetEndpoint(
        externals,
        contentOptions,
        endpoint,
        ionAssetID,
        std::move(ionAccessToken),
        std::move(ionAssetEndpointUrl),
        std::move(headerChangeListener),
        showCreditsOnScreen,
        ellipsoid);
  } else if (type == "3DTILES") {
    endpoint.type = type;
    endpoint.url = url;
    endpoint.accessToken = accessToken;
    endpointCache[requestUrl] = endpoint;
    return mainThreadLoadTilesetJsonFromAssetEndpoint(
        externals,
        endpoint,
        ionAssetID,
        std::move(ionAccessToken),
        std::move(ionAssetEndpointUrl),
        std::move(headerChangeListener),
        showCreditsOnScreen,
        ellipsoid);
  }

  TilesetContentLoaderResult<CesiumIonTilesetLoader> result;
  result.errors.emplaceError(
      fmt::format("Received unsupported asset response type: {}", type));
  return externals.asyncSystem.createResolvedFuture(std::move(result));
}

} // namespace

CesiumIonTilesetLoader::CesiumIonTilesetLoader(
    int64_t ionAssetID,
    std::string&& ionAccessToken,
    std::string&& ionAssetEndpointUrl,
    std::unique_ptr<TilesetContentLoader>&& pAggregatedLoader,
    AuthorizationHeaderChangeListener&& headerChangeListener,
    const CesiumGeospatial::Ellipsoid& ellipsoid)
    : _ellipsoid{ellipsoid},
      _ionAssetID{ionAssetID},
      _ionAccessToken{std::move(ionAccessToken)},
      _ionAssetEndpointUrl{std::move(ionAssetEndpointUrl)},
      _pAggregatedLoader{std::move(pAggregatedLoader)},
      _headerChangeListener{std::move(headerChangeListener)},
      _pLogger(nullptr),
      _pTilesetAccessor(nullptr),
      _pIonAccessor(nullptr),
      _tokenRefreshInProgress() {}

CesiumIonTilesetLoader::~CesiumIonTilesetLoader() noexcept {
  if (this->_pIonAccessor) {
    this->_pIonAccessor->notifyLoaderIsBeingDestroyed();
  }
}

CesiumAsync::Future<TileLoadResult>
CesiumIonTilesetLoader::loadTileContent(const TileLoadInput& loadInput) {
  if (this->_pTilesetAccessor == nullptr) {
    this->_pTilesetAccessor = loadInput.pAssetAccessor;
    this->_pIonAccessor = std::make_shared<CesiumIonAssetAccessor>(
        *this,
        this->_pTilesetAccessor);
  }

  if (this->_pTilesetAccessor != loadInput.pAssetAccessor) {
    // CesiumIonTilesetLoader requires this method to be called with the same
    // asset accessor instance every time.
    CESIUM_ASSERT(false);
    return loadInput.asyncSystem.createResolvedFuture(
        TileLoadResult::createFailedResult(loadInput.pAssetAccessor, nullptr));
  }

  this->_pLogger = loadInput.pLogger;

  TileLoadInput aggregatedInput(
      loadInput.tile,
      loadInput.contentOptions,
      loadInput.asyncSystem,
      this->_pIonAccessor,
      this->_pLogger,
      loadInput.requestHeaders,
      loadInput.ellipsoid);

  return this->_pAggregatedLoader->loadTileContent(aggregatedInput);
}

TileChildrenResult CesiumIonTilesetLoader::createTileChildren(
    const Tile& tile,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  auto pLoader = tile.getLoader();
  return pLoader->createTileChildren(tile, ellipsoid);
}

CesiumAsync::SharedFuture<std::string>
CesiumIonTilesetLoader::refreshTokenInMainThread(
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
      "Refreshing Cesium ion token for asset ID {} from {}.",
      this->_ionAssetID,
      this->_ionAssetEndpointUrl);

  std::string url = createEndpointResource(
      this->_ionAssetID,
      this->_ionAccessToken,
      this->_ionAssetEndpointUrl);
  this->_tokenRefreshInProgress =
      this->_pTilesetAccessor->get(asyncSystem, url)
          .thenInMainThread(
              [this](
                  std::shared_ptr<CesiumAsync::IAssetRequest>&& pIonRequest) {
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
                    this->_headerChangeListener(
                        "Authorization",
                        authorizationHeader);

                    // update cache with new access token
                    auto cacheIt = endpointCache.find(pIonRequest->url());
                    if (cacheIt != endpointCache.end()) {
                      cacheIt->second.accessToken = accessToken.value();
                    }

                    SPDLOG_LOGGER_INFO(
                        this->_pLogger,
                        "Successfuly refreshed Cesium ion token for asset ID "
                        "{} from {}.",
                        this->_ionAssetID,
                        this->_ionAssetEndpointUrl);

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

CesiumAsync::Future<TilesetContentLoaderResult<CesiumIonTilesetLoader>>
CesiumIonTilesetLoader::createLoader(
    const TilesetExternals& externals,
    const TilesetContentOptions& contentOptions,
    int64_t ionAssetID,
    const std::string& ionAccessToken,
    const std::string& ionAssetEndpointUrl,
    const AuthorizationHeaderChangeListener& headerChangeListener,
    bool showCreditsOnScreen,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  std::string ionUrl =
      createEndpointResource(ionAssetID, ionAccessToken, ionAssetEndpointUrl);
  auto cacheIt = endpointCache.find(ionUrl);
  if (cacheIt != endpointCache.end()) {
    const auto& endpoint = cacheIt->second;
    if (endpoint.type == "TERRAIN") {
      return mainThreadLoadLayerJsonFromAssetEndpoint(
                 externals,
                 contentOptions,
                 endpoint,
                 ionAssetID,
                 ionAccessToken,
                 ionAssetEndpointUrl,
                 headerChangeListener,
                 showCreditsOnScreen,
                 ellipsoid)
          .thenInMainThread(
              [externals,
               ellipsoid,
               contentOptions,
               ionAssetID,
               ionAccessToken,
               ionAssetEndpointUrl,
               headerChangeListener,
               showCreditsOnScreen](
                  TilesetContentLoaderResult<CesiumIonTilesetLoader>&& result) {
                return refreshTokenIfNeeded(
                    externals,
                    contentOptions,
                    ionAssetID,
                    ionAccessToken,
                    ionAssetEndpointUrl,
                    headerChangeListener,
                    showCreditsOnScreen,
                    std::move(result),
                    ellipsoid);
              });
    } else if (endpoint.type == "3DTILES") {
      return mainThreadLoadTilesetJsonFromAssetEndpoint(
                 externals,
                 endpoint,
                 ionAssetID,
                 ionAccessToken,
                 ionAssetEndpointUrl,
                 headerChangeListener,
                 showCreditsOnScreen,
                 ellipsoid)
          .thenInMainThread(
              [externals,
               contentOptions,
               ionAssetID,
               ionAccessToken,
               ionAssetEndpointUrl,
               headerChangeListener,
               showCreditsOnScreen,
               ellipsoid](
                  TilesetContentLoaderResult<CesiumIonTilesetLoader>&& result) {
                return refreshTokenIfNeeded(
                    externals,
                    contentOptions,
                    ionAssetID,
                    ionAccessToken,
                    ionAssetEndpointUrl,
                    headerChangeListener,
                    showCreditsOnScreen,
                    std::move(result),
                    ellipsoid);
              });
    }

    TilesetContentLoaderResult<CesiumIonTilesetLoader> result;
    result.errors.emplaceError(fmt::format(
        "Received unsupported asset response type: {}",
        endpoint.type));
    return externals.asyncSystem.createResolvedFuture(std::move(result));
  } else {
    return externals.pAssetAccessor->get(externals.asyncSystem, ionUrl)
        .thenInMainThread(
            [externals,
             ellipsoid,
             ionAssetID,
             ionAccessToken = ionAccessToken,
             ionAssetEndpointUrl = ionAssetEndpointUrl,
             headerChangeListener = headerChangeListener,
             showCreditsOnScreen,
             contentOptions](std::shared_ptr<CesiumAsync::IAssetRequest>&&
                                 pRequest) mutable {
              return mainThreadHandleEndpointResponse(
                  externals,
                  std::move(pRequest),
                  ionAssetID,
                  std::move(ionAccessToken),
                  std::move(ionAssetEndpointUrl),
                  contentOptions,
                  std::move(headerChangeListener),
                  showCreditsOnScreen,
                  ellipsoid);
            });
  }
}
CesiumAsync::Future<TilesetContentLoaderResult<CesiumIonTilesetLoader>>
CesiumIonTilesetLoader::refreshTokenIfNeeded(
    const TilesetExternals& externals,
    const TilesetContentOptions& contentOptions,
    int64_t ionAssetID,
    const std::string& ionAccessToken,
    const std::string& ionAssetEndpointUrl,
    const AuthorizationHeaderChangeListener& headerChangeListener,
    bool showCreditsOnScreen,
    TilesetContentLoaderResult<CesiumIonTilesetLoader>&& result,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  if (result.errors.hasErrors()) {
    if (result.statusCode == 401) {
      endpointCache.erase(createEndpointResource(
          ionAssetID,
          ionAccessToken,
          ionAssetEndpointUrl));
      return CesiumIonTilesetLoader::createLoader(
          externals,
          contentOptions,
          ionAssetID,
          ionAccessToken,
          ionAssetEndpointUrl,
          headerChangeListener,
          showCreditsOnScreen,
          ellipsoid);
    }
  }
  return externals.asyncSystem.createResolvedFuture(std::move(result));
}

} // namespace Cesium3DTilesSelection
