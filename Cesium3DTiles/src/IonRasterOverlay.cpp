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

namespace Cesium3DTiles {

    class IonRasterOverlayProvider : public RasterOverlayTileProvider {
    public:
        IonRasterOverlayProvider(
            IonRasterOverlay* pOverlay,
            std::unique_ptr<RasterOverlayTileProvider>&& pAggregate
        ) :
            RasterOverlayTileProvider(
                pOverlay,
                pAggregate->getExternals(),
                pAggregate->getProjection(),
                pAggregate->getTilingScheme(),
                pAggregate->getCoverageRectangle(),
                pAggregate->getMinimumLevel(),
                pAggregate->getMaximumLevel(),
                pAggregate->getWidth(),
                pAggregate->getHeight()
            ),
            _pAggregate(std::move(pAggregate))
        {
        }

        virtual std::shared_ptr<RasterOverlayTile> requestNewTile(const CesiumGeometry::QuadtreeTileID& tileID, RasterOverlayTileProvider* pOwner) override {
            return this->_pAggregate->getTile(tileID, pOwner ? pOwner : this);
        }

    private:
        std::unique_ptr<RasterOverlayTileProvider> _pAggregate;
    };

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

    void IonRasterOverlay::createTileProvider(TilesetExternals& tilesetExternals, std::function<IonRasterOverlay::CreateTileProviderCallback>&& callback) {
        std::string ionUrl = "https://api.cesium.com/v1/assets/" + std::to_string(this->_ionAssetID) + "/endpoint";
        ionUrl = Uri::addQuery(ionUrl, "access_token", this->_ionAccessToken);
        this->_pMetadataRequest = tilesetExternals.pAssetAccessor->requestAsset(ionUrl);
        this->_pMetadataRequest->bind([this, &tilesetExternals, callback](IAssetRequest* pRequest) mutable {
            IAssetResponse* pResponse = pRequest->response();

            using namespace nlohmann;
            
            json response = json::parse(pResponse->data().begin(), pResponse->data().end());
            if (response.value("type", "unknown") != "IMAGERY") {
                // TODO: report invalid imagery type.
                callback(nullptr);
                return;
            }

            std::string externalType = response.value("externalType", "unknown");
            if (externalType == "BING") {
                json::iterator optionsIt = response.find("options");
                if (optionsIt == response.end()) {
                    // TODO: report incomplete Bing options
                    callback(nullptr);
                    return;
                }

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

            this->_aggregatedOverlay->createTileProvider(tilesetExternals, [this, callback](std::unique_ptr<RasterOverlayTileProvider> pTileProvider) {
                callback(std::move(std::make_unique<IonRasterOverlayProvider>(this, std::move(pTileProvider))));
            });
        });
    }

}
