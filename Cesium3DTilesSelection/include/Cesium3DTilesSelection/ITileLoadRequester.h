#pragma once

namespace Cesium3DTilesSelection {

class Tile;

/**
 * @brief An interface to something that requests that specific tiles be loaded.
 *
 * @see TilesetViewGroup
 * @see TilesetHeightRequest
 */
class ITileLoadRequester {
public:
  virtual ~ITileLoadRequester() = default;

  virtual bool hasMoreTilesToLoad() const = 0;
  virtual Tile* getNextTileToLoad() = 0;
};

} // namespace Cesium3DTilesSelection
