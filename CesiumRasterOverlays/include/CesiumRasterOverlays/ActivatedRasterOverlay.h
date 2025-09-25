#pragma once

#include <CesiumAsync/Future.h>
#include <CesiumAsync/Promise.h>
#include <CesiumAsync/SharedFuture.h>
#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumRasterOverlays/Library.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/ReferenceCounted.h>

#include <glm/vec2.hpp>

namespace CesiumRasterOverlays {

class RasterOverlay;
class RasterOverlayExternals;
class RasterOverlayTile;
class RasterOverlayTileProvider;
struct TileProviderAndTile;

} // namespace CesiumRasterOverlays

namespace CesiumRasterOverlays {

/**
 * @brief Holds a tile and its corresponding tile provider. Used as the return
 * value of @ref ActivatedRasterOverlay::loadTile.
 */
struct TileProviderAndTile {
  /** @brief A \ref CesiumUtility::IntrusivePointer to the \ref
   * RasterOverlayTileProvider used for this tile. */
  CesiumUtility::IntrusivePointer<RasterOverlayTileProvider> pTileProvider;
  /** @brief A \ref CesiumUtility::IntrusivePointer to the \ref
   * RasterOverlayTile used for this tile. */
  CesiumUtility::IntrusivePointer<RasterOverlayTile> pTile;

  /**
   * @brief Constructs an instance.
   * @param pTileProvider_ The tile provider used for this tile.
   * @param pTile_ The tile.
   */
  TileProviderAndTile(
      const CesiumUtility::IntrusivePointer<RasterOverlayTileProvider>&
          pTileProvider_,
      const CesiumUtility::IntrusivePointer<RasterOverlayTile>&
          pTile_) noexcept;

  ~TileProviderAndTile() noexcept;

  /**
   * @brief Copy constructor.
   */
  TileProviderAndTile(const TileProviderAndTile&) noexcept;

  /**
   * @brief Copy assignment operator.
   */
  TileProviderAndTile& operator=(const TileProviderAndTile&) noexcept;

  /**
   * @brief Move constructor.
   */
  TileProviderAndTile(TileProviderAndTile&&) noexcept;

  /**
   * @brief Move assignment operator.
   */
  TileProviderAndTile& operator=(TileProviderAndTile&&) noexcept;
};

/**
 * @brief A @ref RasterOverlay that has been activated for use. While a
 * @ref RasterOverlayTileProvider can be used directly to load images,
 * this class provides additional functionality for managing @ref
 * RasterOverlayTile lifecycle and state.
 *
 * To create an instance of this class, call @ref RasterOverlay::activate.
 */
class CESIUMRASTEROVERLAYS_API ActivatedRasterOverlay
    : public CesiumUtility::ReferenceCountedNonThreadSafe<
          ActivatedRasterOverlay> {
public:
  /**
   * @brief Constructs a new instance.
   *
   * Consider calling @ref RasterOverlay::activate instead of using the
   * constructor directly.
   *
   * @param externals The external interfaces to use.
   * @param pOverlay The overlay to activate.
   * @param ellipsoid The @ref CesiumGeospatial::Ellipsoid.
   */
  ActivatedRasterOverlay(
      const RasterOverlayExternals& externals,
      const CesiumUtility::IntrusivePointer<const RasterOverlay>& pOverlay,
      const CesiumGeospatial::Ellipsoid& ellipsoid
          CESIUM_DEFAULT_ELLIPSOID) noexcept;

  /**
   * @brief Destroys the instance. Use @ref addReference and
   * @ref releaseReference instead of destroying this instance directly.
   */
  ~ActivatedRasterOverlay() noexcept;

  /**
   * @brief Gets a shared future that resolves when this instance is ready to
   * provide tiles.
   *
   * It is safe to call @ref getTile before this future resolves, but the
   * returned tile will be a placeholder.
   */
  CesiumAsync::SharedFuture<void>& getReadyEvent();

  /**
   * @brief Gets the @ref RasterOverlay that was activated to create this
   * instance.
   */
  const CesiumRasterOverlays::RasterOverlay& getOverlay() const noexcept;

  /**
   * @brief Gets the tile provider created for this activated overlay. This will
   * be `nullptr` before `getReadyEvent` resolves.
   */
  const CesiumRasterOverlays::RasterOverlayTileProvider*
  getTileProvider() const noexcept;

  /** @copydoc getTileProvider */
  CesiumRasterOverlays::RasterOverlayTileProvider* getTileProvider() noexcept;

  /**
   * @brief Sets the tile provider for this activated overlay.
   *
   * It is usually unnecessary to call this method because
   * @ref RasterOverlay::activate will call it automatically at the
   * appropriate time.
   *
   * Calling this method will resolve the @ref getReadyEvent.
   *
   * @param pTileProvider The tile provider. This must not be `nullptr`.
   */
  void setTileProvider(
      const CesiumUtility::IntrusivePointer<RasterOverlayTileProvider>&
          pTileProvider);

  /**
   * @brief Gets the placeholder tile provider.
   *
   * The placeholder may be used prior to @ref getReadyEvent resolving,
   * but it will create placeholder tiles only.
   */
  const CesiumRasterOverlays::RasterOverlayTileProvider*
  getPlaceholderTileProvider() const noexcept;

  /** @copydoc getPlaceholderTileProvider */
  CesiumRasterOverlays::RasterOverlayTileProvider*
  getPlaceholderTileProvider() noexcept;

  /**
   * @brief Gets the placeholder tile created by the @ref
   * getPlaceholderTileProvider.
   */
  const CesiumRasterOverlays::RasterOverlayTile*
  getPlaceholderTile() const noexcept;

  /** @copydoc getPlaceholderTile */
  CesiumRasterOverlays::RasterOverlayTile* getPlaceholderTile() noexcept;

  /**
   * @brief Returns a new @ref RasterOverlayTile with the given
   * specifications.
   *
   * The returned tile will not start loading immediately. To start loading,
   * call @ref ActivatedRasterOverlay::loadTile or
   * @ref ActivatedRasterOverlay::loadTileThrottled.
   *
   * @param rectangle The rectangle that the returned image must cover. It is
   * allowed to cover a slightly larger rectangle in order to maintain pixel
   * alignment. It may also cover a smaller rectangle when the overlay itself
   * does not cover the entire rectangle.
   * @param targetScreenPixels The maximum number of pixels on the screen that
   * this tile is meant to cover. The overlay image should be approximately this
   * many pixels divided by the @ref
   * RasterOverlayOptions::maximumScreenSpaceError in order to achieve the
   * desired level-of-detail, but it does not need to be exactly this size.
   * @return The tile.
   */
  CesiumUtility::IntrusivePointer<CesiumRasterOverlays::RasterOverlayTile>
  getTile(
      const CesiumGeometry::Rectangle& rectangle,
      const glm::dvec2& targetScreenPixels);

  /**
   * @brief Gets the number of bytes of tile data that are currently loaded.
   */
  int64_t getTileDataBytes() const noexcept;

  /**
   * @brief Returns the number of tiles that are currently loading.
   */
  uint32_t getNumberOfTilesLoading() const noexcept;

  /**
   * @brief Removes a no-longer-referenced tile from this provider's cache and
   * deletes it.
   *
   * This function is not supposed to be called by client. Calling this method
   * on a tile with a reference count greater than 0 will result in undefined
   * behavior.
   *
   * @param pTile The tile, which must have no oustanding references.
   */
  void removeTile(RasterOverlayTile* pTile) noexcept;

  /**
   * @brief Loads a tile immediately, without throttling requests.
   *
   * If the tile is not in the `RasterOverlayTile::LoadState::Unloaded` state,
   * this method returns without doing anything. Otherwise, it puts the tile
   * into the `RasterOverlayTile::LoadState::Loading` state and begins the
   * asynchronous process to load the tile. When the process completes, the tile
   * will be in the `RasterOverlayTile::LoadState::Loaded` or
   * `RasterOverlayTile::LoadState::Failed` state.
   *
   * Calling this method on many tiles at once can result in very slow
   * performance. Consider using @ref loadTileThrottled instead.
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
   * puts the tile into the `RasterOverlayTile::LoadState::Loading` state,
   * begins the asynchronous process to load the tile, and returns true. When
   * the process completes, the tile will be in the
   * `RasterOverlayTile::LoadState::Loaded` or
   * `RasterOverlayTile::LoadState::Failed` state.
   *
   * The number of allowable simultaneous tile requests is provided in the
   * @ref RasterOverlayOptions::maximumSimultaneousTileLoads property of
   * @ref RasterOverlay::getOptions.
   *
   * @param tile The tile to load.
   * @returns True if the tile load process is started or is already complete,
   * false if the load could not be started because too many loads are already
   * in progress.
   */
  bool loadTileThrottled(RasterOverlayTile& tile);

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

  CesiumUtility::IntrusivePointer<const CesiumRasterOverlays::RasterOverlay>
      _pOverlay;
  CesiumUtility::IntrusivePointer<
      CesiumRasterOverlays::RasterOverlayTileProvider>
      _pPlaceholderTileProvider;
  CesiumUtility::IntrusivePointer<CesiumRasterOverlays::RasterOverlayTile>
      _pPlaceholderTile;
  CesiumUtility::IntrusivePointer<
      CesiumRasterOverlays::RasterOverlayTileProvider>
      _pTileProvider;

  int64_t _tileDataBytes;
  int32_t _totalTilesCurrentlyLoading;
  int32_t _throttledTilesCurrentlyLoading;

  CesiumAsync::Promise<void> _readyPromise;
  CesiumAsync::SharedFuture<void> _readyEvent;
};

} // namespace CesiumRasterOverlays
