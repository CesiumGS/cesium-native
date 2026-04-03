#pragma once

#include "Cesium3DTilesSelection/RasterOverlayCollection.h"
#include "Cesium3DTilesSelection/TilesetOptions.h"
#include "CesiumAsync/AsyncSystem.h"
#include "CesiumUtility/CreditSystem.h"
#include "CesiumUtility/ReferenceCounted.h"

#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileOcclusionRendererProxy.h>
#include <Cesium3DTilesSelection/TilesetFrameState.h>
#include <Cesium3DTilesSelection/TilesetViewGroup.h>
#include <Cesium3DTilesSelection/ViewUpdateResult.h>

#include <cstdint>

namespace Cesium3DTilesSelection {
class ITilesetFrameInfo {
public:
  virtual bool getEnableLodTransitionPeriod() const = 0;
  virtual const std::shared_ptr<CesiumUtility::CreditSystem>&
  getCreditSystem() const = 0;
  virtual std::optional<CesiumUtility::Credit>
  getUserCredit() const noexcept = 0;
  virtual const std::vector<CesiumUtility::Credit>&
  getTilesetCredits() const noexcept = 0;
  virtual const RasterOverlayCollection& getOverlays() const noexcept = 0;
};

class TilesetTileSelection
    : public CesiumUtility::ReferenceCountedNonThreadSafe<
          TilesetTileSelection> {

public:
  const ViewUpdateResult& updateViewGroup(
      const ITilesetFrameInfo& tilesetFrameInfo,
      TilesetViewGroup& viewGroup,
      const std::vector<ViewState>& frustums,
      float deltaTime);

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

  enum class TileSelectionCullingFlags : uint32_t {
    None = 0,
    Fog = 1 << 0,
    Frustum = 1 << 1,
    Excluder = 1 << 2,
    ChildBounds = 1 << 3,
    Occlusion = 1 << 4,
    All = std::numeric_limits<uint32_t>::max()
  };

  TraversalDetails _renderLeaf(
      const TilesetFrameState& frameState,
      Tile& tile,
      double tilePriority,
      ViewUpdateResult& result);
  TraversalDetails _renderInnerTile(
      const TilesetFrameState& frameState,
      Tile& tile,
      ViewUpdateResult& result);
  bool _kickDescendantsAndRenderTile(
      const TilesetFrameState& frameState,
      Tile& tile,
      ViewUpdateResult& result,
      TraversalDetails& traversalDetails,
      size_t firstRenderedDescendantIndex,
      const TilesetViewGroup::LoadQueueCheckpoint& loadQueueBeforeChildren,
      bool queuedForLoad,
      double tilePriority);
  TileOcclusionState _checkOcclusion(const Tile& tile);

  TraversalDetails _visitTile(
      const TilesetFrameState& frameState,
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
      const TilesetFrameState& frameState,
      bool cullWithChildrenBounds,
      CullResult& cullResult);
  void _fogCull(
      const TilesetFrameState& frameState,
      const std::vector<double>& distances,
      CullResult& cullResult);
  bool _meetsSse(
      const std::vector<ViewState>& frustums,
      const Tile& tile,
      const std::vector<double>& distances,
      bool culled) const noexcept;

  TraversalDetails _visitTileIfNeeded(
      const TilesetFrameState& frameState,
      uint32_t depth,
      bool ancestorMeetsSse,
      Tile& tile,
      ViewUpdateResult& result);
  TraversalDetails _visitVisibleChildrenNearToFar(
      const TilesetFrameState& frameState,
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
   * @param queuedForLoad True if this tile has already been queued for loading.
   * @return true The additive-refined tile was queued for load and added to the
   * render list.
   * @return false The non-additive-refined tile was ignored.
   */
  bool _loadAndRenderAdditiveRefinedTile(
      const TilesetFrameState& frameState,
      Tile& tile,
      ViewUpdateResult& result,
      double tilePriority,
      bool queuedForLoad);

  void _unloadCachedTiles(double timeBudget) noexcept;

  void _updateLodTransitions(
      const TilesetFrameState& frameState,
      float deltaTime,
      ViewUpdateResult& result) const noexcept;

  void addTileToLoadQueue(
      const TilesetFrameState& frameState,
      Tile& tile,
      TileLoadPriorityGroup priorityGroup,
      double priority);

  static TraversalDetails createTraversalDetailsForSingleTile(
      const TilesetFrameState& frameState,
      const Tile& tile);

  bool hasCullingFlag(TileSelectionCullingFlags flag);

  TilesetOptions _options;
  TileSelectionCullingFlags _cullingFlags = TileSelectionCullingFlags::All;
  CesiumAsync::AsyncSystem _asyncSystem;
  CesiumUtility::IntrusivePointer<TilesetContentManager>
      _pTilesetContentManager;
  // Holds computed distances, to avoid allocating them on the heap during tile
  // selection.
  std::vector<double> _distances;
  // Holds the occlusion proxies of the children of a tile. Store them in this
  // scratch variable so that it can allocate only when growing bigger.
  std::vector<const TileOcclusionRendererProxy*> _childOcclusionProxies;
};
} // namespace Cesium3DTilesSelection