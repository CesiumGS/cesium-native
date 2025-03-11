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
 */
class CESIUM3DTILESSELECTION_API TilesetViewGroup final
    : public TileLoadRequester {
public:
  /**
   * @brief Constructs a new instance.
   */
  TilesetViewGroup() noexcept;

  /**
   * @brief Constructs a new instance as a copy of an existing one.
   *
   * The new instance will be equivalent to the existing one, but it will not be
   * registered with the {@link Cesium3DTilesSelection::Tileset}.
   *
   * @param rhs The view group to copy.
   */
  TilesetViewGroup(const TilesetViewGroup& rhs) noexcept;

  /**
   * @brief Moves an existing view group into a new one.
   *
   * The new instance will be registered with the
   * {@link Cesium3DTilesSelection::Tileset}, and the old one will be
   * unregistered.
   *
   * @param rhs The view group to move from.
   */
  TilesetViewGroup(TilesetViewGroup&& rhs) noexcept;
  virtual ~TilesetViewGroup() noexcept;

  /**
   * @brief Returns the previous {@link TileSelectionState} of this tile last
   * time this view group was updated.
   *
   * @param tile The tile for which to get the selection state.
   * @return The previous selection state
   */
  TileSelectionState getPreviousSelectionState(const Tile& tile) const noexcept;

  /**
   * @brief Returns the current {@link TileSelectionState} of this tile during
   * the current update of this view group.
   *
   * @param tile The tile for which to get the selection state.
   * @return The current selection state
   */
  TileSelectionState getCurrentSelectionState(const Tile& tile) const noexcept;

  /**
   * @brief Sets the {@link TileSelectionState} of this tile.
   *
   * @param tile The tile for which to set the selection state.
   * @param newState The new state
   */
  void setCurrentSelectionState(
      const Tile& tile,
      const TileSelectionState& newState) noexcept;

  /**
   * @brief Marks a tile as "kicked".
   *
   * @param tile The tile to "kick".
   */
  void kick(const Tile& tile) noexcept;

  struct LoadQueueState {
  private:
    size_t mainThreadQueueSize;
    size_t workerThreadQueueSize;
    friend class TilesetViewGroup;
  };

  void addToLoadQueue(const TileLoadTask& task);
  LoadQueueState saveLoadQueueState();
  size_t restoreLoadQueueState(const LoadQueueState& state);

  size_t getWorkerThreadLoadQueueLength() const;
  size_t getMainThreadLoadQueueLength() const;

  /**
   * @brief Starts a new frame, clearing the set of tiles to be loaded so that a
   * new set can be selected.
   */
  void startNewFrame();

  /**
   * @brief Finishes the current frame by making the current tile selection
   * state the previous one and releasing references to tiles in the old
   * previous one.
   */
  void finishFrame();

  /** @inheritdoc */
  double getWeight() const override;

  /**
   * @brief Sets the weight of this view group relative to other tile
   * requesters.
   *
   * See {@link getWeight} for an explanation of the meaning of the weight.
   *
   * @param weight The new weight for this view group.
   */
  void setWeight(double weight) noexcept;

  /** @inheritdoc */
  bool hasMoreTilesToLoadInWorkerThread() const override;
  /** @inheritdoc */
  Tile* getNextTileToLoadInWorkerThread() override;

  /** @inheritdoc */
  bool hasMoreTilesToLoadInMainThread() const override;
  /** @inheritdoc */
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
};

} // namespace Cesium3DTilesSelection
