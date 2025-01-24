#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeometry/Rectangle.h>
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
#include <CesiumRasterOverlays/WebMapTileServiceRasterOverlay.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/Uri.h>

#include <nonstd/expected.hpp>
#include <spdlog/logger.h>

#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

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
      const std::optional<std::vector<std::string>>& tileMatrixLabels,
      const std::optional<std::map<std::string, std::string>>& dimensions,
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
        _layer(std::move(layer)),
        _style(std::move(style)),
        _tileMatrixSetID(std::move(_tileMatrixSetID)),
        _labels(tileMatrixLabels),
        _staticDimensions(dimensions),
        _subdomains(subdomains) {}

  virtual ~WebMapTileServiceTileProvider() = default;

protected:
  virtual CesiumAsync::Future<LoadedRasterOverlayImage> loadQuadtreeTileImage(
      const CesiumGeometry::QuadtreeTileID& tileID) const override {

    LoadTileImageFromUrlOptions options;
    options.rectangle = this->getTilingScheme().tileToRectangle(tileID);
    options.moreDetailAvailable = tileID.level < this->getMaximumLevel();

    uint32_t level = tileID.level;
    uint32_t col = tileID.x;
    uint32_t row = tileID.computeInvertedY(this->getTilingScheme());

    std::map<std::string, std::string> urlTemplateMap;
    std::string tileMatrix;
    if (_labels && level < _labels.value().size()) {
      tileMatrix = _labels.value()[level];
    } else {
      tileMatrix = std::to_string(level);
    }

    std::string queryString = "?";
    if (this->_url.find(queryString) != std::string::npos) {
      queryString = "&";
    }
    std::string urlTemplate = _url;

    if (!_useKVP) {
      urlTemplateMap.insert(
          {{"Layer", _layer},
           {"Style", _style},
           {"TileMatrix", tileMatrix},
           {"TileRow", std::to_string(row)},
           {"TileCol", std::to_string(col)},
           {"TileMatrixSet", _tileMatrixSetID}});
      if (_subdomains.size() > 0) {
        urlTemplateMap.emplace(
            "s",
            _subdomains[(col + row + level) % _subdomains.size()]);
      }
      if (_staticDimensions) {
        urlTemplateMap.insert(
            _staticDimensions->begin(),
            _staticDimensions->end());
      }
    } else {
      urlTemplateMap.insert(
          {{"layer", _layer},
           {"style", _style},
           {"tilematrix", tileMatrix},
           {"tilerow", std::to_string(row)},
           {"tilecol", std::to_string(col)},
           {"tilematrixset", _tileMatrixSetID},
           {"format", _format}}); // !! These are query parameters
      if (_staticDimensions) {
        urlTemplateMap.insert(
            _staticDimensions->begin(),
            _staticDimensions->end());
      }
      if (_subdomains.size() > 0) {
        urlTemplateMap.emplace(
            "s",
            _subdomains[(col + row + level) % _subdomains.size()]);
      }

      urlTemplate +=
          queryString +
          "request=GetTile&version=1.0.0&service=WMTS&"
          "format={format}&layer={layer}&style={style}&"
          "tilematrixset={tilematrixset}&"
          "tilematrix={tilematrix}&tilerow={tilerow}&tilecol={tilecol}";
    }

    std::string url = CesiumUtility::Uri::substituteTemplateParameters(
        urlTemplate,
        [&map = urlTemplateMap](const std::string& placeholder) {
          auto it = map.find(placeholder);
          return it == map.end() ? "{" + placeholder + "}"
                                 : CesiumUtility::Uri::escape(it->second);
        });

    return this->loadTileImageFromUrl(url, this->_headers, std::move(options));
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

WebMapTileServiceRasterOverlay::~WebMapTileServiceRasterOverlay() = default;

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

  std::optional<Credit> credit = std::nullopt;
  if (pCreditSystem && this->_options.credit) {
    credit = pCreditSystem->createCredit(
        *this->_options.credit,
        pOwner->getOptions().showCreditsOnScreen);
  }

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

  CesiumGeospatial::Projection projection = _options.projection.value_or(
      CesiumGeospatial::WebMercatorProjection(pOwner->getOptions().ellipsoid));
  CesiumGeospatial::GlobeRectangle tilingSchemeRectangle =
      CesiumGeospatial::WebMercatorProjection::MAXIMUM_GLOBE_RECTANGLE;
  uint32_t rootTilesX = 1;
  if (std::get_if<CesiumGeospatial::GeographicProjection>(&projection)) {
    tilingSchemeRectangle =
        CesiumGeospatial::GeographicProjection::MAXIMUM_GLOBE_RECTANGLE;
    rootTilesX = 2;
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
