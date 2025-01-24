#pragma once

#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumRasterOverlays/Library.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/ErrorList.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/ReferenceCounted.h>
#include <CesiumUtility/Tracing.h>

#include <spdlog/fwd.h>

#include <optional>

namespace CesiumRasterOverlays {

class RasterOverlay;
class RasterOverlayTile;
class IPrepareRasterOverlayRendererResources;

/**
 * @brief Summarizes the result of loading an image of a {@link RasterOverlay}.
 */
struct CESIUMRASTEROVERLAYS_API LoadedRasterOverlayImage {
  /**
   * @brief The loaded image.
   *
   * This will be nullptr if the loading failed. In this case, the `errors`
   * vector will contain the corresponding error messages.
   */
  CesiumUtility::IntrusivePointer<CesiumGltf::ImageAsset> pImage{nullptr};

  /**
   * @brief The projected rectangle defining the bounds of this image.
   *
   * The rectangle extends from the left side of the leftmost pixel to the
   * right side of the rightmost pixel, and similar for the vertical direction.
   */
  CesiumGeometry::Rectangle rectangle{};

  /**
   * @brief The {@link CesiumUtility::Credit} objects that decribe the attributions that
   * are required when using the image.
   */
  std::vector<CesiumUtility::Credit> credits{};

  /**
   * @brief Errors and warnings from loading the image.
   *
   * If the image was loaded successfully, there should not be any errors (but
   * there may be warnings).
   */
  CesiumUtility::ErrorList errorList{};

  /**
   * @brief Whether more detailed data, beyond this image, is available within
   * the bounds of this image.
   */
  bool moreDetailAvailable = false;

  /**
   * @brief Returns the size of this `LoadedRasterOverlayImage` in bytes.
   */
  int64_t getSizeBytes() const {
    int64_t accum = 0;
    accum += int64_t(sizeof(LoadedRasterOverlayImage));
    accum += int64_t(this->credits.capacity() * sizeof(CesiumUtility::Credit));
    if (this->pImage) {
      accum += this->pImage->getSizeBytes();
    }
    return accum;
  }
};

/**
 * @brief Options for {@link RasterOverlayTileProvider::loadTileImageFromUrl}.
 */
struct LoadTileImageFromUrlOptions {
  /**
   * @brief The rectangle definining the bounds of the image being loaded,
   * expressed in the {@link RasterOverlayTileProvider}'s projection.
   */
  CesiumGeometry::Rectangle rectangle{};

  /**
   * @brief The credits to display with this tile.
   *
   * This property is copied verbatim to the
   * {@link LoadedRasterOverlayImage::credits} property.
   */
  std::vector<CesiumUtility::Credit> credits{};

  /**
   * @brief Whether more detailed data, beyond this image, is available within
   * the bounds of this image.
   */
  bool moreDetailAvailable = true;

  /**
   * @brief Whether empty (zero length) images are accepted as a valid
   * response.
   *
   * If true, an otherwise valid response with zero length will be accepted as
   * a valid 0x0 image. If false, such a response will be reported as an
   * error.
   *
   * {@link RasterOverlayTileProvider::loadTile} and
   * {@link RasterOverlayTileProvider::loadTileThrottled} will treat such an
   * image as "failed" and use the quadtree parent (or ancestor) image
   * instead, but will not report any error.
   *
   * This flag should only be set to `true` when the tile source uses a
   * zero-length response as an indication that this tile is - as expected -
   * not available.
   */
  bool allowEmptyImages = false;
};

class RasterOverlayTileProvider;

/**
 * @brief Holds a tile and its corresponding tile provider. Used as the return
 * value of {@link RasterOverlayTileProvider::loadTile}.
 */
struct TileProviderAndTile {
  /** @brief A \ref CesiumUtility::IntrusivePointer to the \ref
   * RasterOverlayTileProvider used for this tile. */
  CesiumUtility::IntrusivePointer<RasterOverlayTileProvider> pTileProvider;
  /** @brief A \ref CesiumUtility::IntrusivePointer to the \ref
   * RasterOverlayTile used for this tile. */
  CesiumUtility::IntrusivePointer<RasterOverlayTile> pTile;

  ~TileProviderAndTile() noexcept;
};

/**
 * @brief Provides individual tiles for a {@link RasterOverlay} on demand.
 *
 * Instances of this class must be allocated on the heap, and their lifetimes
 * must be managed with {@link CesiumUtility::IntrusivePointer}.
 */
class CESIUMRASTEROVERLAYS_API RasterOverlayTileProvider
    : public CesiumUtility::ReferenceCountedNonThreadSafe<
          RasterOverlayTileProvider> {
public:
  /**
   * Constructs a placeholder tile provider.
   *
   * @see RasterOverlayTileProvider::isPlaceholder
   *
   * @param pOwner The raster overlay that created this tile provider.
   * @param asyncSystem The async system used to do work in threads.
   * @param pAssetAccessor The interface used to obtain assets (tiles, etc.) for
   * this raster overlay.
   * @param ellipsoid The {@link CesiumGeospatial::Ellipsoid}.
   */
  RasterOverlayTileProvider(
      const CesiumUtility::IntrusivePointer<const RasterOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const CesiumGeospatial::Ellipsoid& ellipsoid
          CESIUM_DEFAULT_ELLIPSOID) noexcept;

  /**
   * @brief Creates a new instance.
   *
   * @param pOwner The raster overlay that created this tile provider.
   * @param asyncSystem The async system used to do work in threads.
   * @param pAssetAccessor The interface used to obtain assets (tiles, etc.) for
   * this raster overlay.
   * @param credit The {@link CesiumUtility::Credit} for this tile provider, if it exists.
   * @param pPrepareRendererResources The interface used to prepare raster
   * images for rendering.
   * @param pLogger The logger to which to send messages about the tile provider
   * and tiles.
   * @param projection The {@link CesiumGeospatial::Projection}.
   * @param coverageRectangle The rectangle that bounds all the area covered by
   * this overlay, expressed in projected coordinates.
   */
  RasterOverlayTileProvider(
      const CesiumUtility::IntrusivePointer<const RasterOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      std::optional<CesiumUtility::Credit> credit,
      const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const CesiumGeospatial::Projection& projection,
      const CesiumGeometry::Rectangle& coverageRectangle) noexcept;

  /** @brief Default destructor. */
  virtual ~RasterOverlayTileProvider() noexcept;

  /**
   * @brief Returns whether this is a placeholder.
   *
   * For many types of {@link RasterOverlay}, we can't create a functioning
   * `RasterOverlayTileProvider` right away. For example, we may not know the
   * bounds of the overlay, or what projection it uses, until after we've
   * (asynchronously) loaded a metadata service that gives us this information.
   *
   * So until that real `RasterOverlayTileProvider` becomes available, we use
   * a placeholder. When {@link RasterOverlayTileProvider::getTile} is invoked
   * on a placeholder, it returns a {@link RasterOverlayTile} that is also
   * a placeholder. And whenever we see a placeholder `RasterOverlayTile` in
   * {@link Cesium3DTilesSelection::RasterMappedTo3DTile::update}, we check if the corresponding `RasterOverlay` is
   * ready yet. Once it's ready, we remove the placeholder tile and replace
   * it with the real tiles.
   *
   * So the placeholder system gives us a way to defer the mapping of raster
   * overlay tiles to geometry tiles until that mapping can be determined.
   */
  bool isPlaceholder() const noexcept { return this->_pPlaceholder != nullptr; }

  /**
   * @brief Returns the {@link RasterOverlay} that created this instance.
   */
  RasterOverlay& getOwner() noexcept { return *this->_pOwner; }

  /** @copydoc getOwner */
  const RasterOverlay& getOwner() const noexcept { return *this->_pOwner; }

  /**
   * @brief Get the system to use for asychronous requests and threaded work.
   */
  const std::shared_ptr<CesiumAsync::IAssetAccessor>&
  getAssetAccessor() const noexcept {
    return this->_pAssetAccessor;
  }

  /**
   * @brief Gets the async system used to do work in threads.
   */
  const CesiumAsync::AsyncSystem& getAsyncSystem() const noexcept {
    return this->_asyncSystem;
  }

  /**
   * @brief Gets the interface used to prepare raster overlay images for
   * rendering.
   */
  const std::shared_ptr<IPrepareRasterOverlayRendererResources>&
  getPrepareRendererResources() const noexcept {
    return this->_pPrepareRendererResources;
  }

  /**
   * @brief Gets the logger to which to send messages about the tile provider
   * and tiles.
   */
  const std::shared_ptr<spdlog::logger>& getLogger() const noexcept {
    return this->_pLogger;
  }

  /**
   * @brief Returns the {@link CesiumGeospatial::Projection} of this instance.
   */
  const CesiumGeospatial::Projection& getProjection() const noexcept {
    return this->_projection;
  }

  /**
   * @brief Returns the coverage {@link CesiumGeometry::Rectangle} of this
   * instance.
   */
  const CesiumGeometry::Rectangle& getCoverageRectangle() const noexcept {
    return this->_coverageRectangle;
  }

  /**
   * @brief Returns a new {@link RasterOverlayTile} with the given
   * specifications.
   *
   * The returned tile will not start loading immediately. To start loading,
   * call {@link RasterOverlayTileProvider::loadTile} or
   * {@link RasterOverlayTileProvider::loadTileThrottled}.
   *
   * @param rectangle The rectangle that the returned image must cover. It is
   * allowed to cover a slightly larger rectangle in order to maintain pixel
   * alignment. It may also cover a smaller rectangle when the overlay itself
   * does not cover the entire rectangle.
   * @param targetScreenPixels The maximum number of pixels on the screen that
   * this tile is meant to cover. The overlay image should be approximately this
   * many pixels divided by the
   * {@link RasterOverlayOptions::maximumScreenSpaceError} in order to achieve
   * the desired level-of-detail, but it does not need to be exactly this size.
   * @return The tile.
   */
  CesiumUtility::IntrusivePointer<RasterOverlayTile> getTile(
      const CesiumGeometry::Rectangle& rectangle,
      const glm::dvec2& targetScreenPixels);

  /**
   * @brief Gets the number of bytes of tile data that are currently loaded.
   */
  int64_t getTileDataBytes() const noexcept { return this->_tileDataBytes; }

  /**
   * @brief Returns the number of tiles that are currently loading.
   */
  uint32_t getNumberOfTilesLoading() const noexcept {
    CESIUM_ASSERT(this->_totalTilesCurrentlyLoading > -1);
    return static_cast<uint32_t>(this->_totalTilesCurrentlyLoading);
  }

  /**
   * @brief Removes a no-longer-referenced tile from this provider's cache and
   * deletes it.
   *
   * This function is not supposed to be called by client. Calling this method
   * in a tile with a reference count greater than 0 will result in undefined
   * behavior.
   *
   * @param pTile The tile, which must have no oustanding references.
   */
  void removeTile(RasterOverlayTile* pTile) noexcept;

  /**
   * @brief Get the per-TileProvider {@link CesiumUtility::Credit} if one exists.
   */
  const std::optional<CesiumUtility::Credit>& getCredit() const noexcept {
    return _credit;
  }

  /**
   * @brief Loads a tile immediately, without throttling requests.
   *
   * If the tile is not in the `Tile::LoadState::Unloaded` state, this method
   * returns without doing anything. Otherwise, it puts the tile into the
   * `Tile::LoadState::Loading` state and begins the asynchronous process
   * to load the tile. When the process completes, the tile will be in the
   * `Tile::LoadState::Loaded` or `Tile::LoadState::Failed` state.
   *
   * Calling this method on many tiles at once can result in very slow
   * performance. Consider using {@link loadTileThrottled} instead.
   *
   * @param tile The tile to load.
   * @return A future that, when the tile is loaded, resolves to the loaded tile
   * and the tile provider that loaded it.
   */
  CesiumAsync::Future<TileProviderAndTile> loadTile(RasterOverlayTile& tile);

  /**
   * @brief Loads a tile, unless there are too many tile loads already in
   * progress.
   *
   * If the tile is not in the `Tile::LoadState::Unloading` state, this method
   * returns true without doing anything. If too many tile loads are
   * already in flight, it returns false without doing anything. Otherwise, it
   * puts the tile into the `Tile::LoadState::Loading` state, begins the
   * asynchronous process to load the tile, and returns true. When the process
   * completes, the tile will be in the `Tile::LoadState::Loaded` or
   * `Tile::LoadState::Failed` state.
   *
   * The number of allowable simultaneous tile requests is provided in the
   * {@link RasterOverlayOptions::maximumSimultaneousTileLoads} property of
   * {@link RasterOverlay::getOptions}.
   *
   * @param tile The tile to load.
   * @returns True if the tile load process is started or is already complete,
   * false if the load could not be started because too many loads are already
   * in progress.
   */
  bool loadTileThrottled(RasterOverlayTile& tile);

protected:
  /**
   * @brief Loads the image for a tile.
   *
   * @param overlayTile The overlay tile for which to load the image.
   * @return A future that resolves to the image or error information.
   */
  virtual CesiumAsync::Future<LoadedRasterOverlayImage>
  loadTileImage(RasterOverlayTile& overlayTile) = 0;

  /**
   * @brief Loads an image from a URL and optionally some request headers.
   *
   * This is a useful helper function for implementing {@link loadTileImage}.
   *
   * @param url The URL.
   * @param headers The request headers.
   * @param options Additional options for the load process.
   * @return A future that resolves to the image or error information.
   */
  CesiumAsync::Future<LoadedRasterOverlayImage> loadTileImageFromUrl(
      const std::string& url,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers = {},
      LoadTileImageFromUrlOptions&& options = {}) const;

private:
  CesiumAsync::Future<TileProviderAndTile>
  doLoad(RasterOverlayTile& tile, bool isThrottledLoad);

  /**
   * @brief Begins the process of loading of a tile.
   *
   * This method should be called at the beginning of the tile load process.
   *
   * @param isThrottledLoad True if the load was originally throttled.
   */
  void beginTileLoad(bool isThrottledLoad) noexcept;

  /**
   * @brief Finalizes loading of a tile.
   *
   * This method should be called at the end of the tile load process,
   * no matter whether the load succeeded or failed.
   *
   * @param isThrottledLoad True if the load was originally throttled.
   */
  void finalizeTileLoad(bool isThrottledLoad) noexcept;

private:
  CesiumUtility::IntrusivePointer<RasterOverlay> _pOwner;
  CesiumAsync::AsyncSystem _asyncSystem;
  std::shared_ptr<CesiumAsync::IAssetAccessor> _pAssetAccessor;
  std::optional<CesiumUtility::Credit> _credit;
  std::shared_ptr<IPrepareRasterOverlayRendererResources>
      _pPrepareRendererResources;
  std::shared_ptr<spdlog::logger> _pLogger;
  CesiumGeospatial::Projection _projection;
  CesiumGeometry::Rectangle _coverageRectangle;
  CesiumUtility::IntrusivePointer<RasterOverlayTile> _pPlaceholder;
  int64_t _tileDataBytes;
  int32_t _totalTilesCurrentlyLoading;
  int32_t _throttledTilesCurrentlyLoading;
  CESIUM_TRACE_DECLARE_TRACK_SET(
      _loadingSlots,
      "Raster Overlay Tile Loading Slot")
};
} // namespace CesiumRasterOverlays
