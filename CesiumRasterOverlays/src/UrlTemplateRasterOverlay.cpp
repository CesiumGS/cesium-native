#include <CesiumRasterOverlays/QuadtreeRasterOverlayTileProvider.h>
#include <CesiumRasterOverlays/UrlTemplateRasterOverlay.h>
#include <CesiumUtility/Uri.h>

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

namespace CesiumRasterOverlays {

namespace {
static bool caseInsensitiveCompare(std::string_view lhs, std::string_view rhs) {
  return lhs.size() == rhs.size() &&
         std::equal(lhs.begin(), lhs.end(), rhs.begin(), [](char a, char b) {
           return std::tolower(a) == std::tolower(b);
         });
}
} // namespace

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

    const std::string substitutedUrl = Uri::substituteTemplateParameters(
        this->_url,
        [this, tileID, &rectangle = options.rectangle, &unprojectedRect](
            const std::string& placeholder) {
          if (caseInsensitiveCompare(placeholder, "x")) {
            return std::to_string(tileID.x);
          } else if (caseInsensitiveCompare(placeholder, "y")) {
            return std::to_string(tileID.y);
          } else if (caseInsensitiveCompare(placeholder, "z")) {
            return std::to_string(tileID.level);
          } else if (caseInsensitiveCompare(placeholder, "reverseX")) {
            return std::to_string(
                tileID.computeInvertedX(this->getTilingScheme()));
          } else if (caseInsensitiveCompare(placeholder, "reverseY")) {
            return std::to_string(
                tileID.computeInvertedY(this->getTilingScheme()));
          } else if (caseInsensitiveCompare(placeholder, "reverseZ")) {
            return std::to_string(this->getMaximumLevel() - tileID.level);
          } else if (caseInsensitiveCompare(placeholder, "westDegrees")) {
            return std::to_string(
                Math::radiansToDegrees(unprojectedRect.getWest()));
          } else if (caseInsensitiveCompare(placeholder, "southDegrees")) {
            return std::to_string(
                Math::radiansToDegrees(unprojectedRect.getSouth()));
          } else if (caseInsensitiveCompare(placeholder, "eastDegrees")) {
            return std::to_string(
                Math::radiansToDegrees(unprojectedRect.getEast()));
          } else if (caseInsensitiveCompare(placeholder, "northDegrees")) {
            return std::to_string(
                Math::radiansToDegrees(unprojectedRect.getNorth()));
          } else if (caseInsensitiveCompare(placeholder, "westDegrees")) {
            return std::to_string(
                Math::radiansToDegrees(unprojectedRect.getWest()));
          } else if (caseInsensitiveCompare(placeholder, "westProjected")) {
            return std::to_string(Math::radiansToDegrees(rectangle.minimumY));
          } else if (caseInsensitiveCompare(placeholder, "southProjected")) {
            return std::to_string(Math::radiansToDegrees(rectangle.minimumX));
          } else if (caseInsensitiveCompare(placeholder, "eastProjected")) {
            return std::to_string(Math::radiansToDegrees(rectangle.maximumY));
          } else if (caseInsensitiveCompare(placeholder, "northProjected")) {
            return std::to_string(Math::radiansToDegrees(rectangle.maximumX));
          } else if (caseInsensitiveCompare(placeholder, "width")) {
            return std::to_string(this->getWidth());
          } else if (caseInsensitiveCompare(placeholder, "height")) {
            return std::to_string(this->getHeight());
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
