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
   * This function is not supposed to be called by clients.
   *
   * @param tile The tile for which to get the selection state.
   * @return The previous selection state
   */
  TileSelectionState getPreviousSelectionState(const Tile& tile) const noexcept;

  /**
   * @brief Returns the current {@link TileSelectionState} of this tile during
   * the current update of this view group.
   *
   * This function is not supposed to be called by clients.
   *
   * @param tile The tile for which to get the selection state.
   * @return The current selection state
   */
  TileSelectionState getCurrentSelectionState(const Tile& tile) const noexcept;

  /**
   * @brief Set the {@link TileSelectionState} of this tile.
   *
   * This function is not supposed to be called by clients.
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

  /**
   * @brief Finishes the current frame by making the current tile selection
   * state the previous one and releasing references to tiles in the old
   * previous one.
   */
  void finishFrame();

  /**
   * @brief Gets the weight of this view group relative to other tile
   * requesters.
   *
   * See {@link setWeight} for information about the meaning of this value.
   *
   * Most requesters should return a weight of 1.0. When all requesters have the
   * same weight, they will all have an equal opportunity to load tiles. If one
   * requester's weight is 2.0 and the rest are 1.0, that requester will have
   * twice as many opportunities to load tiles as the others.
   *
   * A very high weight will prevent all other requesters from loading tiles
   * until this requester has none left to load. A very low weight (but above
   * 0.0!) will allow all other requesters to finish loading tiles before this
   * one starts.
   *
   * @return The weight of this requester, which must be greater than 0.0.
   */
  double getWeight() const override;

  /**
   * @brief Sets the weight of this view group relative to other tile
   * requesters.
   *
   * Most requesters should return a weight of 1.0. When all requesters have the
   * same weight, they will all have an equal opportunity to load tiles. If one
   * requester's weight is 2.0 and the rest are 1.0, that requester will have
   * twice as many opportunities to load tiles as the others.
   *
   * A very high weight will prevent all other requesters from loading tiles
   * until this requester has none left to load. A very low weight (but above
   * 0.0!) will allow all other requesters to finish loading tiles before this
   * one starts.
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

  // So that the Tileset can create instances of this class.
  friend class Tileset;
};

} // namespace Cesium3DTilesSelection
