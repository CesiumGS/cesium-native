#pragma once

#include <Cesium3DTilesSelection/Library.h>

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

  /**
   * @brief The number of tiles in the worker thread load queue.
   */
  int32_t workerThreadTileLoadQueueLength = 0;

  /**
   * @brief The number of tiles in the main thread load queue.
   */
  int32_t mainThreadTileLoadQueueLength = 0;

  /**
   * @brief The number of tiles visited during tileset traversal this frame.
   */
  uint32_t tilesVisited = 0;
  /**
   * @brief The number of culled tiles visited during tileset traversal this
   * frame.
   */
  uint32_t culledTilesVisited = 0;
  /**
   * @brief The number of tiles that were skipped because they were culled
   * during tileset traversal this frame.
   */
  uint32_t tilesCulled = 0;
  /**
   * @brief The number of tiles occluded this frame.
   */
  uint32_t tilesOccluded = 0;
  /**
   * @brief The number of tiles still waiting to obtain a \ref
   * TileOcclusionState this frame.
   */
  uint32_t tilesWaitingForOcclusionResults = 0;
  /**
   * @brief The number of tiles kicked from the render list this frame.
   */
  uint32_t tilesKicked = 0;
  /**
   * @brief The maximum depth of the tile tree visited this frame.
   */
  uint32_t maxDepthVisited = 0;

  /**
   * @brief The frame number. This is incremented every time \ref
   * Tileset::updateView is called.
   */
  int32_t frameNumber = 0;
};

} // namespace Cesium3DTilesSelection
