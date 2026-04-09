#pragma once

#include <Cesium3DTilesSelection/Tile.h>

#include <memory>

namespace Cesium3DTilesSelection {

/**
 * @brief Owns the root of the tile tree.
 *
 * All Tile instances are either held here (as the root) or owned
 * by their parent via `Tile::_children`. This is the single point of
 * tree ownership — it is non-copyable and non-shared.
 *
 * Access must occur on the main thread. [main-thread-only]
 */
class TileHierarchy {
public:
  TileHierarchy() noexcept = default;
  ~TileHierarchy() noexcept = default;

  TileHierarchy(const TileHierarchy&) = delete;
  TileHierarchy& operator=(const TileHierarchy&) = delete;
  TileHierarchy(TileHierarchy&&) noexcept = default;
  TileHierarchy& operator=(TileHierarchy&&) noexcept = default;

  /** @brief Returns the root tile, or nullptr when not yet loaded.
   * [main-thread] */
  const Tile* getRoot() const noexcept { return _pRoot.get(); }
  /** @brief Returns the root tile, or nullptr when not yet loaded.
   * [main-thread] */
  Tile* getRoot() noexcept { return _pRoot.get(); }

  /** @brief Transfers ownership of a new root tile into this hierarchy.
   * [main-thread] */
  void setRoot(std::unique_ptr<Tile>&& pRoot) noexcept {
    _pRoot = std::move(pRoot);
  }

private:
  std::unique_ptr<Tile> _pRoot;
};

} // namespace Cesium3DTilesSelection
