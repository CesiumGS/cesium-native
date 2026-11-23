#include "CesiumIonTilesetLoader.h"

#include "LayerJsonTerrainLoader.h"
#include "TilesetGltfLoader.h"
#include "TilesetJsonLoader.h"

#include <Cesium3DTiles/FileFormat.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileLoadResult.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetContentLoaderResult.h>
#include <Cesium3DTilesSelection/TilesetContentOptions.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <CesiumAsync/CesiumIonAssetAccessor.h>
#include <CesiumAsync/Future.h>
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

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Cesium3DTilesSelection {

using namespace Cesium3DTiles;

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

struct IonLoaderParameters {
  std::string url;
  std::string accessToken;
  CesiumIonTilesetLoader::AuthorizationHeaderChangeListener
      headerChangeListener;
  std::vector<LoaderCreditResult> credits;
};

std::unordered_map<std::string, AssetEndpoint> endpointCache;

CesiumAsync::Future<TilesetContentLoaderResult<CesiumIonTilesetLoader>>
createAndWrapTilesetGltfLoader(
    CesiumGltfReader::GltfReaderResult&& gltfResult,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<spdlog::logger>& pLogger,
    std::string endpointUrl,
    IonLoaderParameters ionLoaderParameters,
    std::vector<CesiumAsync::IAssetAccessor::THeader> requestHeaders,
    const CesiumAsync::HttpHeaders& loaderHeaders,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  if (!gltfResult.model) {
    std::string error = "Errors while trying to read glTF: ";
    for (const std::string& e : gltfResult.errors) {
      error += "- " + e + "\n";
    }
    TilesetContentLoaderResult<CesiumIonTilesetLoader> result;
    result.errors.emplaceError(error);
    return asyncSystem.createResolvedFuture(std::move(result));
  }

  return TilesetGltfLoader::createLoader(
             asyncSystem,
             pAssetAccessor,
             pLogger,
             endpointUrl,
             loaderHeaders,
             std::move(*gltfResult.model),
             ellipsoid)
      .thenImmediately([ionLoaderParameters = std::move(ionLoaderParameters),
                        requestHeaders,
                        ellipsoid](
                           TilesetContentLoaderResult<TilesetGltfLoader>&&
                               tilesetGltfResult) mutable {
        if (tilesetGltfResult.credits.empty()) {
          tilesetGltfResult.credits = std::move(ionLoaderParameters.credits);
        } else {
          tilesetGltfResult.credits.insert(
              tilesetGltfResult.credits.end(),
              ionLoaderParameters.credits.begin(),
              ionLoaderParameters.credits.end());
        }

        TilesetContentLoaderResult<CesiumIonTilesetLoader> result;
        if (!tilesetGltfResult.errors) {
          result.pLoader = std::make_unique<CesiumIonTilesetLoader>(
              std::move(ionLoaderParameters.url),
              std::move(ionLoaderParameters.accessToken),
              std::move(tilesetGltfResult.pLoader),
              std::move(ionLoaderParameters.headerChangeListener),
              ellipsoid);
          result.pRootTile = std::move(tilesetGltfResult.pRootTile);
          result.credits = std::move(tilesetGltfResult.credits);
          result.requestHeaders = std::move(requestHeaders);
        }
        result.errors = std::move(tilesetGltfResult.errors);
        result.statusCode = tilesetGltfResult.statusCode;
        return result;
      });
}

CesiumAsync::Future<TilesetContentLoaderResult<CesiumIonTilesetLoader>>
createAndWrapTilesetJsonLoader(
    rapidjson::Document&& tilesetJson,
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<spdlog::logger>& pLogger,
    std::string endpointUrl,
    IonLoaderParameters ionLoaderParameters,
    std::vector<CesiumAsync::IAssetAccessor::THeader> requestHeaders,
    const CesiumAsync::HttpHeaders& loaderHeaders,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  return TilesetJsonLoader::createLoader(
             asyncSystem,
             pAssetAccessor,
             pLogger,
             endpointUrl,
             loaderHeaders,
             std::move(tilesetJson),
             ellipsoid)
      .thenImmediately([requestHeaders,
                        ionLoaderParameters = std::move(ionLoaderParameters),
                        ellipsoid](
                           TilesetContentLoaderResult<TilesetJsonLoader>&&
                               tilesetJsonResult) mutable {
        if (tilesetJsonResult.credits.empty()) {
          tilesetJsonResult.credits = std::move(ionLoaderParameters.credits);
        } else {
          tilesetJsonResult.credits.insert(
              tilesetJsonResult.credits.end(),
              ionLoaderParameters.credits.begin(),
              ionLoaderParameters.credits.end());
        }

        TilesetContentLoaderResult<CesiumIonTilesetLoader> result;
        if (!tilesetJsonResult.errors) {
          result.pLoader = std::make_unique<CesiumIonTilesetLoader>(
              std::move(ionLoaderParameters.url),
              std::move(ionLoaderParameters.accessToken),
              std::move(tilesetJsonResult.pLoader),
              std::move(ionLoaderParameters.headerChangeListener),
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
mainThreadLoad3dTilesFromAssetEndpoint(
    const TilesetExternals& externals,
    const TilesetContentOptions& contentOptions,
    const AssetEndpoint& endpoint,
    std::string ionUrl,
    std::string ionAccessToken,
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
  if (!endpoint.accessToken.empty()) {
    requestHeaders.emplace_back(
        "Authorization",
        "Bearer " + endpoint.accessToken);
  }

  return externals.pAssetAccessor
      ->get(externals.asyncSystem, endpoint.url, requestHeaders)
      .thenInWorkerThread([ellipsoid,
                           contentOptions,
                           pLogger = externals.pLogger,
                           asyncSystem = externals.asyncSystem,
                           pAssetAccessor = externals.pAssetAccessor,
                           credits = std::move(credits),
                           requestHeaders,
                           ionAccessToken = std::move(ionAccessToken),
                           ionUrl = std::move(ionUrl),
                           headerChangeListener =
                               std::move(headerChangeListener)](
                              const std::shared_ptr<CesiumAsync::IAssetRequest>&
                                  pCompletedRequest) {
        const CesiumAsync::IAssetResponse* pResponse =
            pCompletedRequest->response();
        const std::string& requestUrl = pCompletedRequest->url();
        if (!pResponse) {
          TilesetContentLoaderResult<CesiumIonTilesetLoader> result;
          result.errors.emplaceError(fmt::format(
              "Did not receive a valid response for requested content {}",
              requestUrl));
          return asyncSystem.createResolvedFuture(std::move(result));
        }

        uint16_t statusCode = pResponse->statusCode();
        if (statusCode != 0 && (statusCode < 200 || statusCode >= 300)) {
          TilesetContentLoaderResult<CesiumIonTilesetLoader> result;
          result.errors.emplaceError(fmt::format(
              "Received status code {} for requested content {}",
              statusCode,
              requestUrl));
          result.statusCode = statusCode;
          return asyncSystem.createResolvedFuture(std::move(result));
        }

        IonLoaderParameters ionLoaderParameters{
            std::move(ionUrl),
            std::move(ionAccessToken),
            std::move(headerChangeListener),
            std::move(credits)};
        const std::span<const std::byte>& responseData = pResponse->data();

        // Determine what format this is in to apply the correct reader
        // and loader.
        FileFormat::Type type = FileFormat::getBestFittingFormat(responseData);

        if (type == FileFormat::Type::GltfBinary) {
          CesiumGltfReader::GltfReader gltfReader;
          CesiumGltfReader::GltfReaderResult gltfResult = gltfReader.readGltf(
              responseData,
              contentOptions.toGltfReaderOptions());
          return createAndWrapTilesetGltfLoader(
              std::move(gltfResult),
              asyncSystem,
              pAssetAccessor,
              pLogger,
              requestUrl,
              std::move(ionLoaderParameters),
              requestHeaders,
              pCompletedRequest->headers(),
              ellipsoid);
        }

        // Fall back on JSON and find the best fitting type.
        rapidjson::Document json;
        json.Parse(
            reinterpret_cast<const char*>(responseData.data()),
            responseData.size());
        if (json.HasParseError()) {
          TilesetContentLoaderResult<CesiumIonTilesetLoader> result;
          result.errors.emplaceError(fmt::format(
              "Error when parsing tileset JSON, error code {} at byte offset "
              "{}",
              json.GetParseError(),
              json.GetErrorOffset()));
          return asyncSystem.createResolvedFuture(std::move(result));
        }

        type = FileFormat::getBestFittingFormat(json);

        if (type == FileFormat::Type::GltfJson) {
          CesiumGltfReader::GltfReader gltfReader;
          return gltfReader
              .readGltfAndExternalData(
                  json,
                  asyncSystem,
                  pCompletedRequest->headers(),
                  pAssetAccessor,
                  requestUrl,
                  contentOptions.toGltfReaderOptions())
              .thenImmediately(
                  [asyncSystem,
                   pAssetAccessor,
                   pLogger,
                   requestUrl,
                   ionLoaderParameters = std::move(ionLoaderParameters),
                   requestHeaders,
                   loaderHeaders = pCompletedRequest->headers(),
                   ellipsoid](CesiumGltfReader::GltfReaderResult&& gltfResult) {
                    return createAndWrapTilesetGltfLoader(
                        std::move(gltfResult),
                        asyncSystem,
                        pAssetAccessor,
                        pLogger,
                        requestUrl,
                        std::move(ionLoaderParameters),
                        requestHeaders,
                        loaderHeaders,
                        ellipsoid);
                  });
        }

        if (type == FileFormat::Type::TilesetJson) {
          return createAndWrapTilesetJsonLoader(
              std::move(json),
              asyncSystem,
              pAssetAccessor,
              pLogger,
              requestUrl,
              std::move(ionLoaderParameters),
              requestHeaders,
              pCompletedRequest->headers(),
              ellipsoid);
        }

        TilesetContentLoaderResult<CesiumIonTilesetLoader> result;
        result.errors.emplaceError(fmt::format(
            "Unsupported type for requested 3D Tiles content {}",
            statusCode,
            requestUrl));
        result.statusCode = statusCode;
        return asyncSystem.createResolvedFuture(std::move(result));
      });
}

CesiumAsync::Future<TilesetContentLoaderResult<CesiumIonTilesetLoader>>
mainThreadLoadLayerJsonFromAssetEndpoint(
    const TilesetExternals& externals,
    const TilesetContentOptions& contentOptions,
    const AssetEndpoint& endpoint,
    std::string ionUrl,
    std::string ionAccessToken,
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
  if (!endpoint.accessToken.empty()) {
    requestHeaders.emplace_back(
        "Authorization",
        "Bearer " + endpoint.accessToken);
  }

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
                        ionUrl,
                        ionAccessToken = std::move(ionAccessToken),
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
              std::move(ionUrl),
              std::move(ionAccessToken),
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
    std::string&& ionUrl,
    std::string&& ionAccessToken,
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
        "Error when parsing Cesium ion response JSON, error code {} at "
        "byte "
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
        std::move(ionUrl),
        std::move(ionAccessToken),
        std::move(headerChangeListener),
        showCreditsOnScreen,
        ellipsoid);
  } else if (type == "3DTILES") {
    endpoint.type = type;
    endpoint.url = url;
    endpoint.accessToken = accessToken;
    endpointCache[requestUrl] = endpoint;
    return mainThreadLoad3dTilesFromAssetEndpoint(
        externals,
        contentOptions,
        endpoint,
        std::move(ionUrl),
        std::move(ionAccessToken),
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

CesiumIonTilesetLoader::~CesiumIonTilesetLoader() noexcept {
  if (this->_pIonAccessor) {
    this->_pIonAccessor->notifyOwnerIsBeingDestroyed();
  }
}

CesiumAsync::Future<TileLoadResult>
CesiumIonTilesetLoader::loadTileContent(const TileLoadInput& loadInput) {
  this->_pLogger = loadInput.pLogger;

  if (this->_pTilesetAccessor == nullptr) {
    this->_pTilesetAccessor = loadInput.pAssetAccessor;
    this->_pIonAccessor = std::make_shared<CesiumAsync::CesiumIonAssetAccessor>(
        this->_pLogger,
        this->_pTilesetAccessor,
        this->_url,
        loadInput.requestHeaders,
        [this, asyncSystem = loadInput.asyncSystem](
            const CesiumAsync::CesiumIonAssetAccessor::UpdatedToken& update) {
          this->_headerChangeListener(
              "Authorization",
              update.authorizationHeader);

          // update cache with new access token
          auto cacheIt = endpointCache.find(this->_url);
          if (cacheIt != endpointCache.end()) {
            cacheIt->second.accessToken = update.token;
          }

          return asyncSystem.createResolvedFuture();
        });
  }

  if (this->_pTilesetAccessor != loadInput.pAssetAccessor) {
    // CesiumIonTilesetLoader requires this method to be called with the same
    // asset accessor instance every time.
    CESIUM_ASSERT(false);
    return loadInput.asyncSystem.createResolvedFuture(
        TileLoadResult::createFailedResult(loadInput.pAssetAccessor, nullptr));
  }

  TileLoadInput aggregatedInput(
      loadInput.tile,
      loadInput.contentOptions,
      loadInput.asyncSystem,
      this->_pIonAccessor,
      this->_pLogger,
      loadInput.requestHeaders,
      loadInput.ellipsoid);
  aggregatedInput.pSharedAssetSystem = loadInput.pSharedAssetSystem;

  return this->_pAggregatedLoader->loadTileContent(aggregatedInput);
}

TileChildrenResult CesiumIonTilesetLoader::createTileChildren(
    const Tile& tile,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  auto pLoader = tile.getLoader();
  return pLoader->createTileChildren(tile, ellipsoid);
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
  return CesiumIonTilesetLoader::createLoader(
      externals,
      contentOptions,
      fmt::format(
          "{}v1/assets/{}/endpoint?access_token={}",
          ionAssetEndpointUrl,
          ionAssetID,
          ionAccessToken),
      ionAccessToken,
      false,
      headerChangeListener,
      showCreditsOnScreen,
      ellipsoid);
}

CesiumAsync::Future<TilesetContentLoaderResult<CesiumIonTilesetLoader>>
CesiumIonTilesetLoader::createLoader(
    const TilesetExternals& externals,
    const TilesetContentOptions& contentOptions,
    const std::string& url,
    const std::string& ionAccessToken,
    bool needsAuthHeader,
    const AuthorizationHeaderChangeListener& headerChangeListener,
    bool showCreditsOnScreen,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  auto cacheIt = endpointCache.find(url);
  if (cacheIt != endpointCache.end()) {
    const auto& endpoint = cacheIt->second;
    if (endpoint.type == "TERRAIN") {
      return mainThreadLoadLayerJsonFromAssetEndpoint(
                 externals,
                 contentOptions,
                 endpoint,
                 url,
                 ionAccessToken,
                 headerChangeListener,
                 showCreditsOnScreen,
                 ellipsoid)
          .thenInMainThread(
              [externals,
               ellipsoid,
               contentOptions,
               ionAccessToken,
               url,
               needsAuthHeader,
               headerChangeListener,
               showCreditsOnScreen](
                  TilesetContentLoaderResult<CesiumIonTilesetLoader>&&
                      result) mutable {
                return refreshTokenIfNeeded(
                    externals,
                    contentOptions,
                    url,
                    ionAccessToken,
                    needsAuthHeader,
                    headerChangeListener,
                    showCreditsOnScreen,
                    std::move(result),
                    ellipsoid);
              });
    } else if (endpoint.type == "3DTILES") {
      return mainThreadLoad3dTilesFromAssetEndpoint(
                 externals,
                 contentOptions,
                 endpoint,
                 url,
                 ionAccessToken,
                 headerChangeListener,
                 showCreditsOnScreen,
                 ellipsoid)
          .thenInMainThread(
              [externals,
               contentOptions,
               url,
               ionAccessToken,
               needsAuthHeader,
               headerChangeListener,
               showCreditsOnScreen,
               ellipsoid](TilesetContentLoaderResult<CesiumIonTilesetLoader>&&
                              result) mutable {
                return refreshTokenIfNeeded(
                    externals,
                    contentOptions,
                    url,
                    ionAccessToken,
                    needsAuthHeader,
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
    std::vector<CesiumAsync::IAssetAccessor::THeader> headers;
    if (needsAuthHeader) {
      headers.emplace_back("Authorization", "Bearer " + ionAccessToken);
    }

    return externals.pAssetAccessor->get(externals.asyncSystem, url, headers)
        .thenInMainThread(
            [externals,
             ellipsoid,
             url = url,
             ionAccessToken = ionAccessToken,
             headerChangeListener = headerChangeListener,
             showCreditsOnScreen,
             contentOptions](std::shared_ptr<CesiumAsync::IAssetRequest>&&
                                 pRequest) mutable {
              return mainThreadHandleEndpointResponse(
                  externals,
                  std::move(pRequest),
                  std::move(url),
                  std::move(ionAccessToken),
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
    const std::string& url,
    const std::string& ionAccessToken,
    bool needsAuthHeader,
    const AuthorizationHeaderChangeListener& headerChangeListener,
    bool showCreditsOnScreen,
    TilesetContentLoaderResult<CesiumIonTilesetLoader>&& result,
    const CesiumGeospatial::Ellipsoid& ellipsoid) {
  if (result.errors.hasErrors()) {
    if (result.statusCode == 401) {
      endpointCache.erase(url);
      return CesiumIonTilesetLoader::createLoader(
          externals,
          contentOptions,
          url,
          ionAccessToken,
          needsAuthHeader,
          headerChangeListener,
          showCreditsOnScreen,
          ellipsoid);
    }
  }
  return externals.asyncSystem.createResolvedFuture(std::move(result));
}

void CesiumIonTilesetLoader::setOwnerOfNestedLoaders(
    TilesetContentManager& owner) noexcept {
  if (this->_pAggregatedLoader) {
    this->_pAggregatedLoader->setOwner(owner);
  }
}

CesiumIonTilesetLoader::CesiumIonTilesetLoader(
    std::string&& url,
    std::string&& ionAccessToken,
    std::unique_ptr<TilesetContentLoader>&& pAggregatedLoader,
    AuthorizationHeaderChangeListener&& headerChangeListener,
    const CesiumGeospatial::Ellipsoid& ellipsoid)
    : _ellipsoid(ellipsoid),
      _url(std::move(url)),
      _ionAccessToken(std::move(ionAccessToken)),
      _pAggregatedLoader(std::move(pAggregatedLoader)),
      _headerChangeListener(std::move(headerChangeListener)),
      _pLogger(nullptr),
      _pTilesetAccessor(nullptr),
      _pIonAccessor(nullptr) {}

} // namespace Cesium3DTilesSelection
