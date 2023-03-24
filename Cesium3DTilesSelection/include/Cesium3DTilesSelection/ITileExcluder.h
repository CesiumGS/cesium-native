#pragma once

namespace Cesium3DTilesSelection {

class Tile;

/**
 * @brief An interface that allows tiles to be excluded from loading and
 * rendering when provided in {@link TilesetOptions::excluders}.
 */
class ITileExcluder {
public:
  virtual ~ITileExcluder() = default;

  /**
   * @brief Indicates the start of a new frame, initiated with a call to {@link Tileset::updateView}.
   */
  virtual void startNewFrame() noexcept {}

  /**
   * @brief Determines whether a given tile should be excluded.
   *
   * @param tile The tile to test
   * @return true if this tile and all of its descendants in the bounding volume
   * hierarchy should be excluded from loading and rendering.
   * @return false if this tile should be included.
   */
  virtual bool shouldExclude(const Tile& tile) const noexcept = 0;
};

} // namespace Cesium3DTilesSelection
