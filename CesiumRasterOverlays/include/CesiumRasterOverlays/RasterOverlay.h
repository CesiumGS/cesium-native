#pragma once

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGltf/Ktx2TranscodeTargets.h>
#include <CesiumRasterOverlays/Library.h>
#include <CesiumRasterOverlays/RasterOverlayLoadFailureDetails.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/ReferenceCounted.h>

#include <nonstd/expected.hpp>
#include <spdlog/fwd.h>

#include <any>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace CesiumUtility {
struct Credit;
class CreditSystem;
} // namespace CesiumUtility

namespace CesiumRasterOverlays {

class IPrepareRasterOverlayRendererResources;
class RasterOverlayTileProvider;

/**
 * @brief Options for loading raster overlays.
 */
struct CESIUMRASTEROVERLAYS_API RasterOverlayOptions {
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
  int64_t subTileCacheBytes = static_cast<int64_t>(16 * 1024 * 1024);

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
   * Raster overlay resources include a Cesium ion asset endpoint or any
   * resources required for raster overlay metadata.
   *
   * This callback is invoked by the {@link Cesium3DTilesSelection::RasterOverlayCollection} when an
   * error occurs while it is creating a tile provider for this RasterOverlay.
   * It is always invoked in the main thread.
   */
  std::function<void(const RasterOverlayLoadFailureDetails&)> loadErrorCallback;

  /**
   * @brief Whether or not to display the credits on screen.
   */
  bool showCreditsOnScreen = false;

  /**
   * @brief Arbitrary data that will be passed to {@link Cesium3DTilesSelection::IPrepareRendererResources::prepareRasterInLoadThread},
   * for example, data to control the per-raster overlay client-specific texture
   * properties.
   *
   * This object is copied and given to background texture preparation threads,
   * so it must be inexpensive to copy.
   */
  std::any rendererOptions;

  /**
   * @brief The ellipsoid used for this raster overlay.
   */
  CesiumGeospatial::Ellipsoid ellipsoid = CesiumGeospatial::Ellipsoid::WGS84;
};

/**
 * @brief The base class for a rasterized image that can be draped
 * over a {@link Cesium3DTilesSelection::Tileset}. The image may be very, very high resolution, so only
 * small pieces of it are mapped to the Tileset at a time.
 *
 * Instances of this class can be added to the {@link Cesium3DTilesSelection::RasterOverlayCollection}
 * that is returned by {@link Cesium3DTilesSelection::Tileset::getOverlays}.
 *
 * Instances of this class must be allocated on the heap, and their lifetimes
 * must be managed with {@link CesiumUtility::IntrusivePointer}.
 *
 * @see BingMapsRasterOverlay
 * @see IonRasterOverlay
 * @see TileMapServiceRasterOverlay
 * @see WebMapServiceRasterOverlay
 */
class RasterOverlay
    : public CesiumUtility::ReferenceCountedNonThreadSafe<RasterOverlay> {
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
  virtual ~RasterOverlay() noexcept;

  /**
   * @brief A future that resolves when this RasterOverlay has been destroyed
   * (i.e. its destructor has been called) and all async operations that it was
   * executing have completed.
   *
   * @param asyncSystem The AsyncSystem to use for the returned SharedFuture,
   * if required. If this method is called multiple times, all invocations
   * must pass {@link CesiumAsync::AsyncSystem} instances that compare equal to each other.
   */
  CesiumAsync::SharedFuture<void>&
  getAsyncDestructionCompleteEvent(const CesiumAsync::AsyncSystem& asyncSystem);

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
   * @brief Gets the credits for this overlay.
   */
  const std::vector<CesiumUtility::Credit>& getCredits() const noexcept {
    return this->_credits;
  }

  /**
   * @brief Gets the credits for this overlay.
   */
  std::vector<CesiumUtility::Credit>& getCredits() noexcept {
    return this->_credits;
  }

  /**
   * @brief Create a placeholder tile provider can be used in place of the real
   * one while {@link createTileProvider} completes asynchronously.
   *
   * @param asyncSystem The async system used to do work in threads.
   * @param pAssetAccessor The interface used to download assets like overlay
   * metadata and tiles.
   * @param ellipsoid The {@link CesiumGeospatial::Ellipsoid}.
   * @return The placeholder.
   */
  CesiumUtility::IntrusivePointer<RasterOverlayTileProvider> createPlaceholder(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const CesiumGeospatial::Ellipsoid& ellipsoid
          CESIUM_DEFAULT_ELLIPSOID) const;

  /**
   * @brief A result from a call to \ref createTileProvider. This is expected to
   * be an \ref CesiumUtility::IntrusivePointer "IntrusivePointer" to a \ref
   * RasterOverlayTileProvider, but may be a \ref
   * RasterOverlayLoadFailureDetails if creating the tile provider wasn't
   * successful.
   */
  using CreateTileProviderResult = nonstd::expected<
      CesiumUtility::IntrusivePointer<RasterOverlayTileProvider>,
      RasterOverlayLoadFailureDetails>;

  /**
   * @brief Begins asynchronous creation of a tile provider for this overlay
   * and eventually returns it via a Future.
   *
   * @param asyncSystem The async system used to do work in threads.
   * @param pAssetAccessor The interface used to download assets like overlay
   * metadata and tiles.
   * @param pCreditSystem The {@link CesiumUtility::CreditSystem} to use when creating a
   * per-TileProvider {@link CesiumUtility::Credit}.
   * @param pPrepareRendererResources The interface used to prepare raster
   * images for rendering.
   * @param pLogger The logger to which to send messages about the tile provider
   * and tiles.
   * @param pOwner The overlay that owns this overlay, or nullptr if this
   * overlay is not aggregated.
   * @return The future that resolves to the tile provider when it is ready, or
   * to error details in the case of an error.
   */
  virtual CesiumAsync::Future<CreateTileProviderResult> createTileProvider(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CesiumUtility::CreditSystem>& pCreditSystem,
      const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner) const = 0;

private:
  struct DestructionCompleteDetails {
    CesiumAsync::AsyncSystem asyncSystem;
    CesiumAsync::Promise<void> promise;
    CesiumAsync::SharedFuture<void> future;
  };

  std::string _name;
  RasterOverlayOptions _options;
  std::vector<CesiumUtility::Credit> _credits;
  std::optional<DestructionCompleteDetails> _destructionCompleteDetails;
};

} // namespace CesiumRasterOverlays
