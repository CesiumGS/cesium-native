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
  size_t workerThreadTileLoadQueueLength = 0;
  size_t mainThreadTileLoadQueueLength = 0;

  uint32_t tilesVisited = 0;
  uint32_t culledTilesVisited = 0;
  uint32_t tilesCulled = 0;
  uint32_t tilesOccluded = 0;
  uint32_t tilesWaitingForOcclusionResults = 0;
  uint32_t tilesKicked = 0;
  uint32_t maxDepthVisited = 0;

  uint32_t tilesLoading = 0;
  uint32_t tilesLoaded = 0;
  uint32_t rastersLoading = 0;
  uint32_t rastersLoaded = 0;
  size_t requestsPending = 0;

  void resetStats() {
    workerThreadTileLoadQueueLength = 0;
    mainThreadTileLoadQueueLength = 0;

    tilesVisited = 0;
    culledTilesVisited = 0;
    tilesCulled = 0;
    tilesOccluded = 0;
    tilesWaitingForOcclusionResults = 0;
    tilesKicked = 0;
    maxDepthVisited = 0;

    tilesLoading = 0;
    tilesLoaded = 0;
    rastersLoading = 0;
    rastersLoaded = 0;
    requestsPending = 0;
  }
  //! @endcond

  int32_t frameNumber = 0;
};

} // namespace Cesium3DTilesSelection
