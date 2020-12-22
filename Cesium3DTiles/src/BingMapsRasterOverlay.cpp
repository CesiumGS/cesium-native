#include "Cesium3DTiles/BingMapsRasterOverlay.h"
#include "Cesium3DTiles/RasterOverlayTile.h"
#include "Cesium3DTiles/RasterOverlayTile.h"
#include "Cesium3DTiles/RasterOverlayTileProvider.h"
#include "Cesium3DTiles/spdlog-cesium.h"
#include "Cesium3DTiles/TilesetExternals.h"
#include "Cesium3DTiles/CreditSystem.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumAsync/IAssetResponse.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumGeospatial/Projection.h"
#include "CesiumGeospatial/WebMercatorProjection.h"
#include "CesiumUtility/Math.h"
#include "Uri.h"
#include "JsonHelpers.h"
#include <rapidjson/document.h>
#include <rapidjson/pointer.h>
#include <vector>
#include <optional>
#include <utility>

namespace {
    struct BingMapsCreditCoverageArea {
        CesiumGeospatial::GlobeRectangle rectangle;
        uint32_t zoomMin;
        uint32_t zoomMax;
    };
}

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;

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

    class BingMapsTileProvider final : public RasterOverlayTileProvider {
    public:
        BingMapsTileProvider(
            RasterOverlay& owner,
            const AsyncSystem& asyncSystem,
            Credit bingCredit,
            const std::vector<std::pair<Credit, std::vector<BingMapsCreditCoverageArea>>>& perTileCredits,
            std::shared_ptr<IPrepareRendererResources> pPrepareRendererResources,
            const std::string& baseUrl,
            const std::string& urlTemplate,
            const std::vector<std::string>& subdomains,
            uint32_t width,
            uint32_t height,
            uint32_t minimumLevel,
            uint32_t maximumLevel,
            const std::string& culture
        ) :
            RasterOverlayTileProvider(
                owner,
                asyncSystem,
                std::make_optional(bingCredit),
                pPrepareRendererResources,
                WebMercatorProjection(),
                QuadtreeTilingScheme(
                    WebMercatorProjection::computeMaximumProjectedRectangle(Ellipsoid::WGS84),
                    2, 2
                ),
                WebMercatorProjection::computeMaximumProjectedRectangle(Ellipsoid::WGS84),
                minimumLevel,
                maximumLevel,
                width,
                height
            ),
            _credits(perTileCredits),
            _urlTemplate(urlTemplate),
            _subdomains(subdomains)
        {
            if (this->_urlTemplate.find("n=z") == std::string::npos) {
                this->_urlTemplate = Uri::addQuery(this->_urlTemplate, "n", "z");
            }

            std::string resolvedUrl = Uri::resolve(baseUrl, this->_urlTemplate);

            resolvedUrl = Uri::substituteTemplateParameters(resolvedUrl, [&culture](const std::string& templateKey) {
                if (templateKey == "culture") {
                    return culture;
                }

                // Keep other placeholders
                return "{" + templateKey + "}";
            });

        }

        virtual ~BingMapsTileProvider() {}

    protected:
        virtual std::unique_ptr<RasterOverlayTile> requestNewTile(const QuadtreeTileID& tileID) override {
            std::string url = Uri::substituteTemplateParameters(this->_urlTemplate, [this, &tileID](const std::string& key) {
                if (key == "quadkey") {
                    return BingMapsTileProvider::tileXYToQuadKey(tileID.level, tileID.x, tileID.computeInvertedY(this->getTilingScheme()));
                } else if (key == "subdomain") {
                    size_t subdomainIndex = (tileID.level + tileID.x + tileID.y) % this->_subdomains.size();
                    return this->_subdomains[subdomainIndex];
                }
                return key;
            });

            // Cesium levels start at 0, Bing levels start at 1
            unsigned int bingTileLevel = tileID.level + 1;
            CesiumGeospatial::GlobeRectangle tileRectangle = CesiumGeospatial::unprojectRectangleSimple(this->getProjection(), this->getTilingScheme().tileToRectangle(tileID));
            
            std::vector<Credit> tileCredits;
            for (std::pair<Credit, std::vector<BingMapsCreditCoverageArea>> creditAndCoverage : _credits) {
                
                bool withinCoverage = false;
                for (BingMapsCreditCoverageArea coverageArea : creditAndCoverage.second) {
                    if (coverageArea.zoomMin <= bingTileLevel && bingTileLevel <= coverageArea.zoomMax &&
                        coverageArea.rectangle.intersect(tileRectangle).has_value()
                    ) {
                        withinCoverage = true;
                        break;
                    }
                }

                if (withinCoverage) {
                    tileCredits.push_back(creditAndCoverage.first);
                }
            }
            
            return std::make_unique<RasterOverlayTile>(this->getOwner(), tileID, tileCredits, this->getAsyncSystem().requestAsset(url));
        }
    
    private:
        static std::string tileXYToQuadKey(uint32_t level, uint32_t x, uint32_t y) {
            std::string quadkey = "";
            for (int32_t i = static_cast<int32_t>(level); i >= 0; --i) {
                uint32_t bitmask = static_cast<uint32_t>(1 << i);
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

        std::vector<std::pair<Credit, std::vector<BingMapsCreditCoverageArea>>> _credits;
        std::string _urlTemplate;
        std::vector<std::string> _subdomains;
    };

    BingMapsRasterOverlay::BingMapsRasterOverlay(
        const std::string& url,
        const std::string& key,
        const std::string& mapStyle,
        const std::string& culture,
        const Ellipsoid& ellipsoid
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

    Future<std::unique_ptr<RasterOverlayTileProvider>> BingMapsRasterOverlay::createTileProvider(
        const AsyncSystem& asyncSystem,
        const std::shared_ptr<CreditSystem>& pCreditSystem,
        std::shared_ptr<IPrepareRendererResources> pPrepareRendererResources,
        std::shared_ptr<spdlog::logger> pLogger,
        RasterOverlay* pOwner
    ) {
        std::string metadataUrl = Uri::resolve(this->_url, "REST/v1/Imagery/Metadata/" + this->_mapStyle, true);
        metadataUrl = Uri::addQuery(metadataUrl, "incl", "ImageryProviders");
        metadataUrl = Uri::addQuery(metadataUrl, "key", this->_key);
        metadataUrl = Uri::addQuery(metadataUrl, "uriScheme", "https");

        pOwner = pOwner ? pOwner : this;

        return asyncSystem.requestAsset(metadataUrl).thenInWorkerThread([
            pOwner,
            asyncSystem,
            pCreditSystem,
            pPrepareRendererResources,
            pLogger,
            baseUrl = this->_url,
            culture = this->_culture
        ](std::unique_ptr<IAssetRequest> pRequest) -> std::unique_ptr<RasterOverlayTileProvider> {
            IAssetResponse* pResponse = pRequest->response();

            rapidjson::Document response;
            response.Parse(reinterpret_cast<const char*>(pResponse->data().data()), pResponse->data().size());

            if (response.HasParseError()) {
                SPDLOG_LOGGER_ERROR(pLogger, "Error when parsing Bing Maps imagery metadata, error code {} at byte offset {}", response.GetParseError(), response.GetErrorOffset());
                return nullptr;
            }

            rapidjson::Value* pResource = rapidjson::Pointer("/resourceSets/0/resources/0").Get(response);
            if (!pResource) {
                SPDLOG_LOGGER_ERROR(pLogger, "Resources were not found in the Bing Maps imagery metadata response.");
                return nullptr;
            }

            uint32_t width = JsonHelpers::getUint32OrDefault(*pResource, "imageWidth", 256U);
            uint32_t height = JsonHelpers::getUint32OrDefault(*pResource, "imageHeight", 256U);
            uint32_t maximumLevel = JsonHelpers::getUint32OrDefault(*pResource, "zoomMax", 30U);

            std::vector<std::string> subdomains = JsonHelpers::getStrings(*pResource, "imageUrlSubdomains");
            std::string urlTemplate = JsonHelpers::getStringOrDefault(*pResource, "imageUrl", std::string());
            if (urlTemplate.empty())  {
                SPDLOG_LOGGER_ERROR(pLogger, "Bing Maps tile imageUrl is missing or empty.");
                return nullptr;
            }
            
            const rapidjson::Value& attributions = (*pResource)["imageryProviders"];
            std::vector<std::pair<Credit, std::vector<BingMapsCreditCoverageArea>>> credits;
            for (const rapidjson::Value& attribution : attributions.GetArray()) {
                std::vector<BingMapsCreditCoverageArea> coverageAreas;
                for (const rapidjson::Value& area : attribution["coverageAreas"].GetArray()) {
                    const rapidjson::Value& bbox = area["bbox"];
                    BingMapsCreditCoverageArea coverageArea {
                        CesiumGeospatial::GlobeRectangle::fromDegrees(
                            bbox[1].GetDouble(),
                            bbox[0].GetDouble(),
                            bbox[3].GetDouble(),
                            bbox[2].GetDouble()
                        ),
                        area["zoomMin"].GetUint(),
                        area["zoomMax"].GetUint()
                    };
                    coverageAreas.push_back(coverageArea);
                }

                credits.push_back(
                    std::pair(
                        pCreditSystem->createCredit(attribution["attribution"].GetString()),
                        coverageAreas
                    )
                );
            }

            // TODO: check what exactly should be hardcoded here 
            Credit bingCredit = pCreditSystem->createCredit("a href=\"http://www.bing.com\"><img src=\"Assets/Images/bing_maps_credit.png\" title=\"Bing Imagery\"/></a>");


            return std::make_unique<BingMapsTileProvider>(
                *pOwner,
                asyncSystem,
                bingCredit,
                credits,
                pPrepareRendererResources,
                baseUrl,
                urlTemplate,
                subdomains,
                width,
                height,
                0,
                maximumLevel,
                culture
            );
        });
    }

}
