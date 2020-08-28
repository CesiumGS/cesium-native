#include "Cesium3DTiles/BingMapsRasterOverlay.h"
#include "Cesium3DTiles/IAssetAccessor.h"
#include "Cesium3DTiles/IAssetResponse.h"
#include "Cesium3DTiles/RasterOverlayTileProvider.h"
#include "CesiumGeospatial/WebMercatorProjection.h"
#include "CesiumGeospatial/GlobeRectangle.h"
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
            const std::string& urlTemplate,
            const std::vector<std::string>& subdomains,
            uint32_t width,
            uint32_t height,
            uint32_t minimumLevel,
            uint32_t maximumLevel
        ) :
            RasterOverlayTileProvider(
                CesiumGeospatial::WebMercatorProjection(),
                CesiumGeospatial::QuadtreeTilingScheme(
                    CesiumGeospatial::WebMercatorProjection::MAXIMUM_RECTANGLE.toSimpleRectangle(),
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

        virtual std::unique_ptr<RasterOverlayTileRequest> requestImage(const CesiumGeometry::QuadtreeTileID& id, IAssetAccessor& accessor) override {
            std::string url = Uri::substituteTemplateParameters(this->_urlTemplate, [this, &id](const std::string& key) {
                if (key == "quadkey") {
                    return BingMapsTileProvider::tileXYToQuadKey(id.level, id.x, id.y);
                } else if (key == "subdomain") {
                    size_t subdomainIndex = (id.level + id.x + id.y) % this->_subdomains.size();
                    return this->_subdomains[subdomainIndex];
                }
                return key;
            });

            return std::make_unique<RasterOverlayTileRequest>(accessor.requestAsset(url));
        }
    
    private:
        static std::string tileXYToQuadKey(uint32_t level, uint32_t x, uint32_t y) {
            std::string quadkey = "";
            for (uint32_t i = level; i >= 0; --i) {
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
        _ellipsoid(ellipsoid)
    {
    }

    BingMapsRasterOverlay::~BingMapsRasterOverlay() {
    }

    void BingMapsRasterOverlay::createTileProvider(IAssetAccessor& assetAccessor, std::function<BingMapsRasterOverlay::CreateTileProviderCallback> callback) {
        std::string metadataUrl = Uri::resolve(this->_url, "REST/v1/Imagery/Metadata/" + this->_mapStyle, true);
        metadataUrl = Uri::addQuery(metadataUrl, "incl", "ImageryProviders");
        metadataUrl = Uri::addQuery(metadataUrl, "key", this->_key);
        metadataUrl = Uri::addQuery(metadataUrl, "uriScheme", "https");

        std::unique_ptr<IAssetRequest> pRequest = assetAccessor.requestAsset(metadataUrl);
        pRequest->bind([this, callback](IAssetRequest* pRequest) {
            IAssetResponse* pResponse = pRequest->response();

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

            // TODO: attribution

            std::string url = Uri::resolve(this->_url, urlTemplate);

            // Tell the Bing servers to send a zero-length response instead of a
            // placeholder image for missing tiles.
            url = Uri::addQuery(url, "n", "z");

            url = Uri::substituteTemplateParameters(url, [this](const std::string& key) {
                if (key == "culture") {
                    return this->_culture;
                }

                // Keep other placeholders
                return "{" + key + "}";
            });

            callback(std::make_unique<BingMapsTileProvider>(url, subdomains, width, height, 0, maximumLevel));
        });
    }
}
