#pragma once

#include <Cesium3DTilesSelection/Tile.h>

namespace Cesium3DTilesSelection {

/**
 * @brief Tracks tiles whose content may be evicted when memory is low.
 *
 * Tiles at the head are least-recently-used; tiles at the tail are
 * most-recently-used. All methods must be called from the main thread.
 * [main-thread-only]
 *
 * This type makes the ownership intent explicit: it manages a LRU linked-list
 * of non-owning Tile references. A tile's presence here does NOT imply
 * ownership — the tile hierarchy (TileHierarchy) is the sole owner.
 */
class TileUnloadQueue {
public:
  TileUnloadQueue() noexcept = default;
  ~TileUnloadQueue() noexcept = default;

  TileUnloadQueue(const TileUnloadQueue&) = delete;
  TileUnloadQueue& operator=(const TileUnloadQueue&) = delete;
  TileUnloadQueue(TileUnloadQueue&&) noexcept = default;
  TileUnloadQueue& operator=(TileUnloadQueue&&) noexcept = default;

  /**
   * @brief Adds `tile` to the tail (most-recently-used end) of the queue.
   * No-op if `tile` is already tracked. [main-thread]
   */
  void markEligible(Tile& tile) noexcept {
    if (!_queue.contains(tile)) {
      _queue.insertAtTail(tile);
    }
  }

  /**
   * @brief Removes `tile` from the queue. No-op if not present. [main-thread]
   */
  void markIneligible(Tile& tile) noexcept { _queue.remove(tile); }

  /** @brief Returns true when `tile` is currently in the candidate list.
   * [main-thread] */
  bool contains(const Tile& tile) const noexcept {
    return _queue.contains(tile);
  }

  /**
   * @brief Directly removes `tile` from the list.
   * Prefer markIneligible; this exists for sites that call remove()
   * unconditionally. [main-thread]
   */
  void remove(Tile& tile) noexcept { _queue.remove(tile); }

  /** @brief Returns the least-recently-used tile, or nullptr. [main-thread] */
  Tile* head() noexcept { return _queue.head(); }

  /** @brief Returns the tile following `tile` in LRU order. [main-thread] */
  Tile* next(Tile& tile) noexcept { return _queue.next(tile); }

private:
  Tile::UnusedLinkedList _queue;
};

} // namespace Cesium3DTilesSelection
