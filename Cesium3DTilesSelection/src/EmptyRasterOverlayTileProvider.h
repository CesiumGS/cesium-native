#pragma once

#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>

namespace Cesium3DTilesSelection {

class EmptyRasterOverlayTileProvider
    : public CesiumRasterOverlays::RasterOverlayTileProvider {
public:
  EmptyRasterOverlayTileProvider(
      const CesiumUtility::IntrusivePointer<
          const CesiumRasterOverlays::RasterOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem) noexcept;

protected:
  virtual CesiumAsync::Future<CesiumRasterOverlays::LoadedRasterOverlayImage>
  loadTileImage(CesiumRasterOverlays::RasterOverlayTile& overlayTile) override;
};

} // namespace Cesium3DTilesSelection
