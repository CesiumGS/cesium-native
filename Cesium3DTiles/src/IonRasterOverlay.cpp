#include "Cesium3DTiles/BingMapsRasterOverlay.h"
#include "Cesium3DTiles/IonRasterOverlay.h"
#include "Cesium3DTiles/RasterOverlayTile.h"
#include "Cesium3DTiles/RasterOverlayTileProvider.h"
#include "Cesium3DTiles/spdlog-cesium.h"
#include "Cesium3DTiles/TileMapServiceRasterOverlay.h"
#include "Cesium3DTiles/TilesetExternals.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumAsync/IAssetResponse.h"
#include "Uri.h"
#include "JsonHelpers.h"
#include <rapidjson/document.h>

using namespace CesiumAsync;

namespace Cesium3DTiles {

    IonRasterOverlay::IonRasterOverlay(
        uint32_t ionAssetID,
        const std::string& ionAccessToken
    ) :
        _ionAssetID(ionAssetID),
        _ionAccessToken(ionAccessToken)
    {
    }

    IonRasterOverlay::~IonRasterOverlay() {
    }

    Future<std::unique_ptr<RasterOverlayTileProvider>> IonRasterOverlay::createTileProvider(
        const AsyncSystem& asyncSystem,
        const std::shared_ptr<CreditSystem>& pCreditSystem,
        std::shared_ptr<IPrepareRendererResources> pPrepareRendererResources,
        std::shared_ptr<spdlog::logger> pLogger,
        RasterOverlay* pOwner
    ) {
        std::string ionUrl = "https://api.cesium.com/v1/assets/" + std::to_string(this->_ionAssetID) + "/endpoint";
        ionUrl = Uri::addQuery(ionUrl, "access_token", this->_ionAccessToken);

        pOwner = pOwner ? pOwner : this;

        return asyncSystem.requestAsset(ionUrl).thenInWorkerThread([
            pLogger
        ](
            std::unique_ptr<IAssetRequest> pRequest
        ) -> std::unique_ptr<RasterOverlay> {
            IAssetResponse* pResponse = pRequest->response();

            rapidjson::Document response;
            response.Parse(reinterpret_cast<const char*>(pResponse->data().data()), pResponse->data().size());

            if (response.HasParseError()) {
                SPDLOG_LOGGER_ERROR(pLogger, "Error when parsing ion raster overlay response, error code {} at byte offset {}", response.GetParseError(), response.GetErrorOffset());
                return nullptr;
            }

            std::string type = JsonHelpers::getStringOrDefault(response, "type", "unknown");
            if (type != "IMAGERY") {
                // TODO: report invalid imagery type.
                SPDLOG_LOGGER_ERROR(pLogger, "Ion raster overlay metadata response type is not 'IMAGERY', but {}", type);
                return nullptr;
            }

            std::string externalType = JsonHelpers::getStringOrDefault(response, "externalType", "unknown");
            if (externalType == "BING") {
                auto optionsIt = response.FindMember("options");
                if (optionsIt == response.MemberEnd() || !optionsIt->value.IsObject()) {
                    // TODO: report incomplete Bing options
                    SPDLOG_LOGGER_ERROR(pLogger, "Cesium ion Bing Maps raster overlay metadata response does not contain 'options' or it is not an object.");
                    return nullptr;
                }

                const auto& options = optionsIt->value;
                std::string url = JsonHelpers::getStringOrDefault(options, "url", "");
                std::string key = JsonHelpers::getStringOrDefault(options, "key", "");
                std::string mapStyle = JsonHelpers::getStringOrDefault(options, "mapStyle", "AERIAL");
                std::string culture = JsonHelpers::getStringOrDefault(options, "culture", "");

                return std::make_unique<BingMapsRasterOverlay>(
                    url,
                    key,
                    mapStyle,
                    culture
                );
            } else {
                std::string url = JsonHelpers::getStringOrDefault(response, "url", "");
                return std::make_unique<TileMapServiceRasterOverlay>(
                    url,
                    std::vector<CesiumAsync::IAssetAccessor::THeader> {
                        std::make_pair("Authorization", "Bearer " + JsonHelpers::getStringOrDefault(response, "accessToken", ""))
                    }
                );
            }
        }).thenInMainThread([
            pOwner,
            asyncSystem,
            pCreditSystem,
            pPrepareRendererResources,
            pLogger
        ](std::unique_ptr<RasterOverlay> pAggregatedOverlay) {
            if (pAggregatedOverlay) {
                return pAggregatedOverlay->createTileProvider(asyncSystem, pCreditSystem, pPrepareRendererResources, pLogger, pOwner);
            } else {
                return asyncSystem.createResolvedFuture<std::unique_ptr<RasterOverlayTileProvider>>(nullptr);
            }
        });
    }

}
