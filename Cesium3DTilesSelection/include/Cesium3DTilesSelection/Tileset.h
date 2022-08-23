#pragma once

#include "Library.h"
#include "RasterOverlayCollection.h"
#include "Tile.h"
#include "TilesetContentLoader.h"
#include "TilesetExternals.h"
#include "TilesetLoadFailureDetails.h"
#include "TilesetOptions.h"
#include "ViewState.h"
#include "ViewUpdateResult.h"

#include <CesiumAsync/AsyncSystem.h>

#include <rapidjson/fwd.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Cesium3DTilesSelection {
class TilesetContentManager;

/**
 * @brief A <a
 * href="https://github.com/CesiumGS/3d-tiles/tree/master/specification">3D
 * Tiles tileset</a>, used for streaming massive heterogeneous 3D geospatial
 * datasets.
 */
class CESIUM3DTILESSELECTION_API Tileset final {
public:
  /**
   * @brief Constructs a new instance with a given custom tileset loader.
   * @param externals The external interfaces to use.
   * @param pCustomLoader The custom loader used to load the tileset and tile
   * content.
   * @param pRootTile The root tile that is associated with the custom loader
   * @param options Additional options for the tileset.
   */
  Tileset(
      const TilesetExternals& externals,
      std::unique_ptr<TilesetContentLoader>&& pCustomLoader,
      std::unique_ptr<Tile>&& pRootTile,
      const TilesetOptions& options = TilesetOptions());

  /**
   * @brief Constructs a new instance with a given `tileset.json` URL.
   * @param externals The external interfaces to use.
   * @param url The URL of the `tileset.json`.
   * @param options Additional options for the tileset.
   */
  Tileset(
      const TilesetExternals& externals,
      const std::string& url,
      const TilesetOptions& options = TilesetOptions());

  /**
   * @brief Constructs a new instance with the given asset ID on <a
   * href="https://cesium.com/ion/">Cesium ion</a>.
   * @param externals The external interfaces to use.
   * @param ionAssetID The ID of the Cesium ion asset to use.
   * @param ionAccessToken The Cesium ion access token authorizing access to the
   * asset.
   * @param options Additional options for the tileset.
   * @param ionAssetEndpointUrl The URL of the ion asset endpoint. Defaults
   * to Cesium ion but a custom endpoint can be specified.
   */
  Tileset(
      const TilesetExternals& externals,
      int64_t ionAssetID,
      const std::string& ionAccessToken,
      const TilesetOptions& options = TilesetOptions(),
      const std::string& ionAssetEndpointUrl = "https://api.cesium.com/");

  /**
   * @brief Destroys this tileset.
   * This may block the calling thread while waiting for pending asynchronous
   * tile loads to terminate.
   */
  ~Tileset() noexcept;

  /**
   * @brief Get tileset credits.
   */
  const std::vector<Credit>& getTilesetCredits() const noexcept;

  /**
   * @brief Gets the {@link TilesetExternals} that summarize the external
   * interfaces used by this tileset.
   */
  TilesetExternals& getExternals() noexcept { return this->_externals; }

  /**
   * @brief Gets the {@link TilesetExternals} that summarize the external
   * interfaces used by this tileset.
   */
  const TilesetExternals& getExternals() const noexcept {
    return this->_externals;
  }

  /**
   * @brief Returns the {@link CesiumAsync::AsyncSystem} that is used for
   * dispatching asynchronous tasks.
   */
  CesiumAsync::AsyncSystem& getAsyncSystem() noexcept {
    return this->_asyncSystem;
  }

  /** @copydoc Tileset::getAsyncSystem() */
  const CesiumAsync::AsyncSystem& getAsyncSystem() const noexcept {
    return this->_asyncSystem;
  }

  /** @copydoc Tileset::getOptions() */
  const TilesetOptions& getOptions() const noexcept { return this->_options; }

  /**
   * @brief Gets the {@link TilesetOptions} of this tileset.
   */
  TilesetOptions& getOptions() noexcept { return this->_options; }

  /**
   * @brief Gets the root tile of this tileset.
   *
   * This may be `nullptr` if there is currently no root tile.
   */
  Tile* getRootTile() noexcept;

  /** @copydoc Tileset::getRootTile() */
  const Tile* getRootTile() const noexcept;

  /**
   * @brief Returns the {@link RasterOverlayCollection} of this tileset.
   */
  RasterOverlayCollection& getOverlays() noexcept;

  /** @copydoc Tileset::getOverlays() */
  const RasterOverlayCollection& getOverlays() const noexcept;

  /**
   * @brief Updates this view but waits for all tiles that meet sse to finish
   * loading and ready to be rendered before returning the function. This method
   * is significantly slower than {@link Tileset::updateView} and should only be
   * used for capturing movie or non-realtime situation.
   * @param frustums The {@link ViewState}s that the view should be updated for
   * @returns The set of tiles to render in the updated view. This value is only
   * valid until the next call to `updateView` or until the tileset is
   * destroyed, whichever comes first.
   */
  const ViewUpdateResult&
  updateViewOffline(const std::vector<ViewState>& frustums);

  /**
   * @brief Updates this view, returning the set of tiles to render in this
   * view.
   * @param frustums The {@link ViewState}s that the view should be updated for
   * @returns The set of tiles to render in the updated view. This value is only
   * valid until the next call to `updateView` or until the tileset is
   * destroyed, whichever comes first.
   */
  const ViewUpdateResult&
  updateView(const std::vector<ViewState>& frustums, float deltaTime = 0.0f);

  /**
   * @brief Estimate the percentage of the tiles for the current view that have
   * been loaded.
   */
  float computeLoadProgress() noexcept;

  /**
   * @brief Invokes a function for each tile that is currently loaded.
   *
   * @param callback The function to invoke.
   */
  void forEachLoadedTile(const std::function<void(Tile& tile)>& callback);

  /**
   * @brief Gets the total number of bytes of tile and raster overlay data that
   * are currently loaded.
   */
  int64_t getTotalDataBytes() const noexcept;

private:
  /**
   * @brief The result of traversing one branch of the tile hierarchy.
   *
   * Instances of this structure are created by the `_visit...` functions,
   * and summarize the information that was gathered during the traversal
   * of the respective branch, so that this information can be used by
   * the parent to decide on the further traversal process.
   */
  struct TraversalDetails {
    /**
     * @brief Whether all selected tiles in this tile's subtree are renderable.
     *
     * This is `true` if all selected (i.e. not culled or refined) tiles in this
     * tile's subtree are renderable. If the subtree is renderable, we'll render
     * it; no drama.
     */
    bool allAreRenderable = true;

    /**
     * @brief Whether any tile in this tile's subtree was rendered in the last
     * frame.
     *
     * This is `true` if any tiles in this tile's subtree were rendered last
     * frame. If any were, we must render the subtree rather than this tile,
     * because rendering this tile would cause detail to vanish that was visible
     * last frame, and that's no good.
     */
    bool anyWereRenderedLastFrame = false;

    /**
     * @brief The number of selected tiles in this tile's subtree that are not
     * yet renderable.
     *
     * Counts the number of selected tiles in this tile's subtree that are
     * not yet ready to be rendered because they need more loading. Note that
     * this value will _not_ necessarily be zero when
     * `allAreRenderable` is `true`, for subtle reasons.
     * When `allAreRenderable` and `anyWereRenderedLastFrame` are both `false`,
     * we will render this tile instead of any tiles in its subtree and the
     * `allAreRenderable` value for this tile will reflect only whether _this_
     * tile is renderable. The `notYetRenderableCount` value, however, will
     * still reflect the total number of tiles that we are waiting on, including
     * the ones that we're not rendering. `notYetRenderableCount` is only reset
     * when a subtree is removed from the render queue because the
     * `notYetRenderableCount` exceeds the
     * {@link TilesetOptions::loadingDescendantLimit}.
     */
    uint32_t notYetRenderableCount = 0;
  };

  /**
   * @brief Input information that is constant throughout the traversal.
   *
   * An instance of this structure is created upon entry of the top-level
   * `_visitTile` function, for the traversal for a certain frame, and
   * passed on through the traversal.
   */
  struct FrameState {
    const std::vector<ViewState>& frustums;
    std::vector<double> fogDensities;
    int32_t lastFrameNumber;
    int32_t currentFrameNumber;
  };

  TraversalDetails _renderLeaf(
      const FrameState& frameState,
      Tile& tile,
      double tilePriority,
      ViewUpdateResult& result);
  TraversalDetails _renderInnerTile(
      const FrameState& frameState,
      Tile& tile,
      ViewUpdateResult& result);
  TraversalDetails _refineToNothing(
      const FrameState& frameState,
      Tile& tile,
      ViewUpdateResult& result,
      bool areChildrenRenderable);
  bool _kickDescendantsAndRenderTile(
      const FrameState& frameState,
      Tile& tile,
      ViewUpdateResult& result,
      TraversalDetails& traversalDetails,
      size_t firstRenderedDescendantIndex,
      size_t loadIndexLow,
      size_t loadIndexMedium,
      size_t loadIndexHigh,
      bool queuedForLoad,
      double tilePriority);
  TileOcclusionState
  _checkOcclusion(const Tile& tile, const FrameState& frameState);

  TraversalDetails _visitTile(
      const FrameState& frameState,
      uint32_t depth,
      bool meetsSse,
      bool ancestorMeetsSse,
      Tile& tile,
      double tilePriority,
      ViewUpdateResult& result);

  struct CullResult {
    // whether we should visit this tile
    bool shouldVisit = true;
    // whether this tile was culled (Note: we might still want to visit it)
    bool culled = false;
  };

  // TODO: abstract these into a composable culling interface.
  void _frustumCull(
      const Tile& tile,
      const FrameState& frameState,
      bool cullWithChildrenBounds,
      CullResult& cullResult);
  void _fogCull(
      const FrameState& frameState,
      const std::vector<double>& distances,
      CullResult& cullResult);
  bool _meetsSse(
      const std::vector<ViewState>& frustums,
      const Tile& tile,
      const std::vector<double>& distances,
      bool culled) const noexcept;

  TraversalDetails _visitTileIfNeeded(
      const FrameState& frameState,
      uint32_t depth,
      bool ancestorMeetsSse,
      Tile& tile,
      ViewUpdateResult& result);
  TraversalDetails _visitVisibleChildrenNearToFar(
      const FrameState& frameState,
      uint32_t depth,
      bool ancestorMeetsSse,
      Tile& tile,
      ViewUpdateResult& result);

  /**
   * @brief When called on an additive-refined tile, queues it for load and adds
   * it to the render list.
   *
   * For replacement-refined tiles, this method does nothing and returns false.
   *
   * @param tile The tile to potentially load and render.
   * @param result The current view update result.
   * @param tilePriority The load priority of this tile.
   * priority.
   * @return true The additive-refined tile was queued for load and added to the
   * render list.
   * @return false The non-additive-refined tile was ignored.
   */
  bool _loadAndRenderAdditiveRefinedTile(
      Tile& tile,
      ViewUpdateResult& result,
      double tilePriority);

  /**
   * @brief Queues load of tiles that are _required_ to be loaded before the
   * given tile can be refined in "Forbid Holes" mode.
   *
   * The queued tiles may include descedents, too, if any children are set to
   * Unconditionally Refine ({@link Tile::getUnconditionallyRefine}).
   *
   * This method should only be called if {@link TilesetOptions::forbidHoles} is enabled.
   *
   * @param frameState The state of the current frame.
   * @param tile The tile that is potentially being refined.
   * @param implicitInfo The implicit traversal info.
   * @param tilePriority The load priority of this tile.
   * @return true Some of the required descendents are not yet loaded, so this
   * tile _cannot_ yet be refined.
   * @return false All of the required descendents (if there are any) are
   * loaded, so this tile _can_ be refined.
   */
  bool _queueLoadOfChildrenRequiredForForbidHoles(
      const FrameState& frameState,
      Tile& tile,
      double tilePriority);

  void _processLoadQueue();
  void _unloadCachedTiles() noexcept;
  void _markTileVisited(Tile& tile) noexcept;

  TilesetExternals _externals;
  CesiumAsync::AsyncSystem _asyncSystem;

  TilesetOptions _options;

  int32_t _previousFrameNumber;
  ViewUpdateResult _updateResult;

  struct LoadRecord {
    Tile* pTile;

    /**
     * @brief The relative priority of loading this tile.
     *
     * Lower priority values load sooner.
     */
    double priority;

    bool operator<(const LoadRecord& rhs) const noexcept {
      return this->priority < rhs.priority;
    }
  };

  std::vector<LoadRecord> _loadQueueHigh;
  std::vector<LoadRecord> _loadQueueMedium;
  std::vector<LoadRecord> _loadQueueLow;
  Tile::LoadedLinkedList _loadedTiles;

  // Holds computed distances, to avoid allocating them on the heap during tile
  // selection.
  std::vector<double> _distances;

  // Holds the occlusion proxies of the children of a tile. Store them in this
  // scratch variable so that it can allocate only when growing bigger.
  std::vector<const TileOcclusionRendererProxy*> _childOcclusionProxies;

  std::unique_ptr<TilesetContentManager> _pTilesetContentManager;

  void addTileToLoadQueue(
      std::vector<LoadRecord>& loadQueue,
      Tile& tile,
      double tilePriority);
  void processQueue(
      std::vector<Tileset::LoadRecord>& queue,
      int32_t maximumLoadsInProgress);

  Tileset(const Tileset& rhs) = delete;
  Tileset& operator=(const Tileset& rhs) = delete;
};

} // namespace Cesium3DTilesSelection
