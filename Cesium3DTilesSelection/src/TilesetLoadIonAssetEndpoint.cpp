#include "TilesetLoadIonAssetEndpoint.h"

#include "Cesium3DTilesSelection/TileContentLoadResult.h"
#include "Cesium3DTilesSelection/Tileset.h"
#include "Cesium3DTilesSelection/TilesetLoadFailureDetails.h"
#include "TilesetLoadTilesetDotJson.h"

#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Uri.h>

#include <rapidjson/document.h>

#include <cassert>

using namespace Cesium3DTilesSelection;
using namespace CesiumAsync;
using namespace CesiumUtility;

struct Tileset::LoadIonAssetEndpoint::Private {
  struct AssetEndpointAttribution {
    std::string html;
    bool collapsible;
  };
  struct AssetEndpoint {
    std::string type;
    std::string url;
    std::string accessToken;
    std::vector<AssetEndpointAttribution> attributions;
  };
  static std::unordered_map<std::string, AssetEndpoint> endpointCache;
  static Future<std::optional<TilesetLoadFailureDetails>>
  mainThreadLoadTilesetFromAssetEndpoint(
      Tileset& tileset,
      const AssetEndpoint& endpoint);
  static Future<std::optional<TilesetLoadFailureDetails>>
  loadAssetEndpoint(Tileset& tileset, const std::string& url);
  static Future<std::optional<TilesetLoadFailureDetails>>
  mainThreadHandleResponse(
      Tileset& tileset,
      std::shared_ptr<IAssetRequest>&& pRequest);

  static void mainThreadHandleTokenRefreshResponse(
      Tileset& tileset,
      std::shared_ptr<IAssetRequest>&& pIonRequest,
      TileContext* pContext,
      const std::shared_ptr<spdlog::logger>& pLogger);

  static FailedTileAction onIonTileFailed(Tile& failedTile);

  template <typename T>
  static auto handlePotentialError(Tileset& tileset, T&& operation) {
    return std::move(operation)
        .catchInMainThread([&tileset](const std::exception& e) {
          TilesetLoadFailureDetails failure;
          failure.pTileset = &tileset;
          failure.pRequest = nullptr;
          failure.type = TilesetLoadType::CesiumIon;
          failure.message = fmt::format(
              "Unhandled error for asset {}: {}",
              tileset.getIonAssetID().value_or(0),
              e.what());
          return std::make_optional(failure);
        })
        .thenImmediately(
            [&tileset](
                std::optional<TilesetLoadFailureDetails>&& maybeFailure) {
              if (maybeFailure) {
                tileset.reportError(std::move(*maybeFailure));
              }
            })
        .catchImmediately([](const std::exception& /*e*/) {
          // We should only land here if tileset.reportError above throws an
          // exception, which it shouldn't. Flag it in a debug build and ignore
          // it in a release build.
          assert(false);
        });
  }
};

std::unordered_map<
    std::string,
    Tileset::LoadIonAssetEndpoint::Private::AssetEndpoint>
    Tileset::LoadIonAssetEndpoint::Private::endpointCache;

Future<std::optional<TilesetLoadFailureDetails>>
Tileset::LoadIonAssetEndpoint::Private::loadAssetEndpoint(
    Tileset& tileset,
    const std::string& url) {
  auto cacheIt = endpointCache.find(url);
  if (cacheIt != endpointCache.end()) {
    return tileset.getAsyncSystem().createResolvedFuture().thenInMainThread(
        [&tileset, endpoint = cacheIt->second]() {
          return mainThreadLoadTilesetFromAssetEndpoint(tileset, endpoint);
        });
  } else
    return tileset._externals.pAssetAccessor->get(tileset.getAsyncSystem(), url)
        .thenInMainThread(
            [&tileset](std::shared_ptr<IAssetRequest>&& pRequest) {
              return Private::mainThreadHandleResponse(
                  tileset,
                  std::move(pRequest));
            });
}
Future<std::optional<TilesetLoadFailureDetails>>
Tileset::LoadIonAssetEndpoint::Private::mainThreadLoadTilesetFromAssetEndpoint(
    Tileset& tileset,
    const Private::AssetEndpoint& endpoint) {
  if (tileset.getExternals().pCreditSystem) {
    for (auto& endpointAttribution : endpoint.attributions) {
      tileset._tilesetCredits.push_back(
          tileset.getExternals().pCreditSystem->createCredit(
              endpointAttribution.html));
    }
  }
  auto pContext = std::make_unique<TileContext>();
  pContext->pTileset = &tileset;
  pContext->baseUrl = endpoint.url;
  pContext->requestHeaders.push_back(
      std::make_pair("Authorization", "Bearer " + endpoint.accessToken));
  pContext->failedTileCallback = Private::onIonTileFailed;
  return LoadTilesetDotJson::start(
             tileset,
             pContext->baseUrl,
             pContext->requestHeaders,
             std::move(pContext))
      .thenImmediately([]() -> std::optional<TilesetLoadFailureDetails> {
        return std::nullopt;
      });
}

CesiumAsync::Future<void>
Tileset::LoadIonAssetEndpoint::start(Tileset& tileset) {
  assert(tileset._ionAssetID.has_value());
  assert(tileset._ionAccessToken.has_value());

  CESIUM_TRACE_BEGIN_IN_TRACK("Tileset from ion startup");

  uint32_t ionAssetID = *tileset._ionAssetID;
  std::string ionAccessToken = *tileset._ionAccessToken;

  std::string ionUrl = "https://api.cesium.com/v1/assets/" +
                       std::to_string(ionAssetID) + "/endpoint";
  if (!ionAccessToken.empty()) {
    ionUrl += "?access_token=" + ionAccessToken;
  }
  return Private::handlePotentialError(
             tileset,
             Private::loadAssetEndpoint(tileset, ionUrl))
      .thenImmediately(
          []() { CESIUM_TRACE_END_IN_TRACK("Tileset from ion startup"); });
}

Future<std::optional<TilesetLoadFailureDetails>>
Tileset::LoadIonAssetEndpoint::Private::mainThreadHandleResponse(
    Tileset& tileset,
    std::shared_ptr<IAssetRequest>&& pRequest) {
  const IAssetResponse* pResponse = pRequest->response();
  if (!pResponse) {
    TilesetLoadFailureDetails error;
    error.pTileset = &tileset;
    error.pRequest = std::move(pRequest);
    error.type = TilesetLoadType::CesiumIon;
    error.message = fmt::format(
        "No response received for asset request {}",
        error.pRequest->url());
    return tileset.getAsyncSystem().createResolvedFuture(
        std::make_optional(std::move(error)));
  }

  if (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300) {
    TilesetLoadFailureDetails error;
    error.pTileset = &tileset;
    error.pRequest = std::move(pRequest);
    error.type = TilesetLoadType::CesiumIon;
    error.message = fmt::format(
        "Received status code {} for asset response {}",
        pResponse->statusCode(),
        error.pRequest->url());
    return tileset.getAsyncSystem().createResolvedFuture(
        std::make_optional(std::move(error)));
  }

  const gsl::span<const std::byte> data = pResponse->data();

  rapidjson::Document ionResponse;
  ionResponse.Parse(reinterpret_cast<const char*>(data.data()), data.size());

  if (ionResponse.HasParseError()) {
    TilesetLoadFailureDetails error;
    error.pTileset = &tileset;
    error.pRequest = std::move(pRequest);
    error.type = TilesetLoadType::CesiumIon;
    error.message = fmt::format(
        "Error when parsing Cesium ion response JSON, error code {} at byte "
        "offset {}",
        ionResponse.GetParseError(),
        ionResponse.GetErrorOffset());
    return tileset.getAsyncSystem().createResolvedFuture(
        std::make_optional(std::move(error)));
  }

  AssetEndpoint endpoint;
  if (tileset.getExternals().pCreditSystem) {

    const auto attributionsIt = ionResponse.FindMember("attributions");
    if (attributionsIt != ionResponse.MemberEnd() &&
        attributionsIt->value.IsArray()) {

      for (const rapidjson::Value& attribution :
           attributionsIt->value.GetArray()) {
        const auto html = attribution.FindMember("html");
        if (html != attribution.MemberEnd() && html->value.IsString()) {
          AssetEndpointAttribution& endpointAttribution =
              endpoint.attributions.emplace_back();
          endpointAttribution.html = html->value.GetString();
        }
        // TODO: mandate the user show certain credits on screen, as opposed to
        // an expandable panel auto showOnScreen =
        // attribution.FindMember("collapsible");
        // ...
      }
    }
  }

  std::string url = JsonHelpers::getStringOrDefault(ionResponse, "url", "");
  std::string accessToken =
      JsonHelpers::getStringOrDefault(ionResponse, "accessToken", "");

  std::string type = JsonHelpers::getStringOrDefault(ionResponse, "type", "");
  if (type == "TERRAIN") {
    // For terrain resources, we need to append `/layer.json` to the end of the
    // URL.
    url = CesiumUtility::Uri::resolve(url, "layer.json", true);
  } else if (type != "3DTILES") {
    TilesetLoadFailureDetails error;
    error.pTileset = &tileset;
    error.pRequest = std::move(pRequest);
    error.type = TilesetLoadType::CesiumIon;
    error.message =
        fmt::format("Received unsupported asset response type: {}", type);
    return tileset.getAsyncSystem().createResolvedFuture(
        std::make_optional(std::move(error)));
  }
  endpoint.type = type;
  endpoint.url = url;
  endpoint.accessToken = accessToken;
  endpointCache[pRequest->url()] = endpoint;
  return mainThreadLoadTilesetFromAssetEndpoint(tileset, endpoint);
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
    const IAssetResponse* pIonResponse,
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
  return JsonHelpers::getStringOrDefault(ionResponse, "accessToken", "");
}
/**
 * @brief Updates the context request header with the given token.
 *
 * @param pContext The context
 * @param accessToken The token
 */
void updateContextWithNewToken(
    TileContext* pContext,
    const std::string& accessToken) {
  auto authIt = std::find_if(
      pContext->requestHeaders.begin(),
      pContext->requestHeaders.end(),
      [](auto& headerPair) { return headerPair.first == "Authorization"; });
  if (authIt != pContext->requestHeaders.end()) {
    authIt->second = "Bearer " + accessToken;
  } else {
    pContext->requestHeaders.push_back(
        std::make_pair("Authorization", "Bearer " + accessToken));
  }
}

} // namespace

void Tileset::LoadIonAssetEndpoint::Private::
    mainThreadHandleTokenRefreshResponse(
        Tileset& tileset,
        std::shared_ptr<IAssetRequest>&& pIonRequest,
        TileContext* pContext,
        const std::shared_ptr<spdlog::logger>& pLogger) {
  const IAssetResponse* pIonResponse = pIonRequest->response();

  bool failed = true;
  if (pIonResponse && pIonResponse->statusCode() >= 200 &&
      pIonResponse->statusCode() < 300) {
    auto accessToken = getNewAccessToken(pIonResponse, pLogger);
    if (accessToken.has_value()) {
      failed = false;
      updateContextWithNewToken(pContext, accessToken.value());
      // update cache with new access token
      auto cacheIt = Private::endpointCache.find(pIonRequest->url());
      if (cacheIt != Private::endpointCache.end()) {
        cacheIt->second.accessToken = accessToken.value();
      }
    }
  }

  // Put all auth-failed tiles in this context back into the Unloaded state.
  // TODO: the way this is structured, requests already in progress with the old
  // key might complete after the key has been updated, and there's nothing here
  // clever enough to avoid refreshing the key _again_ in that instance.
  Tile* pTile = tileset._loadedTiles.head();

  while (pTile) {
    if (pTile->getContext() == pContext &&
        pTile->getState() == Tile::LoadState::FailedTemporarily &&
        pTile->getContent() && pTile->getContent()->httpStatusCode == 401) {
      if (failed) {
        pTile->markPermanentlyFailed();
      } else {
        pTile->unloadContent();
      }
    }

    pTile = tileset._loadedTiles.next(*pTile);
  }
}

FailedTileAction
Tileset::LoadIonAssetEndpoint::Private::onIonTileFailed(Tile& failedTile) {
  TileContentLoadResult* pContent = failedTile.getContent();
  if (!pContent) {
    return FailedTileAction::GiveUp;
  }

  if (pContent->httpStatusCode != 401) {
    return FailedTileAction::GiveUp;
  }

  assert(failedTile.getContext()->pTileset != nullptr);
  Tileset& tileset = *failedTile.getContext()->pTileset;

  if (!tileset._ionAssetID) {
    return FailedTileAction::GiveUp;
  }

  if (!tileset._isRefreshingIonToken) {
    tileset._isRefreshingIonToken = true;

    std::string url = "https://api.cesium.com/v1/assets/" +
                      std::to_string(tileset._ionAssetID.value()) + "/endpoint";
    if (tileset._ionAccessToken) {
      url += "?access_token=" + tileset._ionAccessToken.value();
    }

    tileset.notifyTileStartLoading(nullptr);

    tileset.getExternals()
        .pAssetAccessor->get(tileset.getAsyncSystem(), url)
        .thenInMainThread([&tileset, pContext = failedTile.getContext()](
                              std::shared_ptr<IAssetRequest>&& pIonRequest) {
          Private::mainThreadHandleTokenRefreshResponse(
              tileset,
              std::move(pIonRequest),
              pContext,
              tileset.getExternals().pLogger);
        })
        .catchInMainThread([&tileset](const std::exception& e) {
          SPDLOG_LOGGER_ERROR(
              tileset.getExternals().pLogger,
              "Unhandled error when retrying request: {}",
              e.what());
        })
        .thenInMainThread([&tileset]() {
          tileset._isRefreshingIonToken = false;
          tileset.notifyTileDoneLoading(nullptr);
        });
  }

  return FailedTileAction::Wait;
}
