#pragma once

#include <CesiumUtility/IntrusivePointer.h>

#include <string>

namespace CesiumAsync {
class AsyncSystem;
}

namespace Cesium3DTilesSelection {

class Tile;
class TilesetContentManager;
struct TilesetOptions;

/**
 * @brief The base class for something that requests loading of specific tiles
 * from a 3D Tiles {@link Tileset}.
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
   * until this requester has none left to load. A very low weight (but above
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
   * The returned tile _must_ have a reference count greater than zero.
   * Otherwise, the Tile would be immediately eligible for unloading, so it
   * doesn't make sense to load it. In debug builds, this is enforced with an
   * assertion. In release builds, unreferenced tiles are silently ignored.
   *
   * @return The next tile to load in a worker thread.
   */
  virtual const Tile* getNextTileToLoadInWorkerThread() = 0;

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
   * The returned tile _must_ have a reference count greater than zero.
   * Otherwise, the Tile would be immediately eligible for unloading, so it
   * doesn't make sense to load it. In debug builds, this is enforced with an
   * assertion. In release builds, unreferenced tiles are silently ignored.
   *
   * @return The next tile to load in the main thread.
   */
  virtual const Tile* getNextTileToLoadInMainThread() = 0;

  /**
   * @brief Unregister this requester with the {link Tileset} with which it is
   * currently registered. Once unregistered, it will not influence tile loads
   * until registered again.
   *
   * If this instance is not currently registered, this method does nothing.
   *
   * To register an instance with a `Tileset`, call
   * {@link Tileset::registerLoadRequester} on the tileset.
   */
  void unregister() noexcept;

  /**
   * @brief Alternate interface for certain requests. This is called by the {@link Tileset}
   * in every call to {@link Tileset::updateView}.
   *
   * @param asyncSystem The async system used to do work in threads.
   * @param contentManager The content manager.
   * @param options Options associated with the tileset.
   */
  virtual bool doRequest(
      const CesiumAsync::AsyncSystem& asyncSystem,
      TilesetContentManager& contentManager,
      const TilesetOptions& options);

  /**
   * @brief Called when pending requests will not be fulfilled.
   */
  virtual void fail(const std::string& message);

  /**
   * @brief Destroys this instance.
   */
  virtual ~TileLoadRequester() noexcept;

protected:
  /**
   * @brief Constructs a new instance.
   */
  TileLoadRequester() noexcept;

  /**
   * @brief Constructs a new instance as a copy of an existing one.
   *
   * The copy will not be registered with any
   * {@link Cesium3DTilesSelection::Tileset}, even if the existing instance was.
   *
   * @param rhs The existing instance to copy.
   */
  TileLoadRequester(const TileLoadRequester& rhs) noexcept;

  /**
   * @brief Moves an existing instance into a new one.
   *
   * The newly-constructed instance will be registered with the same
   * {@link Cesium3DTilesSelection::Tileset} as the `rhs`. After the
   * constructor returns, the `rhs` will no longer be registered.
   *
   * @param rhs The existing instance to move into this one.
   */
  TileLoadRequester(TileLoadRequester&& rhs) noexcept;

private:
  CesiumUtility::IntrusivePointer<TilesetContentManager>
      _pTilesetContentManager;

  friend class Tileset;
};

} // namespace Cesium3DTilesSelection
