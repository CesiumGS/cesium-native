#include <Cesium3DTilesSelection/Exp_CesiumIonTilesetLoader.h>
#include <Cesium3DTilesSelection/Exp_TilesetJsonLoader.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Uri.h>
#include <rapidjson/document.h>
#include <unordered_map>

namespace Cesium3DTilesSelection {
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
    uint32_t ionAssetID,
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
  const gsl::span<const std::byte> data = pIonResponse->data();
  rapidjson::Document ionResponse;
  ionResponse.Parse(reinterpret_cast<const char*>(data.data()), data.size());
  if (ionResponse.HasParseError()) {
    SPDLOG_LOGGER_ERROR(
        pLogger,
        "Error when parsing Cesium ion response, error code {} at byte offset "
        "{}",
        ionResponse.GetParseError(),
        ionResponse.GetErrorOffset());
    return std::nullopt;
  }
  return CesiumUtility::JsonHelpers::getStringOrDefault(ionResponse, "accessToken", "");
}

CesiumAsync::Future<TilesetContentLoaderResult>
mainThreadLoadTilesetJsonFromAssetEndpoint(
    const TilesetExternals& externals,
    const AssetEndpoint& endpoint,
    uint32_t ionAssetID,
    std::string ionAccessToken,
    std::string ionAssetEndpointUrl,
    bool showCreditsOnScreen) {
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
             requestHeaders)
      .thenImmediately(
          [credits = std::move(credits),
           externals,
           requestHeaders,
           ionAssetID,
           ionAccessToken = std::move(ionAccessToken),
           ionAssetEndpointUrl = std::move(ionAssetEndpointUrl)](
              TilesetContentLoaderResult&& tilesetJsonResult) mutable {
            if (tilesetJsonResult.credits.empty()) {
              tilesetJsonResult.credits = std::move(credits);
            } else {
              tilesetJsonResult.credits.insert(
                  tilesetJsonResult.credits.end(),
                  credits.begin(),
                  credits.end());
            }

            TilesetContentLoaderResult result;
            if (!tilesetJsonResult.errors) {
              result.pLoader = std::make_unique<CesiumIonTilesetLoader>(
                  externals,
                  ionAssetID,
                  std::move(ionAccessToken),
                  std::move(ionAssetEndpointUrl),
                  std::move(result.pLoader));
              result.pRootTile = std::move(tilesetJsonResult.pRootTile);
              result.gltfUpAxis = tilesetJsonResult.gltfUpAxis;
              result.credits = std::move(tilesetJsonResult.credits);
              result.requestHeaders = std::move(requestHeaders);
            }
            result.errors = std::move(tilesetJsonResult.errors);
            return result;
          });
}

CesiumAsync::Future<TilesetContentLoaderResult>
mainThreadHandleEndpointResponse(
    const TilesetExternals& externals,
    std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest,
    uint32_t ionAssetID,
    std::string&& ionAccessToken,
    std::string&& ionAssetEndpointUrl,
    bool showCreditsOnScreen) {
  const CesiumAsync::IAssetResponse* pResponse = pRequest->response();
  const std::string& requestUrl = pRequest->url();
  if (!pResponse) {
    TilesetContentLoaderResult result;
    result.errors.emplace_error(
        fmt::format("No response received for asset request {}", requestUrl));
    return externals.asyncSystem.createResolvedFuture(std::move(result));
  }

  uint16_t statusCode = pResponse->statusCode();
  if (statusCode < 200 || statusCode >= 300) {
    TilesetContentLoaderResult result;
    result.errors.emplace_error(fmt::format(
        "Received status code {} for asset response {}",
        statusCode,
        requestUrl));
    return externals.asyncSystem.createResolvedFuture(std::move(result));
  }

  const gsl::span<const std::byte> data = pResponse->data();

  rapidjson::Document ionResponse;
  ionResponse.Parse(reinterpret_cast<const char*>(data.data()), data.size());

  if (ionResponse.HasParseError()) {
    TilesetContentLoaderResult result;
    result.errors.emplace_error(fmt::format(
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

  std::string url =
      CesiumUtility::JsonHelpers::getStringOrDefault(ionResponse, "url", "");
  std::string accessToken = CesiumUtility::JsonHelpers::getStringOrDefault(
      ionResponse,
      "accessToken",
      "");

  std::string type =
      CesiumUtility::JsonHelpers::getStringOrDefault(ionResponse, "type", "");
  if (type == "TERRAIN") {
    // For terrain resources, we need to append `/layer.json` to the end of the
    // URL.
    url = CesiumUtility::Uri::resolve(url, "layer.json", true);
    endpoint.type = type;
    endpoint.url = url;
    endpoint.accessToken = accessToken;
    endpointCache[requestUrl] = endpoint;
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
        showCreditsOnScreen);
  }

  TilesetContentLoaderResult result;
  result.errors.emplace_error(
      fmt::format("Received unsupported asset response type: {}", type));
  return externals.asyncSystem.createResolvedFuture(std::move(result));
}
} // namespace

CesiumIonTilesetLoader::CesiumIonTilesetLoader(
    const TilesetExternals& externals,
    uint32_t ionAssetID,
    std::string&& ionAccessToken,
    std::string&& ionAssetEndpointUrl,
    std::unique_ptr<TilesetContentLoader>&& pAggregatedLoader)
    : TilesetContentLoader(externals),
      _refreshTokenState{TokenRefreshState::None},
      _ionAssetID{ionAssetID},
      _ionAccessToken{std::move(ionAccessToken)},
      _ionAssetEndpointUrl{std::move(ionAssetEndpointUrl)},
      _pAggregatedLoader{std::move(pAggregatedLoader)} {}

CesiumAsync::Future<TileLoadResult> CesiumIonTilesetLoader::doLoadTileContent(
    Tile& tile,
    const TilesetContentOptions& contentOptions,
    const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders) {
  if (_refreshTokenState == TokenRefreshState::Loading) {
    return _externals.asyncSystem.createResolvedFuture(TileLoadResult{
        TileUnknownContent{},
        TileLoadState::FailedTemporarily,
        0});
  } else if (_refreshTokenState == TokenRefreshState::Failed) {
    return _externals.asyncSystem.createResolvedFuture(
        TileLoadResult{TileUnknownContent{}, TileLoadState::Failed, 0});
  }

  // TODO: the way this is structured, requests already in progress
  // with the old key might complete after the key has been updated,
  // and there's nothing here clever enough to avoid refreshing the
  // key _again_ in that instance.
  auto refreshTokenInMainThread = [this]() {
    this->refreshTokenInMainThread();
  };

  return _pAggregatedLoader
      ->doLoadTileContent(tile, contentOptions, requestHeaders)
      .thenImmediately([asyncSystem = _externals.asyncSystem,
                        refreshTokenInMainThread](TileLoadResult&& result) {
        // check to see if we need to refresh token
        if (result.httpStatusCode == 401) {
          asyncSystem.runInMainThread(refreshTokenInMainThread);
        }

        return std::move(result);
      });
}

void CesiumIonTilesetLoader::doProcessLoadedContent(Tile& tile) {
  _pAggregatedLoader->doProcessLoadedContent(tile);
}

void CesiumIonTilesetLoader::refreshTokenInMainThread() {
  _refreshTokenState = TokenRefreshState::Loading;

  std::string url = createEndpointResource(
      _ionAssetID,
      _ionAccessToken,
      _ionAssetEndpointUrl);
  _externals.pAssetAccessor->get(_externals.asyncSystem, url)
      .thenInMainThread(
          [this](std::shared_ptr<CesiumAsync::IAssetRequest>&& pIonRequest) {
            const CesiumAsync::IAssetResponse* pIonResponse =
                pIonRequest->response();

            if (!pIonResponse) {
              _refreshTokenState = TokenRefreshState::Failed;
              return;
            }

            uint16_t statusCode = pIonResponse->statusCode();
            if (statusCode >= 200 && statusCode < 300) {
              auto accessToken =
                  getNewAccessToken(pIonResponse, _externals.pLogger);
              if (accessToken) {
                broadCastRequestHeaderChange(
                    std::make_pair("Authorization", "Bearer " + *accessToken));

                // update cache with new access token
                auto cacheIt = endpointCache.find(pIonRequest->url());
                if (cacheIt != endpointCache.end()) {
                  cacheIt->second.accessToken = accessToken.value();
                }

                _refreshTokenState = TokenRefreshState::Done;
                return;
              }
            }

            _refreshTokenState = TokenRefreshState::Failed;
          });
}

CesiumAsync::Future<TilesetContentLoaderResult>
CesiumIonTilesetLoader::createLoader(
    const TilesetExternals& externals,
    uint32_t ionAssetID,
    const std::string& ionAccessToken,
    const std::string& ionAssetEndpointUrl,
    bool showCreditsOnScreen) {
  std::string ionUrl =
      createEndpointResource(ionAssetID, ionAccessToken, ionAssetEndpointUrl);
  auto cacheIt = endpointCache.find(ionUrl);
  if (cacheIt != endpointCache.end()) {
    const auto& endpoint = cacheIt->second;
    if (endpoint.type == "TERRAIN") {

    } else if (endpoint.type == "3DTILES") {
      return mainThreadLoadTilesetJsonFromAssetEndpoint(
          externals,
          endpoint,
          ionAssetID,
          ionAccessToken,
          ionAssetEndpointUrl,
          showCreditsOnScreen);
    }

    TilesetContentLoaderResult result;
    result.errors.emplace_error(fmt::format(
        "Received unsupported asset response type: {}",
        endpoint.type));
    return externals.asyncSystem.createResolvedFuture(std::move(result));
  } else {
    return externals.pAssetAccessor->get(externals.asyncSystem, ionUrl)
        .thenInMainThread(
            [externals,
             ionAssetID,
             ionAccessToken = ionAccessToken,
             ionAssetEndpointUrl = ionAssetEndpointUrl,
             showCreditsOnScreen](std::shared_ptr<CesiumAsync::IAssetRequest>&&
                                      pRequest) mutable {
              return mainThreadHandleEndpointResponse(
                  externals,
                  std::move(pRequest),
                  ionAssetID,
                  std::move(ionAccessToken),
                  std::move(ionAssetEndpointUrl),
                  showCreditsOnScreen);
            });
  }
}
} // namespace Cesium3DTilesSelection
