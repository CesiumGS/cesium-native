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
  virtual bool shouldExclude(const Tile& tile) const noexcept = 0;
};

} // namespace Cesium3DTilesSelection
