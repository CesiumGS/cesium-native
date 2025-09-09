#pragma once

#include <CesiumAsync/Future.h>
#include <CesiumRasterOverlays/Library.h>
#include <CesiumRasterOverlays/LoadedRasterOverlayImage.h>

namespace CesiumRasterOverlays {

class RasterOverlayTileProvider;
class RasterOverlayTile;

class IRasterOverlayTileLoader {
public:
  /**
   * @brief Loads the image for a tile.
   *
   * @param provider The tile provider that is loading the image.
   * @param overlayTile The overlay tile for which to load the image.
   * @return A future that resolves to the image or error information.
   */
  virtual CesiumAsync::Future<LoadedRasterOverlayImage> loadTileImage(
      const RasterOverlayTileProvider& provider,
      const RasterOverlayTile& overlayTile) = 0;

  virtual void addReference() const = 0;
  virtual void releaseReference() const = 0;
};

} // namespace CesiumRasterOverlays
