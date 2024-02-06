#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumRasterOverlays/QuadtreeRasterOverlayTileProvider.h>
#include <CesiumRasterOverlays/RasterOverlayLoadFailureDetails.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/WebMapTileServiceRasterOverlay.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/Uri.h>

#include <cstddef>
#include <variant>

using namespace CesiumAsync;
using namespace CesiumUtility;

namespace CesiumRasterOverlays {

class WebMapTileServiceTileProvider final
    : public QuadtreeRasterOverlayTileProvider {
public:
  WebMapTileServiceTileProvider(
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
      const bool useKVP,
      const std::string& format,
      uint32_t width,
      uint32_t height,
      uint32_t minimumLevel,
      uint32_t maximumLevel,
      std::string layer,
      std::string style,
      std::string _tileMatrixSetID,
      const std::optional<std::vector<std::string>> tileMatrixLabels,
      const std::optional<std::map<std::string, std::string>> dimensions,
      const std::vector<std::string>& subdomains)
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
        _useKVP(useKVP),
        _format(format),
        _layer(layer),
        _style(style),
        _tileMatrixSetID(_tileMatrixSetID),
        _labels(tileMatrixLabels),
        _staticDimensions(dimensions),
        _subdomains(subdomains) {}

  virtual ~WebMapTileServiceTileProvider() {}

protected:
  virtual bool getQuadtreeTileImageRequest(
      const CesiumGeometry::QuadtreeTileID& tileID,
      RequestData& requestData,
      std::string&) const override {

    const CesiumGeospatial::GlobeRectangle tileRectangle =
        CesiumGeospatial::unprojectRectangleSimple(
            this->getProjection(),
            this->getTilingScheme().tileToRectangle(tileID));

    std::string queryString = "?";

    if (this->_url.find(queryString) != std::string::npos)
      queryString = "&";

    const std::string urlTemplate =
        this->_url + queryString +
        "request=GetMap&TRANSPARENT=TRUE&version={version}&service="
        "WMS&"
        "format={format}&styles="
        "&width={width}&height={height}&bbox={minx},{miny},{maxx},{maxy}"
        "&layers={layers}&crs=EPSG:4326";

    const auto radiansToDegrees = [](double rad) {
      return std::to_string(CesiumUtility::Math::radiansToDegrees(rad));
    };

    const std::map<std::string, std::string> urlTemplateMap = {
        {"baseUrl", this->_url},
        {"maxx", radiansToDegrees(tileRectangle.getNorth())},
        {"maxy", radiansToDegrees(tileRectangle.getEast())},
        {"minx", radiansToDegrees(tileRectangle.getSouth())},
        {"miny", radiansToDegrees(tileRectangle.getWest())},
        {"format", this->_format},
        {"width", std::to_string(this->getWidth())},
        {"height", std::to_string(this->getHeight())}};

    requestData.url = CesiumUtility::Uri::substituteTemplateParameters(
        urlTemplate,
        [&map = urlTemplateMap](const std::string& placeholder) {
          auto it = map.find(placeholder);
          return it == map.end() ? "{" + placeholder + "}"
                                 : Uri::escape(it->second);
        });

    return true;
  }

  virtual CesiumAsync::Future<RasterLoadResult> loadQuadtreeTileImage(
      const CesiumGeometry::QuadtreeTileID& tileID,
      const std::string& requestUrl,
      uint16_t statusCode,
      const gsl::span<const std::byte>& data) const override {

    LoadTileImageFromUrlOptions options;
    options.rectangle = this->getTilingScheme().tileToRectangle(tileID);
    options.moreDetailAvailable = tileID.level < this->getMaximumLevel();

    return this->loadTileImageFromUrl(
        requestUrl,
        statusCode,
        data,
        std::move(options));
  }

private:
  std::string _url;
  std::vector<IAssetAccessor::THeader> _headers;
  bool _useKVP;
  std::string _format;
  std::string _layer;
  std::string _style;
  std::string _tileMatrixSetID;
  std::optional<std::vector<std::string>> _labels;
  std::optional<std::map<std::string, std::string>> _staticDimensions;
  std::vector<std::string> _subdomains;
};

WebMapTileServiceRasterOverlay::WebMapTileServiceRasterOverlay(
    const std::string& name,
    const std::string& url,
    const std::vector<IAssetAccessor::THeader>& headers,
    const WebMapTileServiceRasterOverlayOptions& wmtsOptions,
    const RasterOverlayOptions& overlayOptions)
    : RasterOverlay(name, overlayOptions),
      _url(url),
      _headers(headers),
      _options(wmtsOptions) {}

WebMapTileServiceRasterOverlay::~WebMapTileServiceRasterOverlay() {}

Future<RasterOverlay::CreateTileProviderResult>
WebMapTileServiceRasterOverlay::createTileProvider(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CesiumUtility::CreditSystem>& pCreditSystem,
    const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
        pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner) const {

  pOwner = pOwner ? pOwner : this;

  const std::optional<Credit> credit =
      this->_options.credit ? std::make_optional(pCreditSystem->createCredit(
                                  this->_options.credit.value(),
                                  pOwner->getOptions().showCreditsOnScreen))
                            : std::nullopt;

  bool hasError = false;
  std::string errorMessage;

  std::string format = _options.format.value_or("image/jpeg");
  uint32_t tileWidth = _options.tileWidth.value_or(256);
  uint32_t tileHeight = _options.tileHeight.value_or(256);

  uint32_t minimumLevel = _options.minimumLevel.value_or(0);
  uint32_t maximumLevel = _options.maximumLevel.value_or(25);

  std::optional<std::map<std::string, std::string>> dimensions =
      _options.dimensions;

  bool useKVP;

  auto countBracket = std::count(_url.begin(), _url.end(), '{');
  if (countBracket < 1 ||
      (countBracket == 1 && _url.find("{s}") != std::string::npos)) {
    useKVP = true;
  } else {
    useKVP = false;
  }

  CesiumGeospatial::Projection projection;
  CesiumGeospatial::GlobeRectangle tilingSchemeRectangle =
      CesiumGeospatial::WebMercatorProjection::MAXIMUM_GLOBE_RECTANGLE;
  uint32_t rootTilesX = 1;
  if (_options.projection) {
    projection = _options.projection.value();
    if (std::get_if<CesiumGeospatial::GeographicProjection>(&projection)) {
      tilingSchemeRectangle =
          CesiumGeospatial::GeographicProjection::MAXIMUM_GLOBE_RECTANGLE;
      rootTilesX = 2;
    }
  } else {
    if (_options.ellipsoid) {
      projection =
          CesiumGeospatial::WebMercatorProjection(_options.ellipsoid.value());
    } else {
      projection = CesiumGeospatial::WebMercatorProjection();
    }
  }
  CesiumGeometry::Rectangle coverageRectangle =
      _options.coverageRectangle.value_or(
          projectRectangleSimple(projection, tilingSchemeRectangle));

  CesiumGeometry::QuadtreeTilingScheme tilingScheme =
      _options.tilingScheme.value_or(CesiumGeometry::QuadtreeTilingScheme(
          coverageRectangle,
          rootTilesX,
          1));

  if (hasError) {
    return asyncSystem
        .createResolvedFuture<RasterOverlay::CreateTileProviderResult>(
            nonstd::make_unexpected(RasterOverlayLoadFailureDetails{
                RasterOverlayLoadType::TileProvider,
                nullptr,
                errorMessage}));
  }
  return asyncSystem
      .createResolvedFuture<RasterOverlay::CreateTileProviderResult>(
          new WebMapTileServiceTileProvider(
              pOwner,
              asyncSystem,
              pAssetAccessor,
              credit,
              pPrepareRendererResources,
              pLogger,
              projection,
              tilingScheme,
              coverageRectangle,
              _url,
              _headers,
              useKVP,
              format,
              tileWidth,
              tileHeight,
              minimumLevel,
              maximumLevel,
              _options.layer,
              _options.style,
              _options.tileMatrixSetID,
              _options.tileMatrixLabels,
              _options.dimensions,
              _options.subdomains));
}

} // namespace CesiumRasterOverlays
