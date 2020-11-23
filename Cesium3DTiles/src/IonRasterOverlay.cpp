#include "Cesium3DTiles/BingMapsRasterOverlay.h"
#include "Cesium3DTiles/IAssetAccessor.h"
#include "Cesium3DTiles/IAssetResponse.h"
#include "Cesium3DTiles/IonRasterOverlay.h"
#include "Cesium3DTiles/RasterOverlayTile.h"
#include "Cesium3DTiles/RasterOverlayTileProvider.h"
#include "Cesium3DTiles/TileMapServiceRasterOverlay.h"
#include "Cesium3DTiles/TilesetExternals.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumGeospatial/WebMercatorProjection.h"
#include "CesiumUtility/Json.h"
#include "Uri.h"
#include "Logging.h"

namespace Cesium3DTiles {

    IonRasterOverlay::IonRasterOverlay(
        uint32_t ionAssetID,
        const std::string& ionAccessToken
    ) :
        _ionAssetID(ionAssetID),
        _ionAccessToken(ionAccessToken),
        _aggregatedOverlay(nullptr)
    {
    }

    IonRasterOverlay::~IonRasterOverlay() {
    }

    void IonRasterOverlay::createTileProvider(const TilesetExternals& externals, RasterOverlay* pOwner, std::function<CreateTileProviderCallback>&& callback) {
        std::string ionUrl = "https://api.cesium.com/v1/assets/" + std::to_string(this->_ionAssetID) + "/endpoint";
        ionUrl = Uri::addQuery(ionUrl, "access_token", this->_ionAccessToken);
        this->_pMetadataRequest = externals.pAssetAccessor->requestAsset(ionUrl);
        this->_pMetadataRequest->bind([this, &externals, pOwner, callback](IAssetRequest* pRequest) mutable {
            IAssetResponse* pResponse = pRequest->response();

            // TODO Can the fact that this returns and leaves the "_aggregatedOverlay"
            // initialized in some cases lead to errors? (If not: Why is the 
            // _aggregatedOverlay stored as a member?)

            using namespace nlohmann;
            json response;
            try
            {
                response = json::parse(pResponse->data().begin(), pResponse->data().end());
            }
            catch (const json::parse_error& error)
            {
                LOG_ERROR("Error when parsing ion raster overlay metadata JSON: %s", error.what());
                callback(nullptr);
                return;
            }

            std::string type = response.value("type", "unknown");
            if (type != "IMAGERY") {
                // TODO: report invalid imagery type.
                LOG_ERROR("Ion raster overlay metadata response type is not 'IMAGERY', but %s", type);
                callback(nullptr);
                return;
            }

            std::string externalType = response.value("externalType", "unknown");
            if (externalType == "BING") {
                json::iterator optionsIt = response.find("options");
                if (optionsIt == response.end()) {
                    // TODO: report incomplete Bing options
                    LOG_ERROR("Ion raster overlay metadata response does not contain 'options'");
                    callback(nullptr);
                    return;
                }

                // TODO Do these values have to be error-checked in any way?
                json options = *optionsIt;
                std::string url = options.value("url", "");
                std::string key = options.value("key", "");
                std::string mapStyle = options.value("mapStyle", "AERIAL");
                std::string culture = options.value("culture", "");

                this->_aggregatedOverlay = std::make_unique<BingMapsRasterOverlay>(
                    url,
                    key,
                    mapStyle,
                    culture
                );
            } else {
                std::string url = response.value("url", "");
                this->_aggregatedOverlay = std::make_unique<TileMapServiceRasterOverlay>(
                    url,
                    std::vector<IAssetAccessor::THeader> {
                        std::make_pair("Authorization", "Bearer " + response.value("accessToken", ""))
                    }
                );
            }

            this->_aggregatedOverlay->createTileProvider(externals, pOwner ? pOwner : this, std::move(callback));
        });
    }

}
