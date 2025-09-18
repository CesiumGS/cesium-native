#include "EmptyRasterOverlayTileProvider.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/IntrusivePointer.h>

#include <optional>

namespace CesiumRasterOverlays {

EmptyRasterOverlayTileProvider::EmptyRasterOverlayTileProvider(
    const CesiumUtility::IntrusivePointer<const RasterOverlay>& pOwner,
    const CesiumAsync::AsyncSystem& asyncSystem) noexcept
    : RasterOverlayTileProvider(
          pOwner,
          asyncSystem,
          nullptr,
          nullptr,
          std::nullopt,
          nullptr,
          nullptr,
          CesiumGeospatial::GeographicProjection(),
          CesiumGeometry::Rectangle()) {}

CesiumAsync::Future<CesiumRasterOverlays::LoadedRasterOverlayImage>
EmptyRasterOverlayTileProvider::loadTileImage(
    const CesiumRasterOverlays::RasterOverlayTile& /*overlayTile*/) {
  return this->getAsyncSystem()
      .createResolvedFuture<CesiumRasterOverlays::LoadedRasterOverlayImage>({});
}

} // namespace CesiumRasterOverlays
