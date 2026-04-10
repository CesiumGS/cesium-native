#pragma once

#include <Cesium3DTilesSelection/Library.h>
#include <Cesium3DTilesSelection/ViewUpdateResult.h>

#include <vector>

namespace Cesium3DTilesSelection {

class Tile;
class TilesetFrameState;
class TilesetExternals;
struct TilesetOptions;
class TileOcclusionRendererProxy;

/**
 * @brief Bundles the immutable per-frame inputs to the tile selection
 * algorithm. Holds only references — no ownership.
 */
struct TileSelectionContext {
  /** @brief The tileset configuration options. */
  const TilesetOptions& options;
  /** @brief The external interfaces (asset accessor, task processor, etc.). */
  const TilesetExternals& externals;
};

/**
 * @brief Runs the tile selection / LOD traversal algorithm.
 *
 * This is a free function with no dependency on `Tileset`, enabling
 * standalone unit testing against an in-memory tile tree.
 *
 * Side effects are limited to explicit reference parameters:
 * - `frameState.viewGroup` receives traversal state updates and load queue
 *   entries.
 * - `scratchDistances` and `scratchOcclusionProxies` are reused across
 *   frames to avoid per-frame heap allocation; their contents on entry are
 *   unspecified.
 *
 * @param ctx      Configuration and external dependencies.
 * @param frameState Per-frame view parameters and the view group to update.
 * @param rootTile Root of the tile hierarchy to traverse.
 * @param scratchDistances Caller-owned scratch buffer.
 * @param scratchOcclusionProxies Caller-owned scratch buffer.
 * @return The frame's render result and statistics.
 */
CESIUM3DTILESSELECTION_API ViewUpdateResult selectTiles(
    const TileSelectionContext& ctx,
    const TilesetFrameState& frameState,
    Tile& rootTile,
    std::vector<double>& scratchDistances,
    std::vector<const TileOcclusionRendererProxy*>& scratchOcclusionProxies);

} // namespace Cesium3DTilesSelection
