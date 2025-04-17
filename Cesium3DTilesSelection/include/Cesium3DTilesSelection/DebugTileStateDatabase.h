#pragma once

#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileSelectionState.h>

#include <memory>
#include <string>
#include <unordered_map>

namespace Cesium3DTilesSelection {

class Tileset;
class TilesetViewGroup;

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
   * The state is obtained from the view group's
   * {@link TilesetViewGroup::getTraversalState} by calling
   * {@link CesiumUtility::TreeTraversalState::slowlyGetPreviousStates}.
   *
   * @param frameNumber The current frame number.
   * @param tileset The tileset.
   * @param viewGroup The view group.
   */
  void recordAllTileStates(
      int32_t frameNumber,
      const Tileset& tileset,
      const TilesetViewGroup& viewGroup);

  /**
   * @brief Records the state of a given tile.
   *
   * The state is obtained from the view group's
   * {@link TilesetViewGroup::getTraversalState} by calling
   * {@link CesiumUtility::TreeTraversalState::slowlyGetPreviousStates}.
   *
   * @param frameNumber The current frame number.
   * @param viewGroup The view group.
   * @param tile The tile.
   */
  void recordTileState(
      int32_t frameNumber,
      const TilesetViewGroup& viewGroup,
      const Tile& tile);

  /**
   * @brief Records the state of a given tile.
   *
   * The state is obtained from the provided map.
   *
   * @param frameNumber The current frame number.
   * @param tile The tile.
   * @param states The lookup table for tile states.
   */
  void recordTileState(
      int32_t frameNumber,
      const Tile& tile,
      const std::unordered_map<Tile::Pointer, TileSelectionState>& states);

private:
  struct Impl;
  std::unique_ptr<Impl> _pImpl;
};

} // namespace Cesium3DTilesSelection
