#include <CesiumRasterOverlays/CreateRasterOverlayTileProviderOptions.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/UrlTemplateRasterOverlay.h>

#include <doctest/doctest.h>

using namespace CesiumRasterOverlays;

namespace {

class MyRasterOverlay : public RasterOverlay {
public:
  MyRasterOverlay() : RasterOverlay("name", {}) {}

  virtual CesiumAsync::Future<CreateTileProviderResult> createTileProvider(
      const CreateRasterOverlayTileProviderOptions& options) const override;
};

//! [use-url-template]
CesiumAsync::Future<RasterOverlay::CreateTileProviderResult>
MyRasterOverlay::createTileProvider(
    const CreateRasterOverlayTileProviderOptions& options) const {
  // Create a new raster overlay with a URL template.
  CesiumGeometry::Rectangle coverageRectangle = CesiumGeospatial::
      WebMercatorProjection::computeMaximumProjectedRectangle();

  UrlTemplateRasterOverlayOptions urlTemplateOptions{
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
          urlTemplateOptions);

  CreateRasterOverlayTileProviderOptions optionsCopy = options;
  if (!optionsCopy.pOwner) {
    optionsCopy.pOwner = this;
  }

  // Get that raster overlay's tile provider.
  return pUrlTemplate->createTileProvider(optionsCopy);
}
//! [use-url-template]

} // namespace

TEST_CASE("RasterOverlay examples") {
  CesiumUtility::IntrusivePointer<RasterOverlay> pOverlay =
      new MyRasterOverlay();
}