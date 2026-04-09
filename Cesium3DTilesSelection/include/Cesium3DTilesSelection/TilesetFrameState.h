#pragma once

#include <Cesium3DTilesSelection/ViewState.h>

#include <cstdint>
#include <functional>
#include <vector>

namespace Cesium3DTilesSelection {

class Tile;
class TilesetViewGroup;

/**
 * @brief Captures information about the current frame during a call to
 * {@link Tileset::updateViewGroup}.
 */
class TilesetFrameState {
public:
  /**
   * @brief The view group being updated.
   */
  TilesetViewGroup& viewGroup;

  /**
   * @brief The frustums in this view group that are viewing the tileset.
   */
  const std::vector<ViewState>& frustums;

  /**
   * @brief The computed fog density for each frustum.
   */
  std::vector<double> fogDensities;

  /**
   * @brief [main-thread] Callback invoked once per visited tile to advance its
   * content state machine (unloading, content-loaded finalization, latent
   * children creation).  Populated by Tileset::updateViewGroup before the
   * traversal begins.
   *
   * Keeping this as a callback rather than a direct TilesetContentManager
   * reference means the traversal functions (_visitTileIfNeeded, _visitTile,
   * etc.) have no compile-time dependency on TilesetContentManager, making
   * them independently testable.
   */
  std::function<void(Tile&)> tileStateUpdater;
};

} // namespace Cesium3DTilesSelection
