#pragma once

#include "Library.h"

#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGltf/Ktx2TranscodeTargets.h>

#include <spdlog/fwd.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace Cesium3DTilesSelection {

class CreditSystem;
class IPrepareRendererResources;
class RasterOverlayTileProvider;
class RasterOverlayCollection;
class RasterOverlayLoadFailureDetails;

/**
 * @brief Options for loading raster overlays.
 */
struct CESIUM3DTILESSELECTION_API RasterOverlayOptions {
  /**
   * @brief The maximum number of overlay tiles that may simultaneously be in
   * the process of loading.
   */
  int32_t maximumSimultaneousTileLoads = 20;

  /**
   * @brief The maximum number of bytes to use to cache sub-tiles in memory.
   *
   * This is used by provider types, such as
   * {@link QuadtreeRasterOverlayTileProvider}, that have an underlying tiling
   * scheme that may not align with the tiling scheme of the geometry tiles on
   * which the raster overlay tiles are draped. Because a single sub-tile may
   * overlap multiple geometry tiles, it is useful to cache loaded sub-tiles
   * in memory in case they're needed again soon. This property controls the
   * maximum size of that cache.
   */
  int64_t subTileCacheBytes = 16 * 1024 * 1024;

  /**
   * @brief The maximum pixel size of raster overlay textures, in either
   * direction.
   *
   * Images created by this overlay will be no more than this number of pixels
   * in either direction. This may result in reduced raster overlay detail in
   * some cases. For example, in a {@link QuadtreeRasterOverlayTileProvider},
   * this property will limit the number of quadtree tiles that may be mapped to
   * a given geometry tile. The selected quadtree level for a geometry tile is
   * reduced in order to stay under this limit.
   */
  int32_t maximumTextureSize = 2048;

  /**
   * @brief The maximum number of pixels of error when rendering this overlay.
   * This is used to select an appropriate level-of-detail.
   *
   * When this property has its default value, 2.0, it means that raster overlay
   * images will be sized so that, when zoomed in closest, a single pixel in
   * the raster overlay maps to approximately 2x2 pixels on the screen.
   */
  double maximumScreenSpaceError = 2.0;

  /**
   * @brief For each possible input transmission format, this struct names
   * the ideal target gpu-compressed pixel format to transcode to.
   */
  CesiumGltf::Ktx2TranscodeTargets ktx2TranscodeTargets;

  /**
   * @brief A callback function that is invoked when a raster overlay resource
   * fails to load.
   *
   * Raster overlay resources include a Cesium ion asset endpoint, any resources
   * required for raster overlay metadata, or an individual overlay image.
   */
  std::function<void(const RasterOverlayLoadFailureDetails&)> loadErrorCallback;

  bool showCreditsOnScreen = false;
};

/**
 * @brief The base class for a quadtree-tiled raster image that can be draped
 * over a {@link Tileset}.
 *
 * Instances of this class can be added to the {@link RasterOverlayCollection}
 * that is returned by {@link Tileset::getOverlays}.
 *
 * @see BingMapsRasterOverlay
 * @see IonRasterOverlay
 * @see TileMapServiceRasterOverlay
 */
class RasterOverlay {
public:
  /**
   * @brief Creates a new instance.
   *
   * @param name The user-given name of this overlay layer.
   * @param overlayOptions The {@link RasterOverlayOptions} for this instance.
   */
  RasterOverlay(
      const std::string& name,
      const RasterOverlayOptions& overlayOptions = RasterOverlayOptions());
  virtual ~RasterOverlay();

  /**
   * @brief Gets the name of this overlay.
   */
  const std::string& getName() const noexcept { return this->_name; }

  /**
   * @brief Gets options for this overlay.
   */
  RasterOverlayOptions& getOptions() noexcept { return this->_options; }

  /** @copydoc getOptions */
  const RasterOverlayOptions& getOptions() const noexcept {
    return this->_options;
  }

  /**
   * @brief Gets the tile provider for this overlay.
   *
   * @return `nullptr` if {@link createTileProvider} has not yet been called or
   * caused an error. If {@link createTileProvider} has been called but the
   * overlay is not yet ready to provide tiles, a placeholder tile provider will
   * be returned.
   */
  RasterOverlayTileProvider* getTileProvider() noexcept;

  /** @copydoc getTileProvider */
  const RasterOverlayTileProvider* getTileProvider() const noexcept;

  /**
   * @brief Gets the placeholder tile provider for this overlay.
   *
   * @return `nullptr` if {@link createTileProvider} has not yet been called or
   * caused an error
   */
  RasterOverlayTileProvider* getPlaceholder() noexcept {
    return this->_pPlaceholder.get();
  }

  /** @copydoc getPlaceholder */
  const RasterOverlayTileProvider* getPlaceholder() const noexcept {
    return this->_pPlaceholder.get();
  }

  /**
   * @brief Returns whether this overlay is in the process of being destroyed.
   */
  bool isBeingDestroyed() const noexcept { return this->_pSelf != nullptr; }

  /**
   * @brief Begins asynchronous creation of the tile provider for this overlay
   * and eventually makes it available directly from this instance.
   *
   * When the tile provider is ready, it will be returned by
   * {@link getTileProvider}.
   *
   * This method does nothing if the tile provider has already been created or
   * is already in the process of being created.
   *
   * @param asyncSystem The async system used to do work in threads.
   * @param pAssetAccessor The interface used to download assets like overlay
   * metadata and tiles.
   * @param pCreditSystem The {@link CreditSystem} to use when creating a
   * per-TileProvider {@link Credit}.
   * @param pPrepareRendererResources The interface used to prepare raster
   * images for rendering.
   * @param pLogger The logger to which to send messages about the tile provider
   * and tiles.
   */
  CesiumAsync::SharedFuture<std::unique_ptr<RasterOverlayTileProvider>>
  loadTileProvider(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CreditSystem>& pCreditSystem,
      const std::shared_ptr<IPrepareRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger);

  /**
   * @brief Begins asynchronous creation of the tile provider for this overlay
   * and eventually returns it via a Future.
   *
   * The created tile provider will not be returned via {@link getTileProvider}.
   * This method is primarily useful for overlays that aggregate other overlays.
   *
   * @param asyncSystem The async system used to do work in threads.
   * @param pAssetAccessor The interface used to download assets like overlay
   * metadata and tiles.
   * @param pCreditSystem The {@link CreditSystem} to use when creating a
   * per-TileProvider {@link Credit}.
   * @param pPrepareRendererResources The interface used to prepare raster
   * images for rendering.
   * @param pLogger The logger to which to send messages about the tile provider
   * and tiles.
   * @param pOwner The overlay that owns this overlay, or nullptr if this
   * overlay is not aggregated.
   * @return The future that contains the tile provider when it is ready, or the
   * `nullptr` in case of an error.
   */
  virtual CesiumAsync::Future<std::unique_ptr<RasterOverlayTileProvider>>
  createTileProvider(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CreditSystem>& pCreditSystem,
      const std::shared_ptr<IPrepareRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      RasterOverlay* pOwner) = 0;

  /**
   * @brief Safely destroys this overlay.
   *
   * This method is not supposed to be called by clients.
   * The overlay will not be truly destroyed until all in-progress tile loads
   * complete. This may happen before this function returns if no loads are in
   * progress.
   *
   * @param pOverlay A unique pointer to this instance, allowing transfer of
   * ownership.
   */
  void destroySafely(std::unique_ptr<RasterOverlay>&& pOverlay) noexcept;

protected:
  void reportError(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<spdlog::logger>& pLogger,
      RasterOverlayLoadFailureDetails&& errorDetails);

private:
  std::string _name;
  std::unique_ptr<RasterOverlayTileProvider> _pPlaceholder;
  std::unique_ptr<RasterOverlay> _pSelf;
  RasterOverlayOptions _options;
  std::optional<
      CesiumAsync::SharedFuture<std::unique_ptr<RasterOverlayTileProvider>>>
      _loadingTileProvider;
};

} // namespace Cesium3DTilesSelection
