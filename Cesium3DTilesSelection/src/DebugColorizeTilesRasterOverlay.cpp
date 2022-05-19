#include "Cesium3DTilesSelection/DebugColorizeTilesRasterOverlay.h"

#include "Cesium3DTilesSelection/RasterOverlayTileProvider.h"

#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumUtility/SpanHelper.h>

using namespace Cesium3DTilesSelection;
using namespace CesiumGeospatial;
using namespace CesiumGltf;
using namespace CesiumUtility;

namespace {

class DebugTileProvider : public RasterOverlayTileProvider {
public:
  DebugTileProvider(
      RasterOverlay& owner,
      const std::shared_ptr<CesiumAsync::AsyncSystem>& pAsyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<IPrepareRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger)
      : RasterOverlayTileProvider(
            owner,
            pAsyncSystem,
            pAssetAccessor,
            std::nullopt,
            pPrepareRendererResources,
            pLogger,
            GeographicProjection(),
            GeographicProjection::computeMaximumProjectedRectangle()) {}

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

    return this->getAsyncSystem()->createResolvedFuture(std::move(result));
  }
};

} // namespace

DebugColorizeTilesRasterOverlay::DebugColorizeTilesRasterOverlay(
    const std::string& name,
    const RasterOverlayOptions& overlayOptions)
    : RasterOverlay(name, overlayOptions) {}

CesiumAsync::Future<std::unique_ptr<RasterOverlayTileProvider>>
DebugColorizeTilesRasterOverlay::createTileProvider(
    const std::shared_ptr<CesiumAsync::AsyncSystem>& pAsyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    const std::shared_ptr<CreditSystem>& /* pCreditSystem */,
    const std::shared_ptr<IPrepareRendererResources>& pPrepareRendererResources,
    const std::shared_ptr<spdlog::logger>& pLogger,
    RasterOverlay* pOwner) {
  pOwner = pOwner ? pOwner : this;

  return pAsyncSystem->createResolvedFuture(
      (std::unique_ptr<RasterOverlayTileProvider>)
          std::make_unique<DebugTileProvider>(
              *this,
              pAsyncSystem,
              pAssetAccessor,
              pPrepareRendererResources,
              pLogger));
}
