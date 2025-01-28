#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/UrlTemplateRasterOverlay.h>

#include <doctest/doctest.h>

using namespace CesiumRasterOverlays;

namespace {

class MyRasterOverlay : public RasterOverlay {
public:
  MyRasterOverlay() : RasterOverlay("name", {}) {}

  virtual CesiumAsync::Future<CreateTileProviderResult> createTileProvider(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CesiumUtility::CreditSystem>& pCreditSystem,
      const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner)
      const override;
};

//! [use-url-template]
CesiumAsync::Future<RasterOverlay::CreateTileProviderResult>
MyRasterOverlay::createTileProvider(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CesiumUtility::CreditSystem>& pCreditSystem,
    const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
        pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner) const {
  // Create a new raster overlay with a URL template.
  CesiumGeometry::Rectangle coverageRectangle = CesiumGeospatial::
      WebMercatorProjection::computeMaximumProjectedRectangle();

  UrlTemplateRasterOverlayOptions options{
      .credit = "Copyright (c) Some Amazing Source",
      .projection = CesiumGeospatial::WebMercatorProjection(),
      .tilingScheme =
          CesiumGeometry::QuadtreeTilingScheme(coverageRectangle, 1, 1),
      .minimumLevel = 0,
      .maximumLevel = 15,
      .tileWidth = 256,
      .tileHeight = 256,
      .coverageRectangle = coverageRectangle,
  };

  CesiumUtility::IntrusivePointer<RasterOverlay> pUrlTemplate =
      new UrlTemplateRasterOverlay(
          this->getName(),
          "https://example.com/level-{z}/column-{x}/row-{y}.png",
          {},
          options);

  // Get that raster overlay's tile provider.
  return pUrlTemplate->createTileProvider(
      asyncSystem,
      pAssetAccessor,
      pCreditSystem,
      pPrepareRendererResources,
      pLogger,
      pOwner != nullptr ? pOwner : this);
}
//! [use-url-template]

} // namespace

TEST_CASE("RasterOverlay examples") {
  CesiumUtility::IntrusivePointer<RasterOverlay> pOverlay =
      new MyRasterOverlay();
}