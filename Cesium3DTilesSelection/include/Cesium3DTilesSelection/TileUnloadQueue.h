#pragma once

#include <Cesium3DTilesSelection/Tile.h>

namespace Cesium3DTilesSelection {

/**
 * @brief LRU list of tiles eligible for content eviction.
 *
 * Head is least-recently-used, tail is most-recently-used.
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
   * @brief Adds `tile` to the tail (most-recently-used end). No-op if already
   * tracked.
   */
  void markEligible(Tile& tile) noexcept {
    if (!_queue.contains(tile)) {
      _queue.insertAtTail(tile);
    }
  }

  /** @brief Removes `tile` from the queue. No-op if not present. */
  void markIneligible(Tile& tile) noexcept { _queue.remove(tile); }

  /** @brief Returns true if `tile` is currently in the queue. */
  bool contains(const Tile& tile) const noexcept {
    return _queue.contains(tile);
  }

  /** @brief Returns the least-recently-used tile, or nullptr. */
  Tile* head() noexcept { return _queue.head(); }

  /** @brief Returns the tile following `tile` in LRU order. */
  Tile* next(Tile& tile) noexcept { return _queue.next(tile); }

private:
  Tile::UnusedLinkedList _queue;
};

} // namespace Cesium3DTilesSelection
