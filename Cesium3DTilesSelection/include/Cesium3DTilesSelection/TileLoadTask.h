#pragma once

namespace Cesium3DTilesSelection {

class Tile;

/**
 * @brief Enumerates broad categories of priority for loading a
 * {@link Cesium3DTilesSelection::Tile} for a
 * {@link Cesium3DTilesSelection::TilesetViewGroup}.
 */
enum class TileLoadPriorityGroup {
  /**
   * @brief Low priority tiles that aren't needed right now, but
   * are being preloaded for the future.
   */
  Preload = 0,

  /**
   * @brief Medium priority tiles that are needed to render the current view
   * at the appropriate level-of-detail.
   */
  Normal = 1,

  /**
   * @brief High priority tiles whose absence is causing extra detail to be
   * rendered in the scene, potentially creating a performance problem and
   * aliasing artifacts.
   */
  Urgent = 2
};

/**
 * @brief Represents the need to load a particular
 * {@link Cesium3DTilesSelection::Tile} with a particular priority.
 */
struct TileLoadTask {
  /**
   * @brief The tile to be loaded.
   */
  Tile* pTile;

  /**
   * @brief The priority group (low / medium / high) in which to load this
   * tile.
   *
   * All tiles in a higher priority group are given a chance to load before
   * any tiles in a lower priority group.
   */
  TileLoadPriorityGroup group;

  /**
   * @brief The priority of this tile within its priority group.
   *
   * Tiles with a _lower_ value for this property load sooner!
   */
  double priority;

  /**
   * @brief Determines whether this task has a lower priority (higher numerical
   * value) than another one.
   *
   * If used with `std::sort`, this operator will put the lowest priority tasks
   * at the front of the container, and the highest priority tasks at the back.
   *
   * @param rhs The other task to compare.
   * @returns true if this task has the lower priority, or false if `rhs` has
   * the lower priority.
   */
  bool operator<(const TileLoadTask& rhs) const noexcept {
    if (this->group == rhs.group)
      return this->priority > rhs.priority;
    else
      return this->group < rhs.group;
  }
};

} // namespace Cesium3DTilesSelection
