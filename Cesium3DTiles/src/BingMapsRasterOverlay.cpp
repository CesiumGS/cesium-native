#include "Cesium3DTiles/BingMapsRasterOverlay.h"
#include "Cesium3DTiles/IAssetAccessor.h"
#include "Cesium3DTiles/IAssetResponse.h"
#include "Cesium3DTiles/RasterOverlayTile.h"
#include "Cesium3DTiles/RasterOverlayTileProvider.h"
#include "Cesium3DTiles/TilesetExternals.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumGeospatial/WebMercatorProjection.h"
#include "CesiumUtility/Json.h"
#include "Uri.h"

namespace Cesium3DTiles {

    const std::string BingMapsStyle::AERIAL = "Aerial";
    const std::string BingMapsStyle::AERIAL_WITH_LABELS = "AerialWithLabels";
    const std::string BingMapsStyle::AERIAL_WITH_LABELS_ON_DEMAND = "AerialWithLabelsOnDemand";
    const std::string BingMapsStyle::ROAD = "Road";
    const std::string BingMapsStyle::ROAD_ON_DEMAND = "RoadOnDemand";
    const std::string BingMapsStyle::CANVAS_DARK = "CanvasDark";
    const std::string BingMapsStyle::CANVAS_LIGHT = "CanvasLight";
    const std::string BingMapsStyle::CANVAS_GRAY = "CanvasGray";
    const std::string BingMapsStyle::ORDNANCE_SURVEY = "OrdnanceSurvey";
    const std::string BingMapsStyle::COLLINS_BART = "CollinsBart";

    class BingMapsTileProvider : public RasterOverlayTileProvider {
    public:
        BingMapsTileProvider(
            BingMapsRasterOverlay* pOverlay,
            TilesetExternals& tilesetExternals,
            const std::string& urlTemplate,
            const std::vector<std::string>& subdomains,
            uint32_t width,
            uint32_t height,
            uint32_t minimumLevel,
            uint32_t maximumLevel
        ) :
            RasterOverlayTileProvider(
                pOverlay,
                tilesetExternals,
                CesiumGeospatial::WebMercatorProjection(),
                CesiumGeometry::QuadtreeTilingScheme(
                    CesiumGeospatial::WebMercatorProjection::computeMaximumProjectedRectangle(CesiumGeospatial::Ellipsoid::WGS84),
                    2, 2
                ),
                minimumLevel,
                maximumLevel,
                width,
                height
            ),
            _urlTemplate(urlTemplate),
            _subdomains(subdomains)
        {
        }

        virtual ~BingMapsTileProvider() {}

    protected:
        virtual std::shared_ptr<RasterOverlayTile> requestNewTile(const CesiumGeometry::QuadtreeTileID& tileID) override {
            std::string url = Uri::substituteTemplateParameters(this->_urlTemplate, [this, &tileID](const std::string& key) {
                if (key == "quadkey") {
                    return BingMapsTileProvider::tileXYToQuadKey(tileID.level, tileID.x, tileID.computeInvertedY(this->getTilingScheme()));
                } else if (key == "subdomain") {
                    size_t subdomainIndex = (tileID.level + tileID.x + tileID.y) % this->_subdomains.size();
                    return this->_subdomains[subdomainIndex];
                }
                return key;
            });

            return std::make_shared<RasterOverlayTile>(*this, tileID, this->getExternals().pAssetAccessor->requestAsset(url));
        }
    
    private:
        static std::string tileXYToQuadKey(uint32_t level, uint32_t x, uint32_t y) {
            std::string quadkey = "";
            for (int32_t i = static_cast<int32_t>(level); i >= 0; --i) {
                uint32_t bitmask = 1 << i;
                uint32_t digit = 0;

                if ((x & bitmask) != 0) {
                    digit |= 1;
                }

                if ((y & bitmask) != 0) {
                    digit |= 2;
                }

                quadkey += std::to_string(digit);
            }

            return quadkey;
        }

        std::string _urlTemplate;
        std::vector<std::string> _subdomains;
    };

    BingMapsRasterOverlay::BingMapsRasterOverlay(
        const std::string& url,
        const std::string& key,
        const std::string& mapStyle,
        const std::string& culture,
        const CesiumGeospatial::Ellipsoid& ellipsoid
    ) :
        _url(url),
        _key(key),
        _mapStyle(mapStyle),
        _culture(culture),
        _ellipsoid(ellipsoid),
        _ionAssetID(0),
        _ionAccessToken("")
    {
    }

    BingMapsRasterOverlay::BingMapsRasterOverlay(
        uint32_t ionAssetID,
        const std::string& ionAccessToken
    ) :
        _url(""),
        _key(""),
        _mapStyle(BingMapsStyle::AERIAL),
        _culture(""),
        _ellipsoid(CesiumGeospatial::Ellipsoid::WGS84),
        _ionAssetID(ionAssetID),
        _ionAccessToken(ionAccessToken)
    {
    }

    BingMapsRasterOverlay::~BingMapsRasterOverlay() {
    }

    void BingMapsRasterOverlay::createTileProvider(TilesetExternals& tilesetExternals, std::function<BingMapsRasterOverlay::CreateTileProviderCallback>&& callback) {
        if (this->_ionAssetID > 0) {
            std::string ionUrl = "https://api.cesium.com/v1/assets/" + std::to_string(this->_ionAssetID) + "/endpoint";
            ionUrl = Uri::addQuery(ionUrl, "access_token", this->_ionAccessToken);
            this->_pMetadataRequest = tilesetExternals.pAssetAccessor->requestAsset(ionUrl);
            this->_pMetadataRequest->bind([this, &tilesetExternals, callback](IAssetRequest* pRequest) mutable {
                IAssetResponse* pResponse = pRequest->response();

                using namespace nlohmann;
                
                json response = json::parse(pResponse->data().begin(), pResponse->data().end());
                json::iterator optionsIt = response.find("options");
                if (
                    optionsIt == response.end() ||
                    response.value("type", "unknown") != "IMAGERY" ||
                    response.value("externalType", "unknown") != "BING"
                ) {
                    // TODO: report invalid imagery type.
                    return;
                }

                json options = *optionsIt;
                std::string url = options.value("url", "");
                std::string key = options.value("key", "");
                std::string mapStyle = options.value("mapStyle", "AERIAL");
                std::string culture = options.value("culture", "");

                this->_pMetadataRequest = BingMapsRasterOverlay::createBingProvider(
                    this,
                    tilesetExternals,
                    std::move(callback),
                    url,
                    key,
                    mapStyle,
                    culture
                );
            });
        } else {
            this->_pMetadataRequest = BingMapsRasterOverlay::createBingProvider(
                this,
                tilesetExternals,
                std::move(callback),
                this->_url,
                this->_key,
                this->_mapStyle,
                this->_culture
            );
        }
    }

    /*static*/ std::unique_ptr<IAssetRequest> BingMapsRasterOverlay::createBingProvider(
        BingMapsRasterOverlay* pOverlay,
        TilesetExternals& tilesetExternals,
        std::function<BingMapsRasterOverlay::CreateTileProviderCallback>&& callback,
        const std::string& url,
        const std::string& key,
        const std::string& mapStyle,
        const std::string& culture
    ) {
        std::string metadataUrl = Uri::resolve(url, "REST/v1/Imagery/Metadata/" + mapStyle, true);
        metadataUrl = Uri::addQuery(metadataUrl, "incl", "ImageryProviders");
        metadataUrl = Uri::addQuery(metadataUrl, "key", key);
        metadataUrl = Uri::addQuery(metadataUrl, "uriScheme", "https");

        std::unique_ptr<IAssetRequest> pRequest = tilesetExternals.pAssetAccessor->requestAsset(metadataUrl);
        pRequest->bind([pOverlay, callback, &tilesetExternals, url, culture](IAssetRequest* pCompletedRequest) {
            IAssetResponse* pResponse = pCompletedRequest->response();

            using namespace nlohmann;
            
            json response = json::parse(pResponse->data().begin(), pResponse->data().end());

            json& resource = response["/resourceSets/0/resources/0"_json_pointer];
            if (!resource.is_object()) {
                callback(nullptr);
                return;
            }

            uint32_t width = resource.value("imageWidth", 256);
            uint32_t height = resource.value("imageHeight", 256);
            uint32_t maximumLevel = resource.value("zoomMax", 30);

            std::vector<std::string> subdomains = resource.value("imageUrlSubdomains", std::vector<std::string>());
            std::string urlTemplate = resource.value("imageUrl", std::string());
            if (urlTemplate.empty())  {
                callback(nullptr);
                return;
            }

            if (urlTemplate.find("n=z") == std::string::npos) {
                urlTemplate = Uri::addQuery(urlTemplate, "n", "z");
            }

            // TODO: attribution

            std::string resolvedUrl = Uri::resolve(url, urlTemplate);

            resolvedUrl = Uri::substituteTemplateParameters(resolvedUrl, [culture](const std::string& templateKey) {
                if (templateKey == "culture") {
                    return culture;
                }

                // Keep other placeholders
                return "{" + templateKey + "}";
            });

            callback(std::make_unique<BingMapsTileProvider>(pOverlay, tilesetExternals, resolvedUrl, subdomains, width, height, 0, maximumLevel));
        });

        return pRequest;
    }

}
