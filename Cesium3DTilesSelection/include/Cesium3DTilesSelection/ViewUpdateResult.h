#pragma once

#include "Library.h"

#include <unordered_set>
#include <vector>

namespace Cesium3DTilesSelection {
class Tile;

/**
 * @brief Reports the results of {@link Tileset::updateView}.
 *
 * Users of a {@link Tileset} will call {@link Tileset::updateView} and receive
 * this structure so that they can update the state of their rendering system
 * accordingly. The tileset will internally keep track the current state of the
 * tiles as their {@link Tile::getLastSelectionState} throughout the rendering
 * process, and use this structure to provide information about the state
 * changes of tiles to clients.
 */
class CESIUM3DTILESSELECTION_API ViewUpdateResult final {
public:
  /**
   * @brief The tiles that were selected by the tileset traversal this frame.
   * These tiles should be rendered by the client.
   */
  std::vector<Tile*> tilesSelectedThisFrame;

  /**
   * @brief The tiles that were no longer selected by the tileset traversal
   * this frame. These tiles may still need to be rendered during a fade out
   * period if {@link TilesetOptions::enableLodTransitionPeriod} is true.
   *
   * If a tile is in this list, there is another LOD that is ready to take its
   * place. So clients may choose to disable non-rendering behavior such as
   * physics colliders for the tiles in this list, since they may conflict with
   * the replacement tiles. If LOD transition periods are not enabled, tiles
   * from this list no longer need to be rendered.
   */
  std::vector<Tile*> tilesNoLongerSelectedThisFrame;

  /**
   * @brief These tiles were not selected this frame, but should continue being
   * rendered by the client while fading out.
   *
   * This list is only relevant when
   * {@link TilesetOptions::enableLodTransitionPeriod} is true.
   */
  std::unordered_set<Tile*> tilesFadingOut;

  /**
   * @brief These tiles are ready to be completely hidden by the client. Tiles
   * on this list have already completed any necessary fading.
   *
   * When {@link TilesetOptions::enableLodTransitionPeriod} is false, this list
   * should be the same as {@link tilesNoLongerSelectedThisFrame}.
   */
  std::vector<Tile*> tilesToHideThisFrame;

  //! @cond Doxygen_Suppress
  uint32_t tilesLoadingLowPriority = 0;
  uint32_t tilesLoadingMediumPriority = 0;
  uint32_t tilesLoadingHighPriority = 0;

  uint32_t tilesVisited = 0;
  uint32_t culledTilesVisited = 0;
  uint32_t tilesCulled = 0;
  uint32_t tilesOccluded = 0;
  uint32_t tilesWaitingForOcclusionResults = 0;
  uint32_t maxDepthVisited = 0;
  //! @endcond
};

} // namespace Cesium3DTilesSelection
