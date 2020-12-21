#include "Cesium3DTiles/RasterOverlayTile.h"
#include "Cesium3DTiles/RasterOverlayTileProvider.h"
#include "Cesium3DTiles/spdlog-cesium.h"
#include "Cesium3DTiles/TileMapServiceRasterOverlay.h"
#include "Cesium3DTiles/TilesetExternals.h"
#include "Cesium3DTiles/CreditSystem.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumAsync/IAssetResponse.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumGeospatial/WebMercatorProjection.h"
#include "tinyxml2.h"
#include "Uri.h"

using namespace CesiumAsync;

namespace Cesium3DTiles {

    class TileMapServiceTileProvider final : public RasterOverlayTileProvider {
    public:
        TileMapServiceTileProvider(
            RasterOverlay& owner,
            const AsyncSystem& asyncSystem,
            std::shared_ptr<IPrepareRendererResources> pPrepareRendererResources,
            const CesiumGeospatial::Projection& projection,
            const CesiumGeometry::QuadtreeTilingScheme& tilingScheme,
            const CesiumGeometry::Rectangle& coverageRectangle,
            const std::string& url,
            const std::vector<IAssetAccessor::THeader>& headers,
            const std::string& fileExtension,
            uint32_t width,
            uint32_t height,
            uint32_t minimumLevel,
            uint32_t maximumLevel
        ) :
            RasterOverlayTileProvider(
                owner,
                asyncSystem,
                pPrepareRendererResources,
                projection,
                tilingScheme,
                coverageRectangle,
                minimumLevel,
                maximumLevel,
                width,
                height
            ),
            _url(url),
            _headers(headers),
            _fileExtension(fileExtension)
        {
        }

        virtual ~TileMapServiceTileProvider() {}

    protected:
        virtual std::unique_ptr<RasterOverlayTile> requestNewTile(const CesiumGeometry::QuadtreeTileID& tileID) override {
            std::string url = Uri::resolve(
                this->_url,
                std::to_string(tileID.level) + "/" +
                    std::to_string(tileID.x) + "/" +
                    std::to_string(tileID.y) +
                    this->_fileExtension,
                true
            );

            return std::make_unique<RasterOverlayTile>(this->getOwner(), tileID, std::vector<Credit>(), this->getAsyncSystem().requestAsset(url, this->_headers));
        }
    
    private:
        std::string _url;
        std::vector<IAssetAccessor::THeader> _headers;
        std::string _fileExtension;
    };

    TileMapServiceRasterOverlay::TileMapServiceRasterOverlay(
        const std::string& url,
        const std::shared_ptr<CreditSystem>& creditSystem,
        const std::vector<IAssetAccessor::THeader>& headers,
        const TileMapServiceRasterOverlayOptions& options
    ) :
        RasterOverlay(creditSystem),
        _credit(options.credit ? 
            std::optional<Credit>(creditSystem->createCredit(options.credit.value())) : 
            std::nullopt
        ),
        _url(url),
        _headers(headers),
        _options(options)
    {
    }

    TileMapServiceRasterOverlay::~TileMapServiceRasterOverlay() {
    }

    static std::optional<std::string> getAttributeString(const tinyxml2::XMLElement* pElement, const char* attributeName) {
        if (!pElement) {
            return std::nullopt;
        }

        const char* pAttrValue = pElement->Attribute(attributeName);
        if (!pAttrValue) {
            return std::nullopt;
        }

        return std::string(pAttrValue);
    }

    static std::optional<uint32_t> getAttributeUint32(const tinyxml2::XMLElement* pElement, const char* attributeName) {
        std::optional<std::string> s = getAttributeString(pElement, attributeName);
        if (s) {
            return std::stoul(s.value());
        }
        return std::nullopt;
    }

    static std::optional<double> getAttributeDouble(const tinyxml2::XMLElement* pElement, const char* attributeName) {
        std::optional<std::string> s = getAttributeString(pElement, attributeName);
        if (s) {
            return std::stod(s.value());
        }
        return std::nullopt;
    }

    Future<std::unique_ptr<RasterOverlayTileProvider>> TileMapServiceRasterOverlay::createTileProvider(
        const AsyncSystem& asyncSystem,
        std::shared_ptr<IPrepareRendererResources> pPrepareRendererResources,
        std::shared_ptr<spdlog::logger> pLogger,
        RasterOverlay* pOwner
    ) {
        std::string xmlUrl = Uri::resolve(this->_url, "tilemapresource.xml");

        pOwner = pOwner ? pOwner : this;

        return asyncSystem.requestAsset(xmlUrl, this->_headers).thenInWorkerThread([
            pOwner,
            asyncSystem,
            pPrepareRendererResources,
            pLogger,
            options = this->_options,
            url = this->_url,
            headers = this->_headers
        ](std::unique_ptr<IAssetRequest> pRequest) -> std::unique_ptr<RasterOverlayTileProvider> {
            IAssetResponse* pResponse = pRequest->response();

            gsl::span<const uint8_t> data = pResponse->data();

            tinyxml2::XMLDocument doc;
            tinyxml2::XMLError error = doc.Parse(reinterpret_cast<const char*>(data.data()), data.size_bytes());
            if (error != tinyxml2::XMLError::XML_SUCCESS) {
                SPDLOG_LOGGER_ERROR(pLogger, "Could not parse tile map service XML.");
                return nullptr;
            }

            tinyxml2::XMLElement* pRoot = doc.RootElement();
            if (!pRoot) {
                SPDLOG_LOGGER_ERROR(pLogger, "Tile map service XML document does not have a root element.");
                return nullptr;
            }

            // CesiumGeospatial::Ellipsoid ellipsoid = this->_options.ellipsoid.value_or(CesiumGeospatial::Ellipsoid::WGS84);

            tinyxml2::XMLElement* pTileFormat = pRoot->FirstChildElement("TileFormat");
            std::string fileExtension = options.fileExtension.value_or(getAttributeString(pTileFormat, "extension").value_or("png"));
            uint32_t tileWidth = options.tileWidth.value_or(getAttributeUint32(pTileFormat, "width").value_or(256));
            uint32_t tileHeight = options.tileHeight.value_or(getAttributeUint32(pTileFormat, "height").value_or(256));

            uint32_t minimumLevel = std::numeric_limits<uint32_t>::max();
            uint32_t maximumLevel = 0;

            tinyxml2::XMLElement* pTilesets = pRoot->FirstChildElement("TileSets");
            if (pTilesets) {
                tinyxml2::XMLElement* pTileset = pTilesets->FirstChildElement("TileSet");
                while (pTileset) {
                    uint32_t level = getAttributeUint32(pTileset, "order").value_or(0);
                    minimumLevel = glm::min(minimumLevel, level);
                    maximumLevel = glm::max(maximumLevel, level);

                    pTileset = pTileset->NextSiblingElement("TileSet");
                }
            }

            CesiumGeospatial::GlobeRectangle tilingSchemeRectangle = CesiumGeospatial::GeographicProjection::MAXIMUM_GLOBE_RECTANGLE;
            CesiumGeospatial::Projection projection;
            uint32_t rootTilesX = 1;
            bool isRectangleInDegrees = false;

            if (options.projection) {
                projection = options.projection.value();
            } else {
                std::string projectionName = getAttributeString(pTilesets, "profile").value_or("mercator");

                if (projectionName == "mercator" || projectionName == "global-mercator") {
                    projection = CesiumGeospatial::WebMercatorProjection();
                    tilingSchemeRectangle = CesiumGeospatial::WebMercatorProjection::MAXIMUM_GLOBE_RECTANGLE;

                    // Determine based on the profile attribute if this tileset was generated by gdal2tiles.py, which
                    // uses 'mercator' and 'geodetic' profiles, or by a tool compliant with the TMS standard, which is
                    // 'global-mercator' and 'global-geodetic' profiles. In the gdal2Tiles case, X and Y are always in
                    // geodetic degrees.
                    isRectangleInDegrees = projectionName.find("global-") != 0;
                } else if (projectionName == "geodetic" || projectionName == "global-geodetic") {
                    projection = CesiumGeospatial::GeographicProjection();
                    tilingSchemeRectangle = CesiumGeospatial::GeographicProjection::MAXIMUM_GLOBE_RECTANGLE;
                    rootTilesX = 2;

                    // The geodetic profile is always in degrees.
                    isRectangleInDegrees = true;
                }
            }

            minimumLevel = glm::min(minimumLevel, maximumLevel);

            minimumLevel = options.minimumLevel.value_or(minimumLevel);
            maximumLevel = options.maximumLevel.value_or(maximumLevel);

            CesiumGeometry::Rectangle coverageRectangle = projectRectangleSimple(projection, tilingSchemeRectangle);

            if (options.coverageRectangle) {
                coverageRectangle = options.coverageRectangle.value();
            } else {
                tinyxml2::XMLElement* pBoundingBox = pRoot->FirstChildElement("BoundingBox");

                std::optional<double> west = getAttributeDouble(pBoundingBox, "minx");
                std::optional<double> south = getAttributeDouble(pBoundingBox, "miny");
                std::optional<double> east = getAttributeDouble(pBoundingBox, "maxx");
                std::optional<double> north = getAttributeDouble(pBoundingBox, "maxy");

                if (west && south && east && north) {
                    if (isRectangleInDegrees) {
                        coverageRectangle = projectRectangleSimple(projection, CesiumGeospatial::GlobeRectangle::fromDegrees(west.value(), south.value(), east.value(), north.value()));
                    } else {
                        coverageRectangle = CesiumGeometry::Rectangle(west.value(), south.value(), east.value(), north.value());
                    }
                }
            }

            CesiumGeometry::QuadtreeTilingScheme tilingScheme(
                projectRectangleSimple(projection, tilingSchemeRectangle),
                rootTilesX,
                1
            );

            return std::make_unique<TileMapServiceTileProvider>(
                *pOwner,
                asyncSystem,
                pPrepareRendererResources,
                projection,
                tilingScheme,
                coverageRectangle,
                url,
                headers,
                fileExtension.size() > 0 ? "." + fileExtension : fileExtension,
                tileWidth,
                tileHeight,
                minimumLevel,
                maximumLevel
            );
        });
    }

}
