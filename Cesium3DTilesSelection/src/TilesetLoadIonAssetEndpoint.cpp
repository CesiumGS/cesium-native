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

  static std::string createEndpointResource(const Tileset& tileset);

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

std::string Tileset::LoadIonAssetEndpoint::Private::createEndpointResource(
    const Tileset& tileset) {
  std::string ionUrl = tileset._ionAssetEndpointUrl.value() + "v1/assets/" +
                       std::to_string(tileset._ionAssetID.value()) +
                       "/endpoint";
  if (tileset._ionAccessToken.has_value()) {
    ionUrl += "?access_token=" + tileset._ionAccessToken.value();
  }

  return ionUrl;
}

CesiumAsync::Future<void>
Tileset::LoadIonAssetEndpoint::start(Tileset& tileset) {
  assert(tileset._ionAssetID.has_value());
  assert(tileset._ionAccessToken.has_value());

  CESIUM_TRACE_BEGIN_IN_TRACK("Tileset from ion startup");

  std::string ionUrl = Private::createEndpointResource(tileset);

  auto operation =
      tileset._externals.pAssetAccessor->get(tileset._asyncSystem, ionUrl)
          .thenInMainThread(
              [&tileset](std::shared_ptr<IAssetRequest>&& pRequest) {
                return Private::mainThreadHandleResponse(
                    tileset,
                    std::move(pRequest));
              });

  return Private::handlePotentialError(tileset, std::move(operation))
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
                  html->value.GetString(),
                  tileset._options.showCreditsOnScreen));
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

  auto pContext = std::make_unique<TileContext>();

  pContext->pTileset = &tileset;
  pContext->baseUrl = url;
  pContext->requestHeaders.push_back(
      std::make_pair("Authorization", "Bearer " + accessToken));
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

    std::string url = Private::createEndpointResource(tileset);

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
