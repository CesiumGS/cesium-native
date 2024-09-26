#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumRasterOverlays/DebugColorizeTilesRasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/SpanHelper.h>

using namespace CesiumRasterOverlays;
using namespace CesiumGeospatial;
using namespace CesiumGltf;
using namespace CesiumUtility;

namespace {

class DebugTileProvider : public RasterOverlayTileProvider {
public:
  DebugTileProvider(
      const IntrusivePointer<const RasterOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const Ellipsoid& ellipsoid)
      : RasterOverlayTileProvider(
            pOwner,
            asyncSystem,
            pAssetAccessor,
            std::nullopt,
            pPrepareRendererResources,
            pLogger,
            GeographicProjection(ellipsoid),
            GeographicProjection::computeMaximumProjectedRectangle(ellipsoid)) {
  }

  virtual CesiumAsync::Future<LoadedRasterOverlayImage>
  loadTileImage(RasterOverlayTile& overlayTile) override {
    LoadedRasterOverlayImage result;

    // Indicate that there is no more detail available so that tiles won't get
    // refined on our behalf.
    result.moreDetailAvailable = false;

    result.rectangle = overlayTile.getRectangle();

    ImageCesium& image = result.image.emplace();
    image.width = 1;
    image.height = 1;
    image.channels = 4;
    image.bytesPerChannel = 1;
    image.pixelData.resize(4);
    gsl::span<uint32_t> pixels =
        reintepretCastSpan<uint32_t>(gsl::span(image.pixelData));
    int red = rand() % 255;
    int green = rand() % 255;
    int blue = rand() % 255;
    uint32_t color = 0x7F000000;
    color += uint32_t(red) << 16;
    color += uint32_t(green) << 8;
    color += uint32_t(blue);
    pixels[0] = color;

    return this->getAsyncSystem().createResolvedFuture(std::move(result));
  }
};

} // namespace

DebugColorizeTilesRasterOverlay::DebugColorizeTilesRasterOverlay(
    const std::string& name,
    const RasterOverlayOptions& overlayOptions)
    : RasterOverlay(name, overlayOptions),
      _ellipsoid(overlayOptions.ellipsoid.value_or(Ellipsoid::WGS84)) {}

CesiumAsync::Future<RasterOverlay::CreateTileProviderResult>
DebugColorizeTilesRasterOverlay::createTileProvider(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& /* pCreditSystem */,
    const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
        pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner) const {
  pOwner = pOwner ? pOwner : this;

  return asyncSystem.createResolvedFuture<CreateTileProviderResult>(
      IntrusivePointer<RasterOverlayTileProvider>(new DebugTileProvider(
          pOwner,
          asyncSystem,
          pAssetAccessor,
          pPrepareRendererResources,
          pLogger,
          this->_ellipsoid)));
}
