#pragma once

#include <Cesium3DTilesSelection/ViewState.h>

#include <cstdint>
#include <vector>

namespace Cesium3DTilesSelection {

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
};

} // namespace Cesium3DTilesSelection
