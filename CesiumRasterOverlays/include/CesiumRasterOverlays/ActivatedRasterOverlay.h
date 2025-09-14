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

class CESIUMRASTEROVERLAYS_API ActivatedRasterOverlay
    : public CesiumUtility::ReferenceCountedNonThreadSafe<
          ActivatedRasterOverlay> {
public:
  ActivatedRasterOverlay(
      const RasterOverlayExternals& externals,
      const CesiumUtility::IntrusivePointer<const RasterOverlay>& pOverlay,
      const CesiumGeospatial::Ellipsoid& ellipsoid);
  ~ActivatedRasterOverlay();

  CesiumAsync::SharedFuture<void>& getReadyEvent();

  const CesiumRasterOverlays::RasterOverlay* getOverlay() const noexcept;

  const CesiumRasterOverlays::RasterOverlayTileProvider*
  getTileProvider() const noexcept;

  CesiumRasterOverlays::RasterOverlayTileProvider* getTileProvider() noexcept;

  void setTileProvider(
      const CesiumUtility::IntrusivePointer<RasterOverlayTileProvider>&
          pTileProvider);

  const CesiumRasterOverlays::RasterOverlayTileProvider*
  getPlaceholderTileProvider() const noexcept;

  CesiumRasterOverlays::RasterOverlayTileProvider*
  getPlaceholderTileProvider() noexcept;

  const CesiumRasterOverlays::RasterOverlayTile*
  getPlaceholderTile() const noexcept;

  CesiumRasterOverlays::RasterOverlayTile* getPlaceholderTile() noexcept;

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
   * in a tile with a reference count greater than 0 will result in undefined
   * behavior.
   *
   * @param pTile The tile, which must have no oustanding references.
   */
  void removeTile(RasterOverlayTile* pTile) noexcept;

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
