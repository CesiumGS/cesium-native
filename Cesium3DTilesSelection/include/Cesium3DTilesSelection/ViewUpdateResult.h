#pragma once

#include "Library.h"

#include <cstdint>
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
   *
   * Tiles in this list may be fading in if
   * {@link TilesetOptions::enableLodTransitionPeriod} is true.
   */
  std::vector<Tile*> tilesToRenderThisFrame;

  /**
   * @brief Tiles on this list are no longer selected for rendering.
   *
   * If {@link TilesetOptions::enableLodTransitionPeriod} is true they may be
   * fading out. If a tile's {TileRenderContent::lodTransitionPercentage} is 0
   * or lod transitions are disabled, the tile should be hidden right away.
   */
  std::unordered_set<Tile*> tilesFadingOut;

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
