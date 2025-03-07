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

  virtual double getWeight() const = 0;

  virtual bool hasMoreTilesToLoadInWorkerThread() const = 0;
  virtual Tile* getNextTileToLoadInWorkerThread() = 0;

  virtual bool hasMoreTilesToLoadInMainThread() const = 0;
  virtual Tile* getNextTileToLoadInMainThread() = 0;
};

} // namespace Cesium3DTilesSelection
