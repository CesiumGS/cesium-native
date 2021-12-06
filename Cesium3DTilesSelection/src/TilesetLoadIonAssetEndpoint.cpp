#include "TilesetLoadIonAssetEndpoint.h"

#include "Cesium3DTilesSelection/TileContentLoadResult.h"
#include "Cesium3DTilesSelection/Tileset.h"
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
  static CesiumAsync::Future<void> mainThreadHandleResponse(
      Tileset& tileset,
      std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest);

  static void mainThreadHandleTokenRefreshResponse(
      Tileset& tileset,
      std::shared_ptr<CesiumAsync::IAssetRequest>&& pIonRequest,
      TileContext* pContext,
      const std::shared_ptr<spdlog::logger>& pLogger);

  static FailedTileAction onIonTileFailed(Tile& failedTile);
};

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

  return tileset._externals.pAssetAccessor
      ->requestAsset(tileset._asyncSystem, ionUrl)
      .thenInMainThread([&tileset](std::shared_ptr<IAssetRequest>&& pRequest) {
        return Private::mainThreadHandleResponse(tileset, std::move(pRequest));
      })
      .catchInMainThread([&tileset, ionAssetID](const std::exception& e) {
        SPDLOG_LOGGER_ERROR(
            tileset._externals.pLogger,
            "Unhandled error for asset {}: {}",
            ionAssetID,
            e.what());
      })
      .thenImmediately(
          []() { CESIUM_TRACE_END_IN_TRACK("Tileset from ion startup"); });
}

Future<void> Tileset::LoadIonAssetEndpoint::Private::mainThreadHandleResponse(
    Tileset& tileset,
    std::shared_ptr<IAssetRequest>&& pRequest) {
  const IAssetResponse* pResponse = pRequest->response();
  if (!pResponse) {
    SPDLOG_LOGGER_ERROR(
        tileset.getExternals().pLogger,
        "No response received for asset request {}",
        pRequest->url());
    return tileset.getAsyncSystem().createResolvedFuture();
  }

  if (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300) {
    SPDLOG_LOGGER_ERROR(
        tileset.getExternals().pLogger,
        "Received status code {} for asset response {}",
        pResponse->statusCode(),
        pRequest->url());
    return tileset.getAsyncSystem().createResolvedFuture();
  }

  const gsl::span<const std::byte> data = pResponse->data();

  rapidjson::Document ionResponse;
  ionResponse.Parse(reinterpret_cast<const char*>(data.data()), data.size());

  if (ionResponse.HasParseError()) {
    SPDLOG_LOGGER_ERROR(
        tileset.getExternals().pLogger,
        "Error when parsing Cesium ion response JSON, error code {} at byte "
        "offset {}",
        ionResponse.GetParseError(),
        ionResponse.GetErrorOffset());
    return tileset.getAsyncSystem().createResolvedFuture();
  }

  if (tileset.getExternals().pCreditSystem) {

    const auto attributionsIt = ionResponse.FindMember("attributions");
    if (attributionsIt != ionResponse.MemberEnd() &&
        attributionsIt->value.IsArray()) {

      for (const rapidjson::Value& attribution :
           attributionsIt->value.GetArray()) {

        const auto html = attribution.FindMember("html");
        if (html != attribution.MemberEnd() && html->value.IsString()) {
          tileset._tilesetCredits.push_back(
              tileset.getExternals().pCreditSystem->createCredit(
                  html->value.GetString()));
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
    SPDLOG_LOGGER_ERROR(
        tileset.getExternals().pLogger,
        "Received unsupported asset response type: {}",
        type);
    return tileset.getAsyncSystem().createResolvedFuture();
  }

  auto pContext = std::make_unique<TileContext>();

  pContext->pTileset = &tileset;
  pContext->baseUrl = url;
  pContext->requestHeaders.push_back(
      std::make_pair("Authorization", "Bearer " + accessToken));
  pContext->failedTileCallback = [](Tile& failedTile) {
    return Private::onIonTileFailed(failedTile);
  };
  return LoadTilesetDotJson::start(
      tileset,
      pContext->baseUrl,
      pContext->requestHeaders,
      std::move(pContext));
}

namespace {

/**
 * @brief Tries to update the context request headers with a new token.
 *
 * This will try to obtain the `accessToken` from the JSON of the
 * given response, and set it as the `Bearer ...` value of the
 * `Authorization` header of the request headers of the given
 * context.
 *
 * @param pContext The context
 * @param pIonResponse The response
 * @return Whether the update succeeded
 */
bool updateContextWithNewToken(
    TileContext* pContext,
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
    return false;
  }
  std::string accessToken =
      JsonHelpers::getStringOrDefault(ionResponse, "accessToken", "");

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
  return true;
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
    failed = !updateContextWithNewToken(pContext, pIonResponse, pLogger);
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
        .pAssetAccessor->requestAsset(tileset.getAsyncSystem(), url)
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
