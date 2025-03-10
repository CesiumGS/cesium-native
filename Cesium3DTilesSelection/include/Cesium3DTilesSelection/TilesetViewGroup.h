#pragma once

#include <Cesium3DTilesSelection/TileLoadRequester.h>
#include <Cesium3DTilesSelection/TileLoadTask.h>
#include <Cesium3DTilesSelection/TileSelectionState.h>
#include <CesiumUtility/IntrusivePointer.h>

#include <unordered_map>
#include <vector>

namespace Cesium3DTilesSelection {

class Tile;
class Tileset;
class TilesetContentManager;

/**
 * @brief A group of views that select tiles from a particular {@link Tileset}
 * together.
 *
 * Create an instance of this class by calling {@link Tileset::createViewGroup}.
 */
class CESIUM3DTILESSELECTION_API TilesetViewGroup final
    : public TileLoadRequester {
public:
  TilesetViewGroup() noexcept;
  TilesetViewGroup(const TilesetViewGroup& rhs) noexcept;
  TilesetViewGroup(TilesetViewGroup&& rhs) noexcept;
  virtual ~TilesetViewGroup() noexcept;

  /**
   * @brief Returns the previous {@link TileSelectionState} of this tile last
   * time this view group was updated.
   *
   * This function is not supposed to be called by clients.
   *
   * @return The previous selection state
   */
  TileSelectionState getPreviousSelectionState(const Tile& tile) const noexcept;

  /**
   * @brief Returns the current {@link TileSelectionState} of this tile during
   * the current update of this view group.
   *
   * This function is not supposed to be called by clients.
   *
   * @return The current selection state
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
   * @brief Finishes the current frame by making the current tile selection
   * state the previous one and releasing references to tiles in the old
   * previous one.
   */
  void finishFrame();

  double getWeight() const override;
  void setWeight(double weight) noexcept;

  bool hasMoreTilesToLoadInWorkerThread() const override;
  Tile* getNextTileToLoadInWorkerThread() override;

  bool hasMoreTilesToLoadInMainThread() const override;
  Tile* getNextTileToLoadInMainThread() override;

private:
  double _weight;
  std::unordered_map<
      CesiumUtility::IntrusivePointer<const Tile>,
      TileSelectionState>
      _previousSelectionStates;
  std::unordered_map<
      CesiumUtility::IntrusivePointer<const Tile>,
      TileSelectionState>
      _currentSelectionStates;

  std::vector<TileLoadTask> _mainThreadLoadQueue;
  std::vector<TileLoadTask> _workerThreadLoadQueue;

  // So that the Tileset can create instances of this class.
  friend class Tileset;
};

} // namespace Cesium3DTilesSelection
