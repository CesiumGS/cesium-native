#pragma once

#include "RasterOverlay.h"

namespace Cesium3DTilesSelection {

/**
 * @brief A raster overlay that gives each tile to which it is attached a random
 * color with 50% opacity. This is useful for debugging a tileset, to visualize
 * how it is divided into tiles.
 */
class CESIUM3DTILESSELECTION_API DebugColorizeTilesRasterOverlay
    : public RasterOverlay {
public:
  /**
   * @copydoc RasterOverlay::RasterOverlay
   */
  DebugColorizeTilesRasterOverlay(
      const std::string& name,
      const RasterOverlayOptions& overlayOptions = RasterOverlayOptions());

  /**
   * @copydoc RasterOverlay::createTileProvider
   */
  virtual CesiumAsync::Future<CreateTileProviderResult> createTileProvider(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CreditSystem>& pCreditSystem,
      const std::shared_ptr<IPrepareRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner)
      const override;
};

} // namespace Cesium3DTilesSelection
