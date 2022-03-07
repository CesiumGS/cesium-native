#include "Cesium3DTilesSelection/TileMapServiceRasterOverlay.h"

#include "Cesium3DTilesSelection/CreditSystem.h"
#include "Cesium3DTilesSelection/QuadtreeRasterOverlayTileProvider.h"
#include "Cesium3DTilesSelection/RasterOverlayLoadFailureDetails.h"
#include "Cesium3DTilesSelection/RasterOverlayTile.h"
#include "Cesium3DTilesSelection/TilesetExternals.h"
#include "Cesium3DTilesSelection/spdlog-cesium.h"
#include "Cesium3DTilesSelection/tinyxml-cesium.h"

#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/WebMercatorProjection.h>
#include <CesiumUtility/Uri.h>

#include <cstddef>

using namespace CesiumAsync;

namespace Cesium3DTilesSelection {

class TileMapServiceTileProvider final
    : public QuadtreeRasterOverlayTileProvider {
public:
  TileMapServiceTileProvider(
      RasterOverlay& owner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
      std::optional<Credit> credit,
      const std::shared_ptr<IPrepareRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const CesiumGeospatial::Projection& projection,
      const CesiumGeometry::QuadtreeTilingScheme& tilingScheme,
      const CesiumGeometry::Rectangle& coverageRectangle,
      const std::string& url,
      const std::vector<IAssetAccessor::THeader>& headers,
      const std::string& fileExtension,
      uint32_t width,
      uint32_t height,
      uint32_t minimumLevel,
      uint32_t maximumLevel)
      : QuadtreeRasterOverlayTileProvider(
            owner,
            asyncSystem,
            pAssetAccessor,
            credit,
            pPrepareRendererResources,
            pLogger,
            projection,
            tilingScheme,
            coverageRectangle,
            minimumLevel,
            maximumLevel,
            width,
            height),
        _url(url),
        _headers(headers),
        _fileExtension(fileExtension) {}

  virtual ~TileMapServiceTileProvider() {}

protected:
  virtual CesiumAsync::Future<LoadedRasterOverlayImage> loadQuadtreeTileImage(
      const CesiumGeometry::QuadtreeTileID& tileID) const override {
    std::string url = CesiumUtility::Uri::resolve(
        this->_url,
        std::to_string(tileID.level) + "/" + std::to_string(tileID.x) + "/" +
            std::to_string(tileID.y) + this->_fileExtension,
        true);

    LoadTileImageFromUrlOptions options;
    options.rectangle = this->getTilingScheme().tileToRectangle(tileID);
    options.moreDetailAvailable = tileID.level < this->getMaximumLevel();
    return this->loadTileImageFromUrl(url, this->_headers, std::move(options));
  }

private:
  std::string _url;
  std::vector<IAssetAccessor::THeader> _headers;
  std::string _fileExtension;
};

TileMapServiceRasterOverlay::TileMapServiceRasterOverlay(
    const std::string& name,
    const std::string& url,
    const std::vector<IAssetAccessor::THeader>& headers,
    const TileMapServiceRasterOverlayOptions& tmsOptions,
    const RasterOverlayOptions& overlayOptions)
    : RasterOverlay(name, overlayOptions),
      _url(url),
      _headers(headers),
      _options(tmsOptions) {}

TileMapServiceRasterOverlay::~TileMapServiceRasterOverlay() {}

static std::optional<std::string> getAttributeString(
    const tinyxml2::XMLElement* pElement,
    const char* attributeName) {
  if (!pElement) {
    return std::nullopt;
  }

  const char* pAttrValue = pElement->Attribute(attributeName);
  if (!pAttrValue) {
    return std::nullopt;
  }

  return std::string(pAttrValue);
}

static std::optional<uint32_t> getAttributeUint32(
    const tinyxml2::XMLElement* pElement,
    const char* attributeName) {
  std::optional<std::string> s = getAttributeString(pElement, attributeName);
  if (s) {
    return std::stoul(s.value());
  }
  return std::nullopt;
}

static std::optional<double> getAttributeDouble(
    const tinyxml2::XMLElement* pElement,
    const char* attributeName) {
  std::optional<std::string> s = getAttributeString(pElement, attributeName);
  if (s) {
    return std::stod(s.value());
  }
  return std::nullopt;
}

Future<std::unique_ptr<RasterOverlayTileProvider>>
TileMapServiceRasterOverlay::createTileProvider(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& pCreditSystem,
    const std::shared_ptr<IPrepareRendererResources>& pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    RasterOverlay* pOwner) {
  std::string xmlUrl =
      CesiumUtility::Uri::resolve(this->_url, "tilemapresource.xml");

  pOwner = pOwner ? pOwner : this;

  const std::optional<Credit> credit =
      this->_options.credit ? std::make_optional(pCreditSystem->createCredit(
                                  this->_options.credit.value(),
                                  pOwner->getOptions().showCreditsOnScreen))
                            : std::nullopt;

  auto reportError = [this, asyncSystem, pLogger](
                         const std::shared_ptr<IAssetRequest>& pRequest,
                         const std::string& message) {
    this->reportError(
        asyncSystem,
        pLogger,
        RasterOverlayLoadFailureDetails{
            this,
            RasterOverlayLoadType::TileProvider,
            pRequest,
            message});
  };

  return pAssetAccessor->get(asyncSystem, xmlUrl, this->_headers)
      .thenInWorkerThread(
          [pOwner,
           asyncSystem,
           pAssetAccessor,
           credit,
           pPrepareRendererResources,
           pLogger,
           options = this->_options,
           url = this->_url,
           headers = this->_headers,
           reportError](const std::shared_ptr<IAssetRequest>& pRequest)
              -> std::unique_ptr<RasterOverlayTileProvider> {
            const IAssetResponse* pResponse = pRequest->response();
            if (!pResponse) {
              reportError(
                  pRequest,
                  "No response received from Tile Map Service.");
              return nullptr;
            }

            const gsl::span<const std::byte> data = pResponse->data();

            tinyxml2::XMLDocument doc;
            const tinyxml2::XMLError error = doc.Parse(
                reinterpret_cast<const char*>(data.data()),
                data.size_bytes());
            if (error != tinyxml2::XMLError::XML_SUCCESS) {
              reportError(pRequest, "Could not parse tile map service XML.");
              return nullptr;
            }

            tinyxml2::XMLElement* pRoot = doc.RootElement();
            if (!pRoot) {
              reportError(
                  pRequest,
                  "Tile map service XML document does not have a root "
                  "element.");
              return nullptr;
            }

            // CesiumGeospatial::Ellipsoid ellipsoid =
            // this->_options.ellipsoid.value_or(CesiumGeospatial::Ellipsoid::WGS84);

            tinyxml2::XMLElement* pTileFormat =
                pRoot->FirstChildElement("TileFormat");
            std::string fileExtension = options.fileExtension.value_or(
                getAttributeString(pTileFormat, "extension").value_or("png"));
            uint32_t tileWidth = options.tileWidth.value_or(
                getAttributeUint32(pTileFormat, "width").value_or(256));
            uint32_t tileHeight = options.tileHeight.value_or(
                getAttributeUint32(pTileFormat, "height").value_or(256));

            uint32_t minimumLevel = std::numeric_limits<uint32_t>::max();
            uint32_t maximumLevel = 0;

            tinyxml2::XMLElement* pTilesets =
                pRoot->FirstChildElement("TileSets");
            if (pTilesets) {
              tinyxml2::XMLElement* pTileset =
                  pTilesets->FirstChildElement("TileSet");
              while (pTileset) {
                const uint32_t level =
                    getAttributeUint32(pTileset, "order").value_or(0);
                minimumLevel = glm::min(minimumLevel, level);
                maximumLevel = glm::max(maximumLevel, level);

                pTileset = pTileset->NextSiblingElement("TileSet");
              }
            }

            if (maximumLevel < minimumLevel && maximumLevel == 0) {
              // Min and max levels unknown, so use defaults.
              minimumLevel = 0;
              maximumLevel = 25;
            }

            CesiumGeospatial::GlobeRectangle tilingSchemeRectangle =
                CesiumGeospatial::GeographicProjection::MAXIMUM_GLOBE_RECTANGLE;
            CesiumGeospatial::Projection projection;
            uint32_t rootTilesX = 1;
            bool isRectangleInDegrees = false;

            if (options.projection) {
              projection = options.projection.value();
            } else {
              std::string projectionName =
                  getAttributeString(pTilesets, "profile").value_or("mercator");

              if (projectionName == "mercator" ||
                  projectionName == "global-mercator") {
                projection = CesiumGeospatial::WebMercatorProjection();
                tilingSchemeRectangle = CesiumGeospatial::
                    WebMercatorProjection::MAXIMUM_GLOBE_RECTANGLE;

                // Determine based on the profile attribute if this tileset was
                // generated by gdal2tiles.py, which uses 'mercator' and
                // 'geodetic' profiles, or by a tool compliant with the TMS
                // standard, which is 'global-mercator' and 'global-geodetic'
                // profiles. In the gdal2Tiles case, X and Y are always in
                // geodetic degrees.
                isRectangleInDegrees = projectionName.find("global-") != 0;
              } else if (
                  projectionName == "geodetic" ||
                  projectionName == "global-geodetic") {
                projection = CesiumGeospatial::GeographicProjection();
                tilingSchemeRectangle = CesiumGeospatial::GeographicProjection::
                    MAXIMUM_GLOBE_RECTANGLE;
                rootTilesX = 2;

                // The geodetic profile is always in degrees.
                isRectangleInDegrees = true;
              }
            }

            minimumLevel = glm::min(minimumLevel, maximumLevel);

            minimumLevel = options.minimumLevel.value_or(minimumLevel);
            maximumLevel = options.maximumLevel.value_or(maximumLevel);

            CesiumGeometry::Rectangle coverageRectangle =
                projectRectangleSimple(projection, tilingSchemeRectangle);

            if (options.coverageRectangle) {
              coverageRectangle = options.coverageRectangle.value();
            } else {
              tinyxml2::XMLElement* pBoundingBox =
                  pRoot->FirstChildElement("BoundingBox");

              std::optional<double> west =
                  getAttributeDouble(pBoundingBox, "minx");
              std::optional<double> south =
                  getAttributeDouble(pBoundingBox, "miny");
              std::optional<double> east =
                  getAttributeDouble(pBoundingBox, "maxx");
              std::optional<double> north =
                  getAttributeDouble(pBoundingBox, "maxy");

              if (west && south && east && north) {
                if (isRectangleInDegrees) {
                  coverageRectangle = projectRectangleSimple(
                      projection,
                      CesiumGeospatial::GlobeRectangle::fromDegrees(
                          west.value(),
                          south.value(),
                          east.value(),
                          north.value()));
                } else {
                  coverageRectangle = CesiumGeometry::Rectangle(
                      west.value(),
                      south.value(),
                      east.value(),
                      north.value());
                }
              }
            }

            CesiumGeometry::QuadtreeTilingScheme tilingScheme(
                projectRectangleSimple(projection, tilingSchemeRectangle),
                rootTilesX,
                1);

            return std::make_unique<TileMapServiceTileProvider>(
                *pOwner,
                asyncSystem,
                pAssetAccessor,
                credit,
                pPrepareRendererResources,
                pLogger,
                projection,
                tilingScheme,
                coverageRectangle,
                url,
                headers,
                !fileExtension.empty() ? "." + fileExtension : fileExtension,
                tileWidth,
                tileHeight,
                minimumLevel,
                maximumLevel);
          });
}

} // namespace Cesium3DTilesSelection
