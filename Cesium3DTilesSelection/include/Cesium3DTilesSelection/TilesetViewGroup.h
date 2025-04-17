#pragma once

#include <Cesium3DTilesSelection/TileLoadRequester.h>
#include <Cesium3DTilesSelection/TileLoadTask.h>
#include <Cesium3DTilesSelection/TileSelectionState.h>
#include <Cesium3DTilesSelection/ViewUpdateResult.h>
#include <CesiumUtility/CreditReferencer.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/TreeTraversalState.h>

#include <cstddef>
#include <unordered_map>
#include <vector>

namespace Cesium3DTilesSelection {

class Tile;
class Tileset;
class TilesetContentManager;
class TilesetFrameState;

/**
 * @brief Represents a group of views that collectively select tiles from a
 * particular {@link Tileset}.
 *
 * Create an instance of this class and pass it repeatedly to
 * {@link Tileset::updateViewGroup} to select tiles suitable for rendering the
 * tileset from a given view or set of views.
 *
 * This class is intentionally decoupled from {@link ViewState}, such that
 * clients are responsible for managing which views are represented by
 * any particular group.
 */
class CESIUM3DTILESSELECTION_API TilesetViewGroup final
    : public TileLoadRequester {
public:
  /**
   * @brief The type of the {@link CesiumUtility::TreeTraversalState}
   * used to track tile selection states for this view group.
   */
  using TraversalState =
      CesiumUtility::TreeTraversalState<Tile::Pointer, TileSelectionState>;

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
   * @brief Gets the result from the last time this view group was updated by
   * calling {@link Tileset::updateViewGroup}.
   */
  const ViewUpdateResult& getViewUpdateResult() const;

  /** @copydoc getViewUpdateResult */
  ViewUpdateResult& getViewUpdateResult();

  /**
   * @brief Gets an object used to track the selection state of tiles as they
   * are traversed for this view group.
   */
  TraversalState& getTraversalState() noexcept { return this->_traversalState; }

  /** @copydoc getTraversalState */
  const TraversalState& getTraversalState() const noexcept {
    return this->_traversalState;
  }

  /**
   * @brief Adds a tile load task to this view group's load queue.
   *
   * Each tile may only be added once per call to {@link startNewFrame}. Adding
   * a tile multiple times will lead to an assertion in debug builds and
   * undefined behavior in release builds.
   *
   * @param task The tile load task to add to the queue.
   */
  void addToLoadQueue(const TileLoadTask& task);

  /**
   * @brief A checkpoint within this view group's load queue.
   *
   * A checkpoint can be created by calling {@link saveTileLoadQueueCheckpoint}.
   * Later, calling {@link restoreTileLoadQueueCheckpoint} will remove all
   * tiles from the queue that were added since the checkpoint was saved.
   */
  struct LoadQueueCheckpoint {
  private:
    size_t mainThread;
    size_t workerThread;
    friend class TilesetViewGroup;
  };

  /**
   * @brief Saves a checkpoint of the tile load queue associated with this view
   * group.
   *
   * The saved checkpoint can later be restored by calling
   * {@link restoreTileLoadQueueCheckpoint}.
   *
   * This method should only be called in between calls to {@link startNewFrame}
   * and {@link finishFrame}.
   *
   * @return The checkpoint.
   */
  LoadQueueCheckpoint saveTileLoadQueueCheckpoint();

  /**
   * @brief Restores a previously-saved checkpoint of the tile load queue
   * associated with this view group.
   *
   * Restoring a checkpoint discards all tiles from the queue that were
   * requested, with a call to {@link addToLoadQueue}, since the checkpoint was
   * created.
   *
   * This method should only be called in between calls to {@link startNewFrame}
   * and {@link finishFrame}.
   *
   * @param checkpoint The previously-created checkpoint.
   * @return The number of tiles that were discarded from the queue as a result
   * of restoring the checkpoint.
   */
  size_t restoreTileLoadQueueCheckpoint(const LoadQueueCheckpoint& checkpoint);

  /**
   * @brief Gets the number of tiles that are currently in the queue waiting to
   * be loaded in the worker thread.
   */
  size_t getWorkerThreadLoadQueueLength() const;

  /**
   * @brief Gets the number of tiles that are currently in the queue waiting to
   * be loaded in the main thread.
   */
  size_t getMainThreadLoadQueueLength() const;

  /**
   * @brief Starts a new frame, clearing the set of tiles to be loaded so that a
   * new set can be selected.
   *
   * @param tileset The tileset that is starting the new frame.
   * @param frameState The state of the new frame.
   */
  void
  startNewFrame(const Tileset& tileset, const TilesetFrameState& frameState);

  /**
   * @brief Finishes the current frame by making the current tile selection
   * state the previous one and releasing references to tiles in the old
   * previous one.
   *
   * This method also updates the load progress percentage returned by
   * {@link getPreviousLoadProgressPercentage} and makes sure credits used by
   * this view group have been referenced on the
   * {@link CesiumUtility::CreditSystem}.
   *
   * @param tileset The tileset that is finishing the current frame.
   * @param frameState The state of the frame.
   */
  void finishFrame(const Tileset& tileset, const TilesetFrameState& frameState);

  /**
   * @brief Gets the previous load progress percentage for this view group as
   * of the last time it was updated.
   *
   * This method reports the progress as of the last call to {@link finishFrame}.
   *
   * The reported percentage is computed as:
   *
   * \f$100.0\frac{totalTilesVisited - tilesNeedingLoading}{totalTilesVisited}\f$
   *
   * When loading is complete, this method will return exactly 100.0.
   */
  float getPreviousLoadProgressPercentage() const;

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
  double _weight = 1.0;
  std::vector<TileLoadTask> _mainThreadLoadQueue;
  std::vector<TileLoadTask> _workerThreadLoadQueue;
  size_t _tilesAlreadyLoadingOrUnloading = 0;
  float _loadProgressPercentage = 0.0f;
  ViewUpdateResult _updateResult;
  TraversalState _traversalState;
  CesiumUtility::CreditReferencer _previousFrameCredits;
  CesiumUtility::CreditReferencer _currentFrameCredits;
};

} // namespace Cesium3DTilesSelection
