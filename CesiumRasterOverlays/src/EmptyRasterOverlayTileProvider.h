#pragma once

#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>

namespace CesiumRasterOverlays {

class EmptyRasterOverlayTileProvider
    : public CesiumRasterOverlays::RasterOverlayTileProvider {
public:
  EmptyRasterOverlayTileProvider(
      const CesiumUtility::IntrusivePointer<
          const CesiumRasterOverlays::RasterOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem) noexcept;

protected:
  virtual CesiumAsync::Future<CesiumRasterOverlays::LoadedRasterOverlayImage>
  loadTileImage(
      const CesiumRasterOverlays::RasterOverlayTile& overlayTile) override;
};

} // namespace CesiumRasterOverlays
