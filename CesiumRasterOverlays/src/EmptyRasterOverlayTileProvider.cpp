#include "EmptyRasterOverlayTileProvider.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/IntrusivePointer.h>

namespace CesiumRasterOverlays {

EmptyRasterOverlayTileProvider::EmptyRasterOverlayTileProvider(
    const CesiumUtility::IntrusivePointer<const RasterOverlay>& pCreator,
    const CreateRasterOverlayTileProviderOptions& options) noexcept
    : RasterOverlayTileProvider(
          pCreator,
          options,
          CesiumGeospatial::GeographicProjection(),
          CesiumGeometry::Rectangle()) {}

CesiumAsync::Future<CesiumRasterOverlays::LoadedRasterOverlayImage>
EmptyRasterOverlayTileProvider::loadTileImage(
    const CesiumRasterOverlays::RasterOverlayTile& /*overlayTile*/) {
  return this->getAsyncSystem()
      .createResolvedFuture<CesiumRasterOverlays::LoadedRasterOverlayImage>({});
}

} // namespace CesiumRasterOverlays
