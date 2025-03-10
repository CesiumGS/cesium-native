#pragma once

#include <CesiumUtility/IntrusivePointer.h>

namespace Cesium3DTilesSelection {

class Tile;
class TilesetContentManager;

/**
 * @brief An interface to something that requests loading of specific tiles from
 * a 3D Tiles {@link Tileset}.
 *
 * When multiple requesters are registered, each is given a fair chance to load
 * tiles in proportion with its {@link TileLoadRequester::getWeight}.
 *
 * Methods of this class may only be called from the main thread.
 *
 * @see TilesetViewGroup
 * @see TilesetHeightRequest
 */
class TileLoadRequester {
public:
  /**
   * @brief Gets the weight of this requester relative to others.
   *
   * Most requesters should return a weight of 1.0. When all requesters have the
   * same weight, they will all have an equal opportunity to load tiles. If one
   * requester's weight is 2.0 and the rest are 1.0, that requester will have
   * twice as many opportunities to load tiles as the others.
   *
   * A very high weight will prevent all other requesters from loading tiles
   * until this requester has none lef to load. A very low weight (but above
   * 0.0!) will allow all other requesters to finish loading tiles before this
   * one starts.
   *
   * @return The weight of this requester, which must be greater than 0.0.
   */
  virtual double getWeight() const = 0;

  /**
   * @brief Determines if this requester has any more tiles that need to be
   * loaded in a worker thread. To determine if a particular {@link Tile} needs
   * to be loaded in a worker thread, call
   * {@link Tile::needsWorkerThreadLoading}.
   *
   * @return true if this requester has further tiles that need loading by a
   * worker thread; otherwise, false.
   */
  virtual bool hasMoreTilesToLoadInWorkerThread() const = 0;

  /**
   * @brief Gets the next {@link Tile} that this requester would like loaded in
   * a worker thread.
   *
   * When {@link hasMoreTilesToLoadInWorkerThread} returns false, this method
   * can and should return `nullptr`. However, when that method returns true,
   * this method _must_ return a valid `Tile` pointer.
   *
   * @return The next tile to load in a worker thread.
   */
  virtual Tile* getNextTileToLoadInWorkerThread() = 0;

  /**
   * @brief Determines if this requester has any more tiles that need to be
   * loaded in the main thread. To determine if a particular {@link Tile} needs
   * to be loaded in the main thread, call
   * {@link Tile::needsMainThreadLoading}.
   *
   * @return true if this requester has further tiles that need loading by the
   * main thread; otherwise, false.
   */
  virtual bool hasMoreTilesToLoadInMainThread() const = 0;

  /**
   * @brief Gets the next {@link Tile} that this requester would like loaded in
   * the main thread.
   *
   * When {@link hasMoreTilesToLoadInMainThread} returns false, this method
   * can and should return `nullptr`. However, when that method returns true,
   * this method _must_ return a valid `Tile` pointer.
   *
   * @return The next tile to load in the main thread.
   */
  virtual Tile* getNextTileToLoadInMainThread() = 0;

  /**
   * @brief Unregister this requester with the {link Tileset} with which it is
   * currently registered. Once unregistered, it will not influence tile loads
   * until registered again.
   *
   * If this instance is not currently registered, this method does nothing.
   */
  void unregister() noexcept;

protected:
  TileLoadRequester() noexcept;
  TileLoadRequester(const TileLoadRequester& rhs) noexcept;
  TileLoadRequester(TileLoadRequester&& rhs) noexcept;
  virtual ~TileLoadRequester() noexcept;

private:
  CesiumUtility::IntrusivePointer<TilesetContentManager>
      _pTilesetContentManager;

  friend class Tileset;
};

} // namespace Cesium3DTilesSelection
