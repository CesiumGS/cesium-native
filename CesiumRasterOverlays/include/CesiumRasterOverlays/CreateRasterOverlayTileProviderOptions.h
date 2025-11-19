#pragma once

#include <CesiumRasterOverlays/Library.h>
#include <CesiumRasterOverlays/RasterOverlayExternals.h>
#include <CesiumUtility/IntrusivePointer.h>

namespace CesiumUtility {
class CreditSource;
}

namespace CesiumRasterOverlays {

class RasterOverlay;

/**
 * @brief Options passed to \ref RasterOverlay::createTileProvider.
 */
struct CESIUMRASTEROVERLAYS_API CreateRasterOverlayTileProviderOptions {
  /**
   * @brief The external interfaces for use by the raster overlay tile provider.
   */
  RasterOverlayExternals externals;

  /**
   * @brief The overlay that owns this overlay.
   *
   * If nullptr, this overlay is not aggregated, and the owner of the tile
   * provider is the overlay that created it.
   */
  CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner;

  /**
   * @brief The credit source that the new tile provider should use for its
   * credits.
   *
   * If nullptr, a new credit source will be created for the tile provider.
   */
  std::shared_ptr<CesiumUtility::CreditSource> pCreditSource;
};

} // namespace CesiumRasterOverlays
