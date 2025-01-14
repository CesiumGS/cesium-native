#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGeospatial/WebMercatorProjection.h>
#include <CesiumRasterOverlays/IPrepareRasterOverlayRendererResources.h>
#include <CesiumRasterOverlays/QuadtreeRasterOverlayTileProvider.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayLoadFailureDetails.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumRasterOverlays/TileMapServiceRasterOverlay.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/ErrorList.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/Uri.h>

#include <glm/common.hpp>
#include <nonstd/expected.hpp>
#include <spdlog/fwd.h>
#include <tinyxml2.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

using namespace CesiumAsync;
using namespace CesiumUtility;

namespace CesiumRasterOverlays {

namespace {
struct TileMapServiceTileset {
  std::string url;
  uint32_t level;
};

std::optional<std::string> getAttributeString(
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

std::optional<uint32_t> getAttributeUint32(
    const tinyxml2::XMLElement* pElement,
    const char* attributeName) {
  std::optional<std::string> s = getAttributeString(pElement, attributeName);
  if (s) {
    return std::stoul(s.value());
  }
  return std::nullopt;
}

std::optional<double> getAttributeDouble(
    const tinyxml2::XMLElement* pElement,
    const char* attributeName) {
  std::optional<std::string> s = getAttributeString(pElement, attributeName);
  if (s) {
    return std::stod(s.value());
  }
  return std::nullopt;
}
} // namespace

class TileMapServiceTileProvider final
    : public QuadtreeRasterOverlayTileProvider {
public:
  TileMapServiceTileProvider(
      const IntrusivePointer<const RasterOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
      std::optional<Credit> credit,
      const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
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
      uint32_t maximumLevel,
      const std::vector<TileMapServiceTileset>& tileSets)
      : QuadtreeRasterOverlayTileProvider(
            pOwner,
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
        _fileExtension(fileExtension),
        _tileSets(tileSets) {}

  virtual ~TileMapServiceTileProvider() = default;

protected:
  virtual CesiumAsync::Future<LoadedRasterOverlayImage> loadQuadtreeTileImage(
      const CesiumGeometry::QuadtreeTileID& tileID) const override {

    LoadTileImageFromUrlOptions options;
    options.rectangle = this->getTilingScheme().tileToRectangle(tileID);
    options.moreDetailAvailable = tileID.level < this->getMaximumLevel();

    uint32_t level = tileID.level - this->getMinimumLevel();

    if (level < _tileSets.size()) {
      const TileMapServiceTileset& tileset = _tileSets[level];
      std::string url = CesiumUtility::Uri::resolve(
          this->_url,
          tileset.url + "/" + std::to_string(tileID.x) + "/" +
              std::to_string(tileID.y) + this->_fileExtension,
          true);
      return this->loadTileImageFromUrl(
          url,
          this->_headers,
          std::move(options));
    } else {
      ErrorList errors;
      errors.emplaceError("Failed to load image from TMS.");
      return this->getAsyncSystem()
          .createResolvedFuture<LoadedRasterOverlayImage>(
              {nullptr,
               options.rectangle,
               {},
               std::move(errors),
               options.moreDetailAvailable});
    }
  }

private:
  std::string _url;
  std::vector<IAssetAccessor::THeader> _headers;
  std::string _fileExtension;
  std::vector<TileMapServiceTileset> _tileSets;
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

TileMapServiceRasterOverlay::~TileMapServiceRasterOverlay() = default;

namespace {

using GetXmlDocumentResult = nonstd::expected<
    std::unique_ptr<tinyxml2::XMLDocument>,
    RasterOverlayLoadFailureDetails>;

Future<GetXmlDocumentResult> getXmlDocument(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor,
    const std::string& url,
    const std::vector<IAssetAccessor::THeader>& headers) {
  return pAssetAccessor->get(asyncSystem, url, headers)
      .thenInWorkerThread(
          [asyncSystem, pAssetAccessor, url, headers](
              std::shared_ptr<IAssetRequest>&& pRequest)
              -> Future<GetXmlDocumentResult> {
            const IAssetResponse* pResponse = pRequest->response();
            if (!pResponse) {
              return asyncSystem.createResolvedFuture<GetXmlDocumentResult>(
                  nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                      RasterOverlayLoadType::TileProvider,
                      std::move(pRequest),
                      "No response received from Tile Map Service."}));
            }

            const std::span<const std::byte> data = pResponse->data();

            std::unique_ptr<tinyxml2::XMLDocument> pDoc =
                std::make_unique<tinyxml2::XMLDocument>(
                    new tinyxml2::XMLDocument());
            const tinyxml2::XMLError error = pDoc->Parse(
                reinterpret_cast<const char*>(data.data()),
                data.size_bytes());

            bool hasError = false;
            std::string errorMessage;

            if (error != tinyxml2::XMLError::XML_SUCCESS) {
              hasError = true;
              errorMessage = "Unable to parse Tile map service XML document.";
            } else {
              tinyxml2::XMLElement* pRoot = pDoc->RootElement();
              if (!pRoot) {
                hasError = true;
                errorMessage =
                    "Tile map service XML document does not have a root "
                    "element.";
              } else {
                tinyxml2::XMLElement* pTilesets =
                    pRoot->FirstChildElement("TileSets");

                if (!pTilesets) {
                  hasError = true;
                  errorMessage = "Tile map service XML document does not have "
                                 "any tilesets.";
                }
                tinyxml2::XMLElement* srs = pRoot->FirstChildElement("SRS");
                if (srs) {
                  std::string srsText = srs->GetText();
                  if (srsText.find("4326") == std::string::npos &&
                      srsText.find("3857") == std::string::npos &&
                      srsText.find("900913") == std::string::npos) {
                    hasError = true;
                    errorMessage = srsText + " is not supported.";
                  }
                } else {
                  hasError = true;
                  errorMessage =
                      "Tile map service XML document does not have an SRS.";
                }
              }
            }
            if (hasError) {
              if (url.find("tilemapresource.xml") == std::string::npos) {
                std::string updatedUrl = url;

                std::string urlPath = Uri::getPath(url);
                if (!(urlPath.size() < 4)) {
                  if (urlPath.substr(urlPath.size() - 4, 4) != ".xml") {
                    if (urlPath[urlPath.size() - 1] != '/') {
                      urlPath += "/";
                      updatedUrl = Uri::setPath(url, urlPath);
                    }
                  }
                }

                return getXmlDocument(
                    asyncSystem,
                    pAssetAccessor,
                    CesiumUtility::Uri::resolve(
                        updatedUrl,
                        "tilemapresource.xml"),
                    headers);
              } else {
                return asyncSystem.createResolvedFuture<GetXmlDocumentResult>(
                    nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                        RasterOverlayLoadType::TileProvider,
                        std::move(pRequest),
                        errorMessage}));
              }
            }

            return asyncSystem.createResolvedFuture<GetXmlDocumentResult>(
                std::move(pDoc));
          });
}

} // namespace

Future<RasterOverlay::CreateTileProviderResult>
TileMapServiceRasterOverlay::createTileProvider(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& pCreditSystem,
    const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
        pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner) const {
  std::string xmlUrl = this->_url;

  pOwner = pOwner ? pOwner : this;

  std::optional<Credit> credit = std::nullopt;
  if (pCreditSystem && this->_options.credit) {
    credit = pCreditSystem->createCredit(
        *this->_options.credit,
        pOwner->getOptions().showCreditsOnScreen);
  }

  return getXmlDocument(asyncSystem, pAssetAccessor, xmlUrl, this->_headers)
      .thenInMainThread(
          [pOwner,
           asyncSystem,
           pAssetAccessor,
           credit,
           pPrepareRendererResources,
           pLogger,
           options = this->_options,
           url = this->_url,
           headers = this->_headers](
              GetXmlDocumentResult&& xml) -> CreateTileProviderResult {
            if (!xml) {
              return nonstd::make_unexpected(std::move(xml).error());
            }

            std::unique_ptr<tinyxml2::XMLDocument> pDoc = std::move(*xml);
            tinyxml2::XMLElement* pRoot = pDoc->RootElement();

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

            std::vector<TileMapServiceTileset> tileSets;

            tinyxml2::XMLElement* pTilesets =
                pRoot->FirstChildElement("TileSets");
            if (pTilesets) {
              tinyxml2::XMLElement* pTileset =
                  pTilesets->FirstChildElement("TileSet");
              while (pTileset) {
                TileMapServiceTileset& tileSet = tileSets.emplace_back();
                tileSet.level =
                    getAttributeUint32(pTileset, "order").value_or(0);
                minimumLevel = glm::min(minimumLevel, tileSet.level);
                maximumLevel = glm::max(maximumLevel, tileSet.level);
                tileSet.url = getAttributeString(pTileset, "href")
                                  .value_or(std::to_string(tileSet.level));
                pTileset = pTileset->NextSiblingElement("TileSet");
              }
            }

            if (maximumLevel < minimumLevel && maximumLevel == 0) {
              // Min and max levels unknown, so use defaults.
              minimumLevel = 0;
              maximumLevel = 25;
            }

            const CesiumGeospatial::Ellipsoid& ellipsoid =
                pOwner->getOptions().ellipsoid;

            CesiumGeospatial::GlobeRectangle tilingSchemeRectangle =
                CesiumGeospatial::GeographicProjection::MAXIMUM_GLOBE_RECTANGLE;
            CesiumGeospatial::Projection projection =
                CesiumGeospatial::WebMercatorProjection(ellipsoid);
            uint32_t rootTilesX = 1;
            bool isRectangleInDegrees = false;

            if (options.projection) {
              projection = options.projection.value();
            } else {
              std::string projectionName =
                  getAttributeString(pTilesets, "profile").value_or("mercator");

              if (projectionName == "mercator" ||
                  projectionName == "global-mercator") {
                projection = CesiumGeospatial::WebMercatorProjection(ellipsoid);
                tilingSchemeRectangle = CesiumGeospatial::
                    WebMercatorProjection::MAXIMUM_GLOBE_RECTANGLE;

                // Determine based on the profile attribute if this tileset was
                // generated by gdal2tiles.py, which uses 'mercator' and
                // 'geodetic' profiles, or by a tool compliant with the TMS
                // standard, which is 'global-mercator' and 'global-geodetic'
                // profiles. In the gdal2Tiles case, X and Y are always in
                // geodetic degrees.
                isRectangleInDegrees = !projectionName.starts_with("global-");
              } else if (
                  projectionName == "geodetic" ||
                  projectionName == "global-geodetic") {
                projection = CesiumGeospatial::GeographicProjection(ellipsoid);
                tilingSchemeRectangle = CesiumGeospatial::GeographicProjection::
                    MAXIMUM_GLOBE_RECTANGLE;
                rootTilesX = 2;

                // The geodetic profile is always in degrees.
                isRectangleInDegrees = true;
              } else {
                tinyxml2::XMLElement* srs = pRoot->FirstChildElement("SRS");
                if (srs) {
                  std::string srsText = srs->GetText();
                  if (srsText.find("4326") != std::string::npos) {
                    projection =
                        CesiumGeospatial::GeographicProjection(ellipsoid);
                    tilingSchemeRectangle = CesiumGeospatial::
                        GeographicProjection::MAXIMUM_GLOBE_RECTANGLE;
                    rootTilesX = 2;
                    isRectangleInDegrees = true;
                  } else if (
                      srsText.find("3857") != std::string::npos ||
                      srsText.find("900913") != std::string::npos) {
                    projection =
                        CesiumGeospatial::WebMercatorProjection(ellipsoid);
                    tilingSchemeRectangle = CesiumGeospatial::
                        WebMercatorProjection::MAXIMUM_GLOBE_RECTANGLE;
                    isRectangleInDegrees = true;
                  }
                }
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

            std::string updatedUrl = url;

            std::string urlPath = Uri::getPath(url);
            if (!(urlPath.size() < 4)) {
              if (urlPath.substr(urlPath.size() - 4, 4) != ".xml") {
                if (urlPath[urlPath.size() - 1] != '/') {
                  urlPath += "/";
                  updatedUrl = Uri::setPath(url, urlPath);
                }
              }
            }

            return new TileMapServiceTileProvider(
                pOwner,
                asyncSystem,
                pAssetAccessor,
                credit,
                pPrepareRendererResources,
                pLogger,
                projection,
                tilingScheme,
                coverageRectangle,
                updatedUrl,
                headers,
                !fileExtension.empty() ? "." + fileExtension : fileExtension,
                tileWidth,
                tileHeight,
                minimumLevel,
                maximumLevel,
                tileSets);
          });
}

} // namespace CesiumRasterOverlays
