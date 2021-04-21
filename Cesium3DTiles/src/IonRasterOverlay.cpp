#include "Cesium3DTiles/IonRasterOverlay.h"
#include "Cesium3DTiles/BingMapsRasterOverlay.h"
#include "Cesium3DTiles/RasterOverlayTile.h"
#include "Cesium3DTiles/RasterOverlayTileProvider.h"
#include "Cesium3DTiles/TileMapServiceRasterOverlay.h"
#include "Cesium3DTiles/TilesetExternals.h"
#include "Cesium3DTiles/spdlog-cesium.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumAsync/IAssetResponse.h"
#include "CesiumUtility/JsonHelpers.h"
#include "CesiumUtility/Uri.h"
#include <map>
#include <rapidjson/document.h>

using namespace CesiumAsync;
using namespace CesiumUtility;

namespace {
struct BingOverlayArgs {
  std::string url;
  std::string key;
  std::string mapStyle;
  std::string culture;
};

std::map<std::string, BingOverlayArgs> cachedBingImageryAssets = {};
} // namespace

namespace Cesium3DTiles {

IonRasterOverlay::IonRasterOverlay(
    uint32_t ionAssetID,
    const std::string& ionAccessToken)
    : _ionAssetID(ionAssetID), _ionAccessToken(ionAccessToken) {}

IonRasterOverlay::~IonRasterOverlay() {}

Future<std::unique_ptr<RasterOverlayTileProvider>>
IonRasterOverlay::createTileProvider(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& pCreditSystem,
    const std::shared_ptr<IPrepareRendererResources>& pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    RasterOverlay* pOwner) {
  std::string ionUrl = "https://api.cesium.com/v1/assets/" +
                       std::to_string(this->_ionAssetID) + "/endpoint";
  ionUrl = CesiumUtility::Uri::addQuery(
      ionUrl,
      "access_token",
      this->_ionAccessToken);

  pOwner = pOwner ? pOwner : this;

  auto cachedBingArgsIt = cachedBingImageryAssets.find(ionUrl);
  if (cachedBingArgsIt != cachedBingImageryAssets.end()) {
    const BingOverlayArgs& cachedBingArgs = cachedBingArgsIt->second;
    SPDLOG_LOGGER_ERROR(pLogger, "-----------REUSING BING SESSION----------");
    return std::make_unique<BingMapsRasterOverlay>(
               cachedBingArgs.url,
               cachedBingArgs.key,
               cachedBingArgs.mapStyle,
               cachedBingArgs.culture)
        ->createTileProvider(
            asyncSystem,
            pAssetAccessor,
            pCreditSystem,
            pPrepareRendererResources,
            pLogger,
            pOwner);
  }

  return pAssetAccessor->requestAsset(asyncSystem, ionUrl)
      .thenInWorkerThread(
          [pLogger, ionUrl](const std::shared_ptr<IAssetRequest>& pRequest)
              -> std::unique_ptr<RasterOverlay> {
            const IAssetResponse* pResponse = pRequest->response();

            rapidjson::Document response;
            response.Parse(
                reinterpret_cast<const char*>(pResponse->data().data()),
                pResponse->data().size());

            if (response.HasParseError()) {
              SPDLOG_LOGGER_ERROR(
                  pLogger,
                  "Error when parsing ion raster overlay response, error code "
                  "{} at byte offset {}",
                  response.GetParseError(),
                  response.GetErrorOffset());
              return nullptr;
            }

            std::string type =
                JsonHelpers::getStringOrDefault(response, "type", "unknown");
            if (type != "IMAGERY") {
              SPDLOG_LOGGER_ERROR(
                  pLogger,
                  "Ion raster overlay metadata response type is not 'IMAGERY', "
                  "but {}",
                  type);
              return nullptr;
            }

            std::string externalType = JsonHelpers::getStringOrDefault(
                response,
                "externalType",
                "unknown");
            if (externalType == "BING") {
              auto optionsIt = response.FindMember("options");
              if (optionsIt == response.MemberEnd() ||
                  !optionsIt->value.IsObject()) {
                SPDLOG_LOGGER_ERROR(
                    pLogger,
                    "Cesium ion Bing Maps raster overlay metadata response "
                    "does not contain 'options' or it is not an object.");
                return nullptr;
              }

              const auto& options = optionsIt->value;
              std::string url =
                  JsonHelpers::getStringOrDefault(options, "url", "");
              std::string key =
                  JsonHelpers::getStringOrDefault(options, "key", "");
              std::string mapStyle = JsonHelpers::getStringOrDefault(
                  options,
                  "mapStyle",
                  "AERIAL");
              std::string culture =
                  JsonHelpers::getStringOrDefault(options, "culture", "");

              cachedBingImageryAssets[ionUrl] = {url, key, mapStyle, culture};

              return std::make_unique<BingMapsRasterOverlay>(
                  url,
                  key,
                  mapStyle,
                  culture);
            }
            std::string url =
                JsonHelpers::getStringOrDefault(response, "url", "");
            return std::make_unique<TileMapServiceRasterOverlay>(
                url,
                std::vector<CesiumAsync::IAssetAccessor::THeader>{
                    std::make_pair(
                        "Authorization",
                        "Bearer " + JsonHelpers::getStringOrDefault(
                                        response,
                                        "accessToken",
                                        ""))});
          })
      .thenInMainThread([asyncSystem,
                         pOwner,
                         pAssetAccessor,
                         pCreditSystem,
                         pPrepareRendererResources,
                         pLogger](
                            std::unique_ptr<RasterOverlay> pAggregatedOverlay) {
        // Handle the case that the code above bails out with an error,
        // returning a nullptr.
        if (pAggregatedOverlay) {
          return pAggregatedOverlay->createTileProvider(
              asyncSystem,
              pAssetAccessor,
              pCreditSystem,
              pPrepareRendererResources,
              pLogger,
              pOwner);
        }
        return asyncSystem
            .createResolvedFuture<std::unique_ptr<RasterOverlayTileProvider>>(
                nullptr);
      });
}

} // namespace Cesium3DTiles
