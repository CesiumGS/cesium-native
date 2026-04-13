#pragma once

#include <Cesium3DTilesSelection/Tile.h>

namespace Cesium3DTilesSelection {

/**
 * @brief Tracks tiles whose content may be evicted when memory is low.
 *
 * Tiles at the head are least-recently-used; tiles at the tail are
 * most-recently-used.
 *
 * Manages a LRU linked-list of non-owning Tile references.
 */
class TileUnloadQueue {
public:
  TileUnloadQueue() noexcept = default;
  ~TileUnloadQueue() noexcept = default;

  TileUnloadQueue(const TileUnloadQueue&) = delete;
  TileUnloadQueue& operator=(const TileUnloadQueue&) = delete;
  /** @brief Move constructor. */
  TileUnloadQueue(TileUnloadQueue&&) noexcept = default;
  /** @brief Move assignment. */
  TileUnloadQueue& operator=(TileUnloadQueue&&) noexcept = default;

  /**
   * @brief Adds `tile` to the tail (most-recently-used end) of the queue.
   * No-op if `tile` is already tracked.
   */
  void markEligible(Tile& tile) noexcept {
    if (!_queue.contains(tile)) {
      _queue.insertAtTail(tile);
    }
  }

  /**
   * @brief Removes `tile` from the queue. No-op if not present.
   */
  void markIneligible(Tile& tile) noexcept { _queue.remove(tile); }

  /** @brief Returns true when `tile` is currently in the candidate list. */
  bool contains(const Tile& tile) const noexcept {
    return _queue.contains(tile);
  }

  /**
   * @brief Removes `tile` from the list.
   */
  void remove(Tile& tile) noexcept { _queue.remove(tile); }

  /** @brief Returns the least-recently-used tile, or nullptr. */
  Tile* head() noexcept { return _queue.head(); }

  /** @brief Returns the tile following `tile` in LRU order. */
  Tile* next(Tile& tile) noexcept { return _queue.next(tile); }

private:
  Tile::UnusedLinkedList _queue;
};

} // namespace Cesium3DTilesSelection
