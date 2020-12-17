#include "Cesium3DTiles/BingMapsRasterOverlay.h"
#include "Cesium3DTiles/IonRasterOverlay.h"
#include "Cesium3DTiles/RasterOverlayTile.h"
#include "Cesium3DTiles/RasterOverlayTileProvider.h"
#include "Cesium3DTiles/spdlog-cesium.h"
#include "Cesium3DTiles/TileMapServiceRasterOverlay.h"
#include "Cesium3DTiles/TilesetExternals.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumAsync/IAssetResponse.h"
#include "CesiumUtility/Json.h"
#include "Uri.h"

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

            using namespace nlohmann;
            json response;
            try {
                response = json::parse(pResponse->data().begin(), pResponse->data().end());
            } catch (const json::parse_error& error) {
                SPDLOG_LOGGER_ERROR(pLogger, "Error when parsing ion raster overlay metadata JSON: {}", error.what());
                return nullptr;
            }

            std::string type = response.value("type", "unknown");
            if (type != "IMAGERY") {
                // TODO: report invalid imagery type.
                SPDLOG_LOGGER_ERROR(pLogger, "Ion raster overlay metadata response type is not 'IMAGERY', but {}", type);
                return nullptr;
            }

            std::string externalType = response.value("externalType", "unknown");
            if (externalType == "BING") {
                json::iterator optionsIt = response.find("options");
                if (optionsIt == response.end()) {
                    // TODO: report incomplete Bing options
                    SPDLOG_LOGGER_ERROR(pLogger, "Ion raster overlay metadata response does not contain 'options'");
                    return nullptr;
                }

                json options = *optionsIt;
                std::string url = options.value("url", "");
                std::string key = options.value("key", "");
                std::string mapStyle = options.value("mapStyle", "AERIAL");
                std::string culture = options.value("culture", "");

                return std::make_unique<BingMapsRasterOverlay>(
                    url,
                    key,
                    mapStyle,
                    culture
                );
            } else {
                std::string url = response.value("url", "");
                return std::make_unique<TileMapServiceRasterOverlay>(
                    url,
                    std::vector<CesiumAsync::IAssetAccessor::THeader> {
                        std::make_pair("Authorization", "Bearer " + response.value("accessToken", ""))
                    }
                );
            }
        }).thenInMainThread([
            pOwner,
            asyncSystem,
            pPrepareRendererResources,
            pLogger
        ](std::unique_ptr<RasterOverlay> pAggregatedOverlay) {
            return pAggregatedOverlay->createTileProvider(asyncSystem, pPrepareRendererResources, pLogger, pOwner);
        });
    }

}
