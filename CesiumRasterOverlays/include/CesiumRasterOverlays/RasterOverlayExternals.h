#pragma once

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumRasterOverlays/IPrepareRasterOverlayRendererResources.h>
#include <CesiumRasterOverlays/Library.h>
#include <CesiumUtility/CreditSystem.h>

#include <spdlog/spdlog.h>

#include <memory>

namespace CesiumRasterOverlays {

/**
 * @brief External interfaces used by a @ref RasterOverlay.
 */
class CESIUMRASTEROVERLAYS_API RasterOverlayExternals final {
public:
  /**
   * @brief The @ref CesiumAsync::IAssetAccessor that is used to download
   * raster overlay tiles and other assets.
   *
   * This may only be `nullptr` if the raster overlay does not attempt to
   * download any resources.
   */
  std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor;

  /**
   * @brief The @ref IPrepareRasterOverlayRendererResources that is used to
   * create renderer-specific resources for raster overlay tiles.
   *
   * This may be `nullptr` if the renderer does not need to create any resources
   * for raster overlays.
   */
  std::shared_ptr<IPrepareRasterOverlayRendererResources>
      pPrepareRendererResources;

  /**
   * @brief The async system to use to do work in threads.
   */
  CesiumAsync::AsyncSystem asyncSystem;

  /**
   * @brief The @ref CesiumUtility::CreditSystem that can be used to manage
   * credit strings and periodically query which credits to show and which to
   * remove from the screen.
   *
   * While not recommended, this may be `nullptr` if the client does not need to
   * receive credits.
   */
  std::shared_ptr<CesiumUtility::CreditSystem> pCreditSystem;

  /**
   * @brief A spdlog logger that will receive log messages.
   *
   * If not specified, defaults to `spdlog::default_logger()`.
   */
  std::shared_ptr<spdlog::logger> pLogger = spdlog::default_logger();
};

} // namespace CesiumRasterOverlays
