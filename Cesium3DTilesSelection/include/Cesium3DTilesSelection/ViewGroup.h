#pragma once

#include <Cesium3DTilesSelection/TileSelectionState.h>

#include <unordered_map>

namespace Cesium3DTilesSelection {

class Tile;

/**
 * @brief A group of views that select tiles together.
 */
class CESIUM3DTILESSELECTION_API ViewGroup final {
public:
  /**
   * @brief Returns the {@link TileSelectionState} of this tile.
   *
   * This function is not supposed to be called by clients.
   *
   * @return The last selection state
   */
  TileSelectionState getPreviousSelectionState(const Tile& tile) const noexcept;

  /**
   * @brief Returns the current {@link TileSelectionState} of this tile.
   *
   * This function is not supposed to be called by clients.
   *
   * @return The curren t selection state
   */
  TileSelectionState getCurrentSelectionState(const Tile& tile) const noexcept;

  /**
   * @brief Set the {@link TileSelectionState} of this tile.
   *
   * This function is not supposed to be called by clients.
   *
   * @param newState The new state
   */
  void setCurrentSelectionState(
      const Tile& tile,
      const TileSelectionState& newState) noexcept;

  void kick(const Tile& tile) noexcept;

  /**
   * @brief Starts the next frame by making the current tile selection state the
   * previous one, and clearing the current one.
   */
  void startNextFrame();

private:
  std::unordered_map<const Tile*, TileSelectionState> _previousSelectionStates;
  std::unordered_map<const Tile*, TileSelectionState> _currentSelectionStates;
};

} // namespace Cesium3DTilesSelection
