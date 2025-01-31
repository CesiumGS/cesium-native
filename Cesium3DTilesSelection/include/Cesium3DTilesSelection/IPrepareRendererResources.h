#pragma once

#include <Cesium3DTilesSelection/Library.h>
#include <Cesium3DTilesSelection/TileLoadResult.h>
#include <CesiumAsync/Future.h>
#include <CesiumRasterOverlays/IPrepareRasterOverlayRendererResources.h>

#include <glm/vec2.hpp>

#include <any>
#include <span>

namespace CesiumAsync {
class AsyncSystem;
}

namespace CesiumGeometry {
struct Rectangle;
}

namespace CesiumGltf {
struct Model;
} // namespace CesiumGltf

namespace CesiumRasterOverlays {
class RasterOverlayTile;
}

namespace Cesium3DTilesSelection {

class Tile;

/**
 * The data of a loaded tile together with a pointer to "render resources" data
 * representing the result of \ref
 * IPrepareRendererResources::prepareInLoadThread "prepareInLoadThread".
 */
struct TileLoadResultAndRenderResources {
  /**
   * @brief The \ref TileLoadResult passed to \ref
   * IPrepareRendererResources::prepareInLoadThread "prepareInLoadThread" in the
   * first place.
   */
  TileLoadResult result;
  /**
   * @brief A pointer to the render resources for this tile.

   * Cesium Native doesn't know what this pointer means, and doesn't need to
   * know what it means. This pointer is stored in a tile's content as a \ref
   * TileRenderContent only so that it can be returned to the implementing
   * application as needed and used for rendering there.
   */
  void* pRenderResources{nullptr};
};

/**
 * @brief When implemented for a rendering engine, allows renderer resources to
 * be created and destroyed under the control of a {@link Tileset}.
 *
 * It is not supposed to be used directly by clients. It is implemented
 * for specific rendering engines to provide an infrastructure for preparing the
 * data of a {@link Tile} so that it can be used for rendering.
 *
 * Instances of this class are associated with a {@link Tileset}, in the
 * {@link TilesetExternals} structure that is passed to the constructor.
 */
class CESIUM3DTILESSELECTION_API IPrepareRendererResources
    : public CesiumRasterOverlays::IPrepareRasterOverlayRendererResources {
public:
  virtual ~IPrepareRendererResources() = default;

  /**
   * @brief Prepares renderer resources for the given tile. This method is
   * invoked in the load thread.
   *
   * @param asyncSystem The AsyncSystem used to do work in threads.
   * @param tileLoadResult The tile data loaded so far.
   * @param transform The tile's transformation.
   * @param rendererOptions Renderer options associated with the tile from
   * {@link TilesetOptions::rendererOptions}.
   * @returns A future that resolves to the loaded tile data along with
   * arbitrary "render resources" data representing the result of the load
   * process. The loaded data may be the same as was originally given to this
   * method, or it may be modified. The render resources are passed to
   * {@link prepareInMainThread} as the `pLoadThreadResult` parameter.
   */
  virtual CesiumAsync::Future<TileLoadResultAndRenderResources>
  prepareInLoadThread(
      const CesiumAsync::AsyncSystem& asyncSystem,
      TileLoadResult&& tileLoadResult,
      const glm::dmat4& transform,
      const std::any& rendererOptions) = 0;

  /**
   * @brief Further prepares renderer resources.
   *
   * This is called after {@link prepareInLoadThread}, and unlike that method,
   * this one is called from the same thread that called
   * {@link Tileset::updateView}.
   *
   * @param tile The tile to prepare.
   * @param pLoadThreadResult The value returned from
   * {@link prepareInLoadThread}.
   * @returns Arbitrary data representing the result of the load process.
   * Note that the value returned by {@link prepareInLoadThread} will _not_ be
   * automatically preserved and passed to {@link free}. If you need to free
   * that value, do it in this method before returning. If you need that value
   * later, add it to the object returned from this method.
   */
  virtual void* prepareInMainThread(Tile& tile, void* pLoadThreadResult) = 0;

  /**
   * @brief Frees previously-prepared renderer resources.
   *
   * This method is always called from the thread that called
   * {@link Tileset::updateView} or deleted the tileset.
   *
   * @param tile The tile for which to free renderer resources.
   * @param pLoadThreadResult The result returned by
   * {@link prepareInLoadThread}. If {@link prepareInMainThread} has
   * already been called, this parameter will be `nullptr`.
   * @param pMainThreadResult The result returned by
   * {@link prepareInMainThread}. If {@link prepareInMainThread} has
   * not yet been called, this parameter will be `nullptr`.
   */
  virtual void free(
      Tile& tile,
      void* pLoadThreadResult,
      void* pMainThreadResult) noexcept = 0;

  /**
   * @brief Attaches a raster overlay tile to a geometry tile.
   *
   * @param tile The geometry tile.
   * @param overlayTextureCoordinateID The ID of the overlay texture coordinate
   * set to use.
   * @param rasterTile The raster overlay tile to add. The raster tile will have
   * been previously prepared with a call to {@link prepareRasterInLoadThread}
   * followed by {@link prepareRasterInMainThread}.
   * @param pMainThreadRendererResources The renderer resources for this raster
   * tile, as created and returned by {@link prepareRasterInMainThread}.
   * @param translation The translation to apply to the texture coordinates
   * identified by `overlayTextureCoordinateID`. The texture coordinates to use
   * to sample the raster image are computed as `overlayTextureCoordinates *
   * scale + translation`.
   * @param scale The scale to apply to the texture coordinates identified by
   * `overlayTextureCoordinateID`. The texture coordinates to use to sample the
   * raster image are computed as `overlayTextureCoordinates * scale +
   * translation`.
   */
  virtual void attachRasterInMainThread(
      const Tile& tile,
      int32_t overlayTextureCoordinateID,
      const CesiumRasterOverlays::RasterOverlayTile& rasterTile,
      void* pMainThreadRendererResources,
      const glm::dvec2& translation,
      const glm::dvec2& scale) = 0;

  /**
   * @brief Detaches a raster overlay tile from a geometry tile.
   *
   * @param tile The geometry tile.
   * @param overlayTextureCoordinateID The ID of the overlay texture coordinate
   * set to which the raster tile was previously attached.
   * @param rasterTile The raster overlay tile to remove.
   * @param pMainThreadRendererResources The renderer resources for this raster
   * tile, as created and returned by {@link prepareRasterInMainThread}.
   */
  virtual void detachRasterInMainThread(
      const Tile& tile,
      int32_t overlayTextureCoordinateID,
      const CesiumRasterOverlays::RasterOverlayTile& rasterTile,
      void* pMainThreadRendererResources) noexcept = 0;
};

} // namespace Cesium3DTilesSelection
