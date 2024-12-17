#pragma once

#include <memory>
#include <string>

namespace Cesium3DTilesSelection {

class Tile;
class Tileset;

/**
 * @brief Helps debug the tile selection algorithm by recording the state of
 * tiles each frame to a SQLite database.
 */
class DebugTileStateDatabase {
public:
  /**
   * @brief Creates a new instance.
   *
   * @param databaseFilename The full path and filename of the output SQLite
   * database.
   */
  DebugTileStateDatabase(const std::string& databaseFilename);
  ~DebugTileStateDatabase() noexcept;

  /**
   * @brief Records the state of all tiles that are currently loaded by the
   * given tileset.
   *
   * @param frameNumber The current frame number.
   * @param tileset The tileset.
   */
  void recordAllTileStates(int32_t frameNumber, const Tileset& tileset);

  /**
   * @brief Records the state of a given tile.
   *
   * @param frameNumber The current frame number.
   * @param tile The tile.
   */
  void recordTileState(int32_t frameNumber, const Tile& tile);

private:
  struct Impl;
  std::unique_ptr<Impl> _pImpl;
};

} // namespace Cesium3DTilesSelection
