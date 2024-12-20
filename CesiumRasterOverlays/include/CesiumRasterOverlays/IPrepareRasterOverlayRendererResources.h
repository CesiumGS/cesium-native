#pragma once

#include <CesiumRasterOverlays/Library.h>

#include <any>

namespace CesiumGltf {
struct ImageAsset;
}

namespace CesiumRasterOverlays {
class RasterOverlayTile;
}

namespace CesiumRasterOverlays {

/**
 * @brief An interface between Cesium Native and the application using it,
 * allowing Cesium Native to pass loaded raster overlay data to the implementing
 * application in order for the application to prepare it to be used in its
 * renderer of choice. This could involve creating a texture asset, uploading
 * the texture data to the GPU, or any other similar tasks required.
 */
class CESIUMRASTEROVERLAYS_API IPrepareRasterOverlayRendererResources {
public:
  /**
   * @brief Prepares a raster overlay tile.
   *
   * This method is invoked in the load thread and may modify the image.
   *
   * @param image The raster tile image to prepare.
   * @param rendererOptions Renderer options associated with the raster overlay tile from {@link RasterOverlayOptions::rendererOptions}.
   * @returns Arbitrary data representing the result of the load process. This
   * data is passed to {@link prepareRasterInMainThread} as the
   * `pLoadThreadResult` parameter.
   */
  virtual void* prepareRasterInLoadThread(
      CesiumGltf::ImageAsset& image,
      const std::any& rendererOptions) = 0;

  /**
   * @brief Further preprares a raster overlay tile.
   *
   * This is called after {@link prepareRasterInLoadThread}, and unlike that
   * method, this one is called from the same thread that called
   * {@link Cesium3DTilesSelection::Tileset::updateView}.
   *
   * @param rasterTile The raster tile to prepare.
   * @param pLoadThreadResult The value returned from
   * {@link prepareRasterInLoadThread}.
   * @returns Arbitrary data representing the result of the load process. Note
   * that the value returned by {@link prepareRasterInLoadThread} will _not_ be
   * automatically preserved and passed to {@link freeRaster}. If you need to free
   * that value, do it in this method before returning. If you need that value
   * later, add it to the object returned from this method.
   */
  virtual void* prepareRasterInMainThread(
      RasterOverlayTile& rasterTile,
      void* pLoadThreadResult) = 0;

  /**
   * @brief Frees previously-prepared renderer resources for a raster tile.
   *
   * This method is always called from the thread that destroyed the
   * {@link RasterOverlayTile}. When raster overlays are used with tilesets,
   * this is the thread that called {@link Cesium3DTilesSelection::Tileset::updateView} or deleted the
   * tileset.
   *
   * @param rasterTile The tile for which to free renderer resources.
   * @param pLoadThreadResult The result returned by
   * {@link prepareRasterInLoadThread}. If {@link prepareRasterInMainThread}
   * has already been called, this parameter will be `nullptr`.
   * @param pMainThreadResult The result returned by
   * {@link prepareRasterInMainThread}. If {@link prepareRasterInMainThread}
   * has not yet been called, this parameter will be `nullptr`.
   */
  virtual void freeRaster(
      const RasterOverlayTile& rasterTile,
      void* pLoadThreadResult,
      void* pMainThreadResult) noexcept = 0;
};

} // namespace CesiumRasterOverlays
