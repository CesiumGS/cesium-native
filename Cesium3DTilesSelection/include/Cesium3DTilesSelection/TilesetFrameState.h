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
   * @brief Called once per visited tile to advance its content state (loading,
   * finalization, child creation). Set by Tileset::updateViewGroup; may be
   * null when testing selectTiles directly.
   */
  std::function<void(Tile&)> tileStateUpdater;
};

} // namespace Cesium3DTilesSelection
