#include "ITwinRealityDataContentLoader.h"

#include "ITwinUtilities.h"
#include "TilesetJsonLoader.h"

#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileLoadResult.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetContentLoaderResult.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/HttpHeaders.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/JsonHelpers.h>
#include <CesiumUtility/Result.h>
#include <CesiumUtility/Uri.h>

#include <fmt/format.h>
#include <rapidjson/document.h>
#include <spdlog/spdlog.h>

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace Cesium3DTilesSelection {

// This is an IAssetAccessor decorator that handles token refresh for any asset
// that returns a 403 error.
class RealityDataAssetAccessor
    : public std::enable_shared_from_this<RealityDataAssetAccessor>,
      public CesiumAsync::IAssetAccessor {
public:
  RealityDataAssetAccessor(
      ITwinRealityDataContentLoader& tilesetLoader,
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

          return pThis->_pTilesetLoader->obtainNewAccessToken().thenImmediately(
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
        .thenImmediately([asyncSystem, refreshToken = std::move(refreshToken)](
                             std::shared_ptr<CesiumAsync::IAssetRequest>&&
                                 pRequest) mutable {
          const CesiumAsync::IAssetResponse* pResponse = pRequest->response();
          if (!pResponse) {
            return asyncSystem.createResolvedFuture(std::move(pRequest));
          }

          if (pResponse->statusCode() == 403) {
            // We need to refresh the iTwin access token.
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
  ITwinRealityDataContentLoader* _pTilesetLoader;
  std::shared_ptr<CesiumAsync::IAssetAccessor> _pAggregatedAccessor;
};

namespace {

enum class RealityDataType {
  Cesium3DTiles,
  RealityMesh3DTiles,
  Terrain3DTiles,
  Pnts,
  Unsupported
};

struct RealityDataDetails {
  std::string id;
  std::string rootDocument;
  RealityDataType type;
};

std::optional<RealityDataDetails>
parseRealityDataDetails(const rapidjson::Document& jsonDocument) {
  const auto& realityData = jsonDocument.FindMember("realityData");
  if (realityData == jsonDocument.MemberEnd() ||
      !realityData->value.IsObject()) {
    return std::nullopt;
  }

  const rapidjson::Value::ConstObject& realityDataObj =
      realityData->value.GetObject();

  RealityDataDetails details;
  details.id =
      CesiumUtility::JsonHelpers::getStringOrDefault(realityDataObj, "id", "");
  details.rootDocument = CesiumUtility::JsonHelpers::getStringOrDefault(
      realityDataObj,
      "rootDocument",
      "");

  const std::string typeStr = CesiumUtility::JsonHelpers::getStringOrDefault(
      realityDataObj,
      "type",
      "");
  // Types from
  // https://developer.bentley.com/apis/reality-management/rm-rd-details/#types
  if (typeStr == "Cesium3DTiles") {
    details.type = RealityDataType::Cesium3DTiles;
  } else if (typeStr == "PNTS") {
    details.type = RealityDataType::Pnts;
  } else if (typeStr == "RealityMesh3DTiles") {
    details.type = RealityDataType::RealityMesh3DTiles;
  } else if (typeStr == "Terrain3DTiles") {
    details.type = RealityDataType::Terrain3DTiles;
  } else {
    details.type = RealityDataType::Unsupported;
  }

  return details;
}

std::string getContainerUrl(const rapidjson::Document& jsonDocument) {
  const auto& links = jsonDocument.FindMember("_links");
  if (links == jsonDocument.MemberEnd() || !links->value.IsObject()) {
    return {};
  }

  const auto& containerUrl =
      links->value.GetObject().FindMember("containerUrl");
  if (containerUrl == jsonDocument.MemberEnd() ||
      !containerUrl->value.IsObject()) {
    return {};
  }

  return CesiumUtility::JsonHelpers::getStringOrDefault(
      containerUrl->value,
      "href",
      "");
}

CesiumAsync::Future<TilesetContentLoaderResult<ITwinRealityDataContentLoader>>
requestRealityDataContainer(
    const TilesetExternals& externals,
    const RealityDataDetails& details,
    const std::optional<std::string>& iTwinId,
    const std::string& iTwinAccessToken,
    ITwinRealityDataContentLoader::TokenRefreshCallback&& tokenRefreshCallback,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  CesiumUtility::Uri readAccessUri(fmt::format(
      "https://api.bentley.com/reality-management/reality-data/{}/readaccess",
      details.id));
  if (iTwinId.has_value()) {
    CesiumUtility::UriQuery query("");
    query.setValue("iTwinId", *iTwinId);
    readAccessUri.setQuery(query.toQueryString());
  }

  const std::vector<CesiumAsync::IAssetAccessor::THeader> headers = {
      {"Authorization", "Bearer " + iTwinAccessToken},
      {"Accept", "application/vnd.bentley.itwin-platform.v1+json"}};

  return externals.pAssetAccessor
      ->get(
          externals.asyncSystem,
          std::string{readAccessUri.toString()},
          headers)
      .thenImmediately([externals,
                        details,
                        iTwinId,
                        iTwinAccessToken,
                        tokenRefreshCallback = std::move(tokenRefreshCallback),
                        ellipsoid](std::shared_ptr<CesiumAsync::IAssetRequest>&&
                                       pRequest) mutable {
        const CesiumAsync::IAssetResponse* pResponse = pRequest->response();
        const std::string& requestUrl = pRequest->url();
        if (!pResponse) {
          TilesetContentLoaderResult<ITwinRealityDataContentLoader> result;
          result.errors.emplaceError(fmt::format(
              "No response received for reality data read access request {}",
              requestUrl));
          return externals.asyncSystem.createResolvedFuture(std::move(result));
        }

        uint16_t statusCode = pResponse->statusCode();
        if (statusCode < 200 || statusCode >= 300) {
          TilesetContentLoaderResult<ITwinRealityDataContentLoader> result;
          result.errors.emplaceError(fmt::format(
              "Received status code {} for reality data read access response "
              "{}",
              statusCode,
              requestUrl));
          result.statusCode = statusCode;
          parseITwinErrorResponseIntoErrorList(*pResponse, result.errors);
          return externals.asyncSystem.createResolvedFuture(std::move(result));
        }

        const std::span<const std::byte> data = pResponse->data();

        rapidjson::Document readAccessResponse;
        readAccessResponse.Parse(
            reinterpret_cast<const char*>(data.data()),
            data.size());

        const std::string parsedContainerUrl =
            getContainerUrl(readAccessResponse);
        if (parsedContainerUrl.empty()) {
          TilesetContentLoaderResult<ITwinRealityDataContentLoader> result;
          result.errors.emplaceError(fmt::format(
              "Couldn't obtain container URL for reality data {}",
              details.id));
          return externals.asyncSystem.createResolvedFuture(std::move(result));
        }

        // containerUrl provides the directory that contains the data,
        // rootDocument provides the file that contains the tileset info, we
        // need to combine the two.
        CesiumUtility::Uri containerUri(parsedContainerUrl);
        containerUri.setPath(
            fmt::format("{}/{}", containerUri.getPath(), details.rootDocument));

        return TilesetJsonLoader::createLoader(
                   externals,
                   std::string{containerUri.toString()},
                   std::vector<CesiumAsync::IAssetAccessor::THeader>{},
                   ellipsoid)
            .thenImmediately([tokenRefreshCallback =
                                  std::move(tokenRefreshCallback),
                              iTwinAccessToken](
                                 TilesetContentLoaderResult<TilesetJsonLoader>&&
                                     tilesetJsonResult) mutable {
              TilesetContentLoaderResult<ITwinRealityDataContentLoader> result;
              if (!tilesetJsonResult.errors) {
                result.pLoader =
                    std::make_unique<ITwinRealityDataContentLoader>(
                        iTwinAccessToken,
                        std::move(tokenRefreshCallback),
                        std::move(tilesetJsonResult.pLoader));
                result.pRootTile = std::move(tilesetJsonResult.pRootTile);
                result.credits = std::move(tilesetJsonResult.credits);
              }
              result.errors = std::move(tilesetJsonResult.errors);
              result.statusCode = tilesetJsonResult.statusCode;
              return result;
            });
      });
}

} // namespace

CesiumAsync::Future<TilesetContentLoaderResult<ITwinRealityDataContentLoader>>
ITwinRealityDataContentLoader::createLoader(
    const TilesetExternals& externals,
    const std::string& realityDataId,
    const std::optional<std::string>& iTwinId,
    const std::string& iTwinAccessToken,
    TokenRefreshCallback&& tokenRefreshCallback,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  CesiumUtility::Uri realityMetadataUri(
      "https://api.bentley.com/reality-management/reality-data/" +
      realityDataId);
  if (iTwinId.has_value()) {
    CesiumUtility::UriQuery query("");
    query.setValue("iTwinId", *iTwinId);
    realityMetadataUri.setQuery(query.toQueryString());
  }

  const std::vector<CesiumAsync::IAssetAccessor::THeader> headers = {
      {"Authorization", "Bearer " + iTwinAccessToken},
      {"Accept", "application/vnd.bentley.itwin-platform.v1+json"}};

  return externals.pAssetAccessor
      ->get(
          externals.asyncSystem,
          std::string{realityMetadataUri.toString()},
          headers)
      .thenImmediately([externals,
                        realityDataId,
                        iTwinId,
                        iTwinAccessToken,
                        tokenRefreshCallback = std::move(tokenRefreshCallback),
                        ellipsoid](std::shared_ptr<CesiumAsync::IAssetRequest>&&
                                       pRequest) mutable {
        const CesiumAsync::IAssetResponse* pResponse = pRequest->response();
        const std::string& requestUrl = pRequest->url();
        if (!pResponse) {
          TilesetContentLoaderResult<ITwinRealityDataContentLoader> result;
          result.errors.emplaceError(fmt::format(
              "No response received for reality data metadata request {}",
              requestUrl));
          return externals.asyncSystem.createResolvedFuture(std::move(result));
        }

        uint16_t statusCode = pResponse->statusCode();
        if (statusCode < 200 || statusCode >= 300) {
          TilesetContentLoaderResult<ITwinRealityDataContentLoader> result;
          result.errors.emplaceError(fmt::format(
              "Received status code {} for reality data metadata response {}",
              statusCode,
              requestUrl));
          result.statusCode = statusCode;
          parseITwinErrorResponseIntoErrorList(*pResponse, result.errors);
          return externals.asyncSystem.createResolvedFuture(std::move(result));
        }

        const std::span<const std::byte> data = pResponse->data();

        rapidjson::Document metadataResponse;
        metadataResponse.Parse(
            reinterpret_cast<const char*>(data.data()),
            data.size());

        const std::optional<RealityDataDetails> details =
            parseRealityDataDetails(metadataResponse);
        if (!details || details->type == RealityDataType::Unsupported) {
          TilesetContentLoaderResult<ITwinRealityDataContentLoader> result;
          result.errors.emplaceError(fmt::format(
              "No 3D Tiles reality data found for id {}",
              realityDataId));
          return externals.asyncSystem.createResolvedFuture(std::move(result));
        }

        return requestRealityDataContainer(
            externals,
            *details,
            iTwinId,
            iTwinAccessToken,
            std::move(tokenRefreshCallback),
            ellipsoid);
      });
}
CesiumAsync::Future<TileLoadResult>
ITwinRealityDataContentLoader::loadTileContent(const TileLoadInput& loadInput) {
  if (this->_pTilesetAccessor == nullptr) {
    this->_pTilesetAccessor = loadInput.pAssetAccessor;
    this->_pRealityDataAccessor = std::make_shared<RealityDataAssetAccessor>(
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
      this->_pRealityDataAccessor,
      this->_pLogger,
      loadInput.requestHeaders,
      loadInput.ellipsoid);

  return this->_pAggregatedLoader->loadTileContent(aggregatedInput);
}
TileChildrenResult ITwinRealityDataContentLoader::createTileChildren(
    const Tile& tile,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  auto pLoader = tile.getLoader();
  return pLoader->createTileChildren(tile, ellipsoid);
}

ITwinRealityDataContentLoader::ITwinRealityDataContentLoader(
    const std::string& accessToken,
    TokenRefreshCallback&& tokenRefreshCallback,
    std::unique_ptr<TilesetContentLoader>&& pAggregatedLoader)
    : _pAggregatedLoader(std::move(pAggregatedLoader)),
      _iTwinAccessToken(accessToken),
      _tokenRefreshCallback(std::move(tokenRefreshCallback)) {}

ITwinRealityDataContentLoader::~ITwinRealityDataContentLoader() {
  this->_pRealityDataAccessor->notifyLoaderIsBeingDestroyed();
}

CesiumAsync::Future<std::string>
ITwinRealityDataContentLoader::obtainNewAccessToken() {
  return this->_tokenRefreshCallback(this->_iTwinAccessToken)
      .thenInMainThread(
          [this](CesiumUtility::Result<std::string>&& result) -> std::string {
            if (!result.value) {
              result.errors.logError(
                  this->_pLogger ? this->_pLogger : spdlog::default_logger(),
                  "Errors while trying to obtain new iTwin access token:");
              return {};
            }

            this->_iTwinAccessToken = *result.value;

            return *result.value;
          });
}
} // namespace Cesium3DTilesSelection
