#include "EmptyRasterOverlayTileProvider.h"

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/IntrusivePointer.h>

#include <optional>

using namespace CesiumRasterOverlays;

namespace Cesium3DTilesSelection {

EmptyRasterOverlayTileProvider::EmptyRasterOverlayTileProvider(
    const CesiumUtility::IntrusivePointer<const RasterOverlay>& pOwner,
    const CesiumAsync::AsyncSystem& asyncSystem) noexcept
    : RasterOverlayTileProvider(
          pOwner,
          asyncSystem,
          nullptr,
          std::nullopt,
          nullptr,
          nullptr,
          CesiumGeospatial::GeographicProjection(),
          CesiumGeometry::Rectangle()) {}

CesiumAsync::Future<CesiumRasterOverlays::LoadedRasterOverlayImage>
EmptyRasterOverlayTileProvider::loadTileImage(
    CesiumRasterOverlays::RasterOverlayTile& /*overlayTile*/) {
  return this->getAsyncSystem()
      .createResolvedFuture<CesiumRasterOverlays::LoadedRasterOverlayImage>({});
}

} // namespace Cesium3DTilesSelection
