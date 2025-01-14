#include <CesiumAsync/HttpHeaders.h>
#include <CesiumRasterOverlays/QuadtreeRasterOverlayTileProvider.h>
#include <CesiumRasterOverlays/UrlTemplateRasterOverlay.h>
#include <CesiumUtility/Uri.h>

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

namespace CesiumRasterOverlays {

class UrlTemplateRasterOverlayTileProvider final
    : public QuadtreeRasterOverlayTileProvider {
public:
  UrlTemplateRasterOverlayTileProvider(
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
      uint32_t width,
      uint32_t height,
      uint32_t minimumLevel,
      uint32_t maximumLevel)
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
        _headers(headers) {}

  virtual ~UrlTemplateRasterOverlayTileProvider() {}

protected:
  virtual CesiumAsync::Future<LoadedRasterOverlayImage> loadQuadtreeTileImage(
      const CesiumGeometry::QuadtreeTileID& tileID) const override {

    LoadTileImageFromUrlOptions options;
    options.rectangle = this->getTilingScheme().tileToRectangle(tileID);
    options.moreDetailAvailable = tileID.level < this->getMaximumLevel();

    const GlobeRectangle unprojectedRect =
        unprojectRectangleSimple(this->getProjection(), options.rectangle);

    const std::map<std::string, std::string, CaseInsensitiveCompare>
        placeholdersMap{
            {"x", std::to_string(tileID.x)},
            {"y", std::to_string(tileID.y)},
            {"z", std::to_string(tileID.level)},
            {"reverseX",
             std::to_string(tileID.computeInvertedX(this->getTilingScheme()))},
            {"reverseY",
             std::to_string(tileID.computeInvertedY(this->getTilingScheme()))},
            {"reverseZ",
             std::to_string(this->getMaximumLevel() - tileID.level)},
            {"westDegrees",
             std::to_string(Math::radiansToDegrees(unprojectedRect.getWest()))},
            {"southDegrees",
             std::to_string(
                 Math::radiansToDegrees(unprojectedRect.getSouth()))},
            {"eastDegrees",
             std::to_string(Math::radiansToDegrees(unprojectedRect.getEast()))},
            {"northDegrees",
             std::to_string(
                 Math::radiansToDegrees(unprojectedRect.getNorth()))},
            {"westProjected",
             std::to_string(
                 Math::radiansToDegrees(options.rectangle.minimumY))},
            {"southProjected",
             std::to_string(
                 Math::radiansToDegrees(options.rectangle.minimumX))},
            {"eastProjected",
             std::to_string(
                 Math::radiansToDegrees(options.rectangle.maximumY))},
            {"northProjected",
             std::to_string(
                 Math::radiansToDegrees(options.rectangle.maximumX))},
            {"width", std::to_string(this->getWidth())},
            {"height", std::to_string(this->getHeight())}};

    const std::string substitutedUrl = Uri::substituteTemplateParameters(
        this->_url,
        [&placeholdersMap](const std::string& placeholder) {
          auto placeholderIt = placeholdersMap.find(placeholder);
          if (placeholderIt != placeholdersMap.end()) {
            return placeholderIt->second;
          }
          return std::string("[UNKNOWN PLACEHOLDER]");
        });
    return this->loadTileImageFromUrl(
        substitutedUrl,
        this->_headers,
        std::move(options));
  }

private:
  std::string _url;
  std::vector<IAssetAccessor::THeader> _headers;
};

CesiumAsync::Future<RasterOverlay::CreateTileProviderResult>
UrlTemplateRasterOverlay::createTileProvider(
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

  return asyncSystem
      .createResolvedFuture<RasterOverlay::CreateTileProviderResult>(
          new UrlTemplateRasterOverlayTileProvider(
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
              _options.tileWidth,
              _options.tileHeight,
              _options.minimumLevel,
              _options.maximumLevel));
}

} // namespace CesiumRasterOverlays