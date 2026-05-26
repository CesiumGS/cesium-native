#include <Cesium3DTilesSelection/BoundingVolume.h>
#include <Cesium3DTilesSelection/ITileExcluder.h>
#include <Cesium3DTilesSelection/RasterMappedTo3DTile.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/TileLoadTask.h>
#include <Cesium3DTilesSelection/TileOcclusionRendererProxy.h>
#include <Cesium3DTilesSelection/TileRefine.h>
#include <Cesium3DTilesSelection/TileSelectionState.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/TilesetFrameState.h>
#include <Cesium3DTilesSelection/TilesetOptions.h>
#include <Cesium3DTilesSelection/TilesetSelection.h>
#include <Cesium3DTilesSelection/TilesetViewGroup.h>
#include <Cesium3DTilesSelection/ViewState.h>
#include <Cesium3DTilesSelection/ViewUpdateResult.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumUtility/Assert.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/Math.h>

#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/geometric.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <vector>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumRasterOverlays;
using namespace CesiumUtility;

namespace Cesium3DTilesSelection {

namespace {

/**
 * @brief The result of traversing one branch of the tile hierarchy.
 *
 * Instances of this structure are created by the `_visit...` functions,
 * and summarize the information that was gathered during the traversal
 * of the respective branch, so that this information can be used by
 * the parent to decide on the further traversal process.
 *
 * @private
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

struct CullResult {
  // whether we should visit this tile
  bool shouldVisit = true;
  // whether this tile was culled (Note: we might still want to visit it)
  bool culled = false;
};

enum class VisitTileAction { Render, Refine };

// Visits a tile for possible rendering. When we call this function with a tile:
//   * It is not yet known whether the tile is visible.
//   * Its parent tile does _not_ meet the SSE (unless ancestorMeetsSse=true,
//   see comments below).
//   * The tile may or may not be renderable.
//   * The tile has not yet been added to a load queue.
TraversalDetails visitTileIfNeeded(
    const TileSelectionContext& context,
    const TilesetFrameState& frameState,
    uint32_t depth,
    bool ancestorMeetsSse,
    Tile& tile,
    ViewUpdateResult& result);

} // namespace

void selectTiles(
    const TileSelectionContext& context,
    const TilesetFrameState& frameState,
    Tile& rootTile,
    ViewUpdateResult& result) {

  visitTileIfNeeded(context, frameState, 0, false, rootTile, result);
}

namespace {

TileSelectionState getPreviousState(
    const TilesetViewGroup& viewGroup,
    [[maybe_unused]] const Tile& tile) {
  const TilesetViewGroup::TraversalState& traversalState =
      viewGroup.getTraversalState();
  CESIUM_ASSERT(traversalState.getCurrentNode() == &tile);
  const TileSelectionState* pState = traversalState.previousState();
  return pState == nullptr ? TileSelectionState() : *pState;
}

void addToTilesFadingOutIfPreviouslyRendered(
    TileSelectionState::Result lastResult,
    Tile& tile,
    ViewUpdateResult& result) {
  if (lastResult == TileSelectionState::Result::Rendered ||
      (lastResult == TileSelectionState::Result::Refined &&
       tile.getRefine() == TileRefine::Add)) {
    result.tilesFadingOut.insert(&tile);
    TileRenderContent* pRenderContent = tile.getContent().getRenderContent();
    if (pRenderContent) {
      pRenderContent->setLodTransitionFadePercentage(0.0f);
    }
  }
}

void addCurrentTileToTilesFadingOutIfPreviouslyRendered(
    TilesetViewGroup& viewGroup,
    Tile& tile,
    ViewUpdateResult& result) {
  TileSelectionState::Result lastResult =
      getPreviousState(viewGroup, tile).getResult();
  addToTilesFadingOutIfPreviouslyRendered(lastResult, tile, result);
}

void addCurrentTileDescendantsToTilesFadingOutIfPreviouslyRendered(
    TilesetViewGroup& viewGroup,
    [[maybe_unused]] Tile& tile,
    ViewUpdateResult& result) {
  if (getPreviousState(viewGroup, tile).getResult() ==
      TileSelectionState::Result::Refined) {
    viewGroup.getTraversalState().forEachPreviousDescendant(
        [&](const Tile::Pointer& pTile, const TileSelectionState& state) {
          addToTilesFadingOutIfPreviouslyRendered(
              state.getResult(),
              *pTile,
              result);
        });
  }
}

void addCurrentTileAndDescendantsToTilesFadingOutIfPreviouslyRendered(
    TilesetViewGroup& viewGroup,
    Tile& tile,
    ViewUpdateResult& result) {
  addCurrentTileToTilesFadingOutIfPreviouslyRendered(viewGroup, tile, result);
  addCurrentTileDescendantsToTilesFadingOutIfPreviouslyRendered(
      viewGroup,
      tile,
      result);
}

/**
 * @brief Returns whether a tile with the given bounding volume is visible for
 * the camera.
 *
 * @param viewState The {@link ViewState}
 * @param boundingVolume The bounding volume of the tile
 * @param forceRenderTilesUnderCamera Whether tiles under the camera should
 * always be considered visible and rendered (see
 * {@link Cesium3DTilesSelection::TilesetOptions}).
 * @return Whether the tile is visible according to the current camera
 * configuration
 */
bool isVisibleFromCamera(
    const ViewState& viewState,
    const BoundingVolume& boundingVolume,
    const Ellipsoid& ellipsoid,
    bool forceRenderTilesUnderCamera) {
  if (viewState.isBoundingVolumeVisible(boundingVolume)) {
    return true;
  }
  if (!forceRenderTilesUnderCamera) {
    return false;
  }
  const std::optional<CesiumGeospatial::Cartographic>& position =
      viewState.getPositionCartographic();

  // TODO: it would be better to test a line pointing down (and up?) from the
  // camera against the bounding volume itself, rather than transforming the
  // bounding volume to a region.
  std::optional<GlobeRectangle> maybeRectangle =
      estimateGlobeRectangle(boundingVolume, ellipsoid);
  if (position && maybeRectangle) {
    return maybeRectangle->contains(position.value());
  }
  return false;
}

/**
 * @brief Returns whether a tile at the given distance is visible in the fog.
 *
 * @param distance The distance of the tile bounding volume to the camera
 * @param fogDensity The fog density
 * @return Whether the tile is visible in the fog
 */
bool isVisibleInFog(double distance, double fogDensity) noexcept {
  if (fogDensity <= 0.0) {
    return true;
  }
  const double fogScalar = distance * fogDensity;
  return glm::exp(-(fogScalar * fogScalar)) > 0.0;
}

double computeTilePriority(
    const Tile& tile,
    const std::vector<ViewState>& frustums,
    const std::vector<double>& distances) {
  double highestLoadPriority = std::numeric_limits<double>::max();
  const glm::dvec3 boundingVolumeCenter =
      getBoundingVolumeCenter(tile.getBoundingVolume());

  for (size_t i = 0; i < frustums.size() && i < distances.size(); ++i) {
    const ViewState& frustum = frustums[i];
    const double distance = distances[i];

    glm::dvec3 tileDirection = boundingVolumeCenter - frustum.getPosition();
    const double magnitude = glm::length(tileDirection);

    if (magnitude >= CesiumUtility::Math::Epsilon5) {
      tileDirection /= magnitude;
      const double loadPriority =
          (1.0 - glm::dot(tileDirection, frustum.getDirection())) * distance;
      if (loadPriority < highestLoadPriority) {
        highestLoadPriority = loadPriority;
      }
    }
  }
  return highestLoadPriority;
}

void computeDistances(
    const Tile& tile,
    const std::vector<ViewState>& frustums,
    std::vector<double>& distances) {
  const BoundingVolume& boundingVolume = tile.getBoundingVolume();
  distances.clear();
  distances.resize(frustums.size());
  std::transform(
      frustums.begin(),
      frustums.end(),
      distances.begin(),
      [boundingVolume](const ViewState& frustum) -> double {
        return glm::sqrt(glm::max(
            frustum.computeDistanceSquaredToBoundingVolume(boundingVolume),
            0.0));
      });
}

void addTileToLoadQueue(
    const TileSelectionContext& context,
    const TilesetFrameState& frameState,
    Tile& tile,
    TileLoadPriorityGroup priorityGroup,
    double priority) {
  frameState.viewGroup.addToLoadQueue(
      TileLoadTask{&tile, priorityGroup, priority},
      context.externals.pGltfModifier);
}

void addTileToRender(ViewUpdateResult& result, Tile& tile, double sse) {
  result.tilesToRenderThisFrame.emplace_back(&tile);
  result.tileScreenSpaceErrorThisFrame.emplace_back(sse);
}

double computeSse(
    const TileSelectionContext& context,
    const TilesetFrameState& frameState,
    const Tile& tile) noexcept {
  double largestSse = 0.0;
  const auto& frustums = frameState.frustums;
  const auto& distances = context.scratchDistances;
  CESIUM_ASSERT(frustums.size() == distances.size());
  for (size_t i = 0; i < frustums.size(); ++i) {
    const double sse = frustums[i].computeScreenSpaceError(
        tile.getGeometricError(),
        distances[i]);
    if (sse > largestSse) {
      largestSse = sse;
    }
  }
  return largestSse;
}

bool meetsSseThreshold(
    const TileSelectionContext& context,
    double sse,
    bool culled) noexcept {
  return culled ? !context.options.enforceCulledScreenSpaceError ||
                      sse < context.options.culledScreenSpaceError
                : sse < context.options.maximumScreenSpaceError;
}

bool isLeaf(const Tile& tile) noexcept { return tile.getChildren().empty(); }

/**
 * @brief Determines if we must refine this tile so that we can continue
 * rendering the deeper descendant tiles of this tile.
 *
 * If this tile was refined last frame, and is not yet renderable, then we
 * should REFINE past this tile in order to continue rendering the deeper tiles
 * that we rendered last frame, until such time as this tile is loaded and we
 * can render it instead. This is necessary to avoid detail vanishing when
 * the camera zooms out and lower-detail tiles are not yet loaded.
 *
 * @param tile The tile to check, which is assumed to meet the SSE for
 * rendering.
 * @param lastFrameSelectionState The selection state of this tile last frame.
 * @return True if this tile must be refined instead of rendered, so that we can
 * continue rendering deeper tiles.
 */
bool mustContinueRefiningToDeeperTiles(
    const Tile& tile,
    const TileSelectionState& lastFrameSelectionState) noexcept {
  const TileSelectionState::Result originalResult =
      lastFrameSelectionState.getOriginalResult();
  return originalResult == TileSelectionState::Result::Refined &&
         !tile.isRenderable();
}

TraversalDetails visitVisibleChildrenNearToFar(
    const TileSelectionContext& context,
    const TilesetFrameState& frameState,
    uint32_t depth,
    bool ancestorMeetsSse,
    Tile& tile,
    ViewUpdateResult& result);

// TODO: abstract thse into a composable culling interface.
void frustumCull(
    const TileSelectionContext& context,
    const TilesetFrameState& frameState,
    const Tile& tile,
    bool cullWithChildrenBounds,
    CullResult& cullResult) {

  if (!cullResult.shouldVisit || cullResult.culled) {
    return;
  }

  const Ellipsoid& ellipsoid = context.options.ellipsoid;
  const std::vector<ViewState>& frustums = frameState.frustums;

  if (cullWithChildrenBounds) {
    // Frustum cull using the children's bounds.
    if (std::any_of(
            frustums.begin(),
            frustums.end(),
            [&ellipsoid,
             children = tile.getChildren(),
             renderTilesUnderCamera = context.options.renderTilesUnderCamera](
                const ViewState& frustum) {
              for (const Tile& child : children) {
                if (isVisibleFromCamera(
                        frustum,
                        child.getBoundingVolume(),
                        ellipsoid,
                        renderTilesUnderCamera)) {
                  return true;
                }
              }
              return false;
            })) {
      // At least one child is visible in at least one frustum, so don't cull.
      return;
    }
    // Frustum cull based on the actual tile's bounds.
  } else if (std::any_of(
                 frustums.begin(),
                 frustums.end(),
                 [&ellipsoid,
                  &boundingVolume = tile.getBoundingVolume(),
                  renderTilesUnderCamera =
                      context.options.renderTilesUnderCamera](
                     const ViewState& frustum) {
                   return isVisibleFromCamera(
                       frustum,
                       boundingVolume,
                       ellipsoid,
                       renderTilesUnderCamera);
                 })) {
    // The tile is visible in at least one frustum, so don't cull.
    return;
  }

  // If we haven't returned yet, this tile is frustum culled.
  cullResult.culled = true;

  if (context.options.enableFrustumCulling) {
    // frustum culling is enabled so we shouldn't visit this off-screen tile
    cullResult.shouldVisit = false;
  }
}

void fogCull(
    const TileSelectionContext& context,
    const TilesetFrameState& frameState,
    CullResult& cullResult) {

  if (!cullResult.shouldVisit || cullResult.culled) {
    return;
  }

  const auto& frustums = frameState.frustums;
  const auto& fogDensities = frameState.fogDensities;
  const auto& distances = context.scratchDistances;

  // prevent out-of-bounds access in the loops below.
  CESIUM_ASSERT(fogDensities.size() == frustums.size());

  // distances is always resized to frustums.size() by computeDistances which is
  // called just before fogCull in visitTileIfNeeded so distances.size() should
  // always be the same as frustums.size() here, but we'll assert just in case
  // this is ever called independently.
  CESIUM_ASSERT(distances.size() == frustums.size());
  bool isFogCulled = true;
  for (size_t i = 0; i < frustums.size(); ++i) {
    if (isVisibleInFog(distances[i], fogDensities[i])) {
      isFogCulled = false;
      break;
    }
  }

  if (isFogCulled) {
    // this tile is occluded by fog so it is a culled tile
    cullResult.culled = true;
    if (context.options.enableFogCulling) {
      // fog culling is enabled so we shouldn't visit this tile
      cullResult.shouldVisit = false;
    }
  }
}

TileOcclusionState
checkOcclusion(const TileSelectionContext& context, const Tile& tile) {
  const std::shared_ptr<TileOcclusionRendererProxyPool>& pOcclusionPool =
      context.externals.pTileOcclusionProxyPool;
  if (!pOcclusionPool) {
    // We don't have an occlusion pool to query occlusion with, treat everything
    // as unoccluded.
    return TileOcclusionState::NotOccluded;
  }

  // First check if this tile's bounding volume has occlusion info and is
  // known to be occluded.
  const TileOcclusionRendererProxy* pOcclusion =
      pOcclusionPool->fetchOcclusionProxyForTile(tile);
  if (!pOcclusion) {
    // This indicates we ran out of occlusion proxies. We don't want to wait
    // on occlusion info here since it might not ever arrive, so treat this
    // tile as if it is _known_ to be unoccluded.
    return TileOcclusionState::NotOccluded;
  }

  switch (static_cast<TileOcclusionState>(pOcclusion->getOcclusionState())) {
  case TileOcclusionState::OcclusionUnavailable:
    // We have an occlusion proxy, but it does not have valid occlusion
    // info yet, wait for it.
    return TileOcclusionState::OcclusionUnavailable;
  case TileOcclusionState::Occluded:
    return TileOcclusionState::Occluded;
  case TileOcclusionState::NotOccluded:
    if (tile.getChildren().empty()) {
      // This is a leaf tile, so we can't use children bounding volumes.
      return TileOcclusionState::NotOccluded;
    }
    break;
  }

  // The tile's bounding volume is known to be unoccluded, but check the
  // union of the children bounding volumes since it is tighter fitting.

  // If any children are to be unconditionally refined, we can't rely on
  // their bounding volumes. We also don't want to recurse indefinitely to
  // find a valid descendant bounding volumes union.
  for (const Tile& child : tile.getChildren()) {
    if (child.getUnconditionallyRefine()) {
      return TileOcclusionState::NotOccluded;
    }
  }

  std::vector<const TileOcclusionRendererProxy*>& childOcclusionProxies =
      context.scratchOcclusionProxies;
  childOcclusionProxies.clear();
  childOcclusionProxies.reserve(tile.getChildren().size());
  for (const Tile& child : tile.getChildren()) {
    const TileOcclusionRendererProxy* pChildProxy =
        pOcclusionPool->fetchOcclusionProxyForTile(child);
    if (!pChildProxy) {
      // We ran out of occlusion proxies, treat this as if it is _known_ to
      // be unoccluded so we don't wait for it.
      return TileOcclusionState::NotOccluded;
    }
    childOcclusionProxies.push_back(pChildProxy);
  }

  // Check if any of the proxies are known to be unoccluded.
  for (const TileOcclusionRendererProxy* pChildProxy : childOcclusionProxies) {
    if (pChildProxy->getOcclusionState() == TileOcclusionState::NotOccluded) {
      return TileOcclusionState::NotOccluded;
    }
  }

  // Check if any of the proxies are waiting for valid occlusion info.
  for (const TileOcclusionRendererProxy* pChildProxy : childOcclusionProxies) {
    if (pChildProxy->getOcclusionState() ==
        TileOcclusionState::OcclusionUnavailable) {
      // We have an occlusion proxy, but it does not have valid occlusion
      // info yet, wait for it.
      return TileOcclusionState::OcclusionUnavailable;
    }
  }

  // If we know the occlusion state of all children, and none are unoccluded,
  // we can treat this tile as occluded.
  return TileOcclusionState::Occluded;
}

TraversalDetails createTraversalDetailsForSingleTile(
    const TilesetFrameState& frameState,
    const Tile& tile) {
  TileSelectionState::Result lastFrameResult =
      getPreviousState(frameState.viewGroup, tile).getResult();

  bool isRenderable = tile.isRenderable();
  bool wasRenderedLastFrame =
      lastFrameResult == TileSelectionState::Result::Rendered;

  if (!wasRenderedLastFrame &&
      lastFrameResult == TileSelectionState::Result::Refined) {
    if (tile.getRefine() == TileRefine::Add) {
      wasRenderedLastFrame = true;
    } else {
      // With replace-refinement, if any of this refined tile's children were
      // rendered last frame, but are no longer rendered because this tile is
      // loaded and has sufficient detail, we must treat this tile as rendered
      // last frame, too. This is necessary to prevent this tile from being
      // kicked just because _it_ wasn't rendered last frame (which could cause
      // a new hole to appear).
      frameState.viewGroup.getTraversalState().forEachPreviousDescendant(
          [&](const Tile::Pointer& /* pTile */,
              const TileSelectionState& state) {
            if (state.getResult() == TileSelectionState::Result::Rendered) {
              wasRenderedLastFrame = true;
            }
          });
    }
  }

  TraversalDetails details;
  details.allAreRenderable = isRenderable;
  details.anyWereRenderedLastFrame = isRenderable && wasRenderedLastFrame;
  details.notYetRenderableCount = isRenderable ? 0 : 1;
  return details;
}

TraversalDetails renderLeaf(
    const TileSelectionContext& context,
    const TilesetFrameState& frameState,
    Tile& tile,
    double tilePriority,
    double tileSse,
    ViewUpdateResult& result) {
  frameState.viewGroup.getTraversalState().currentState() =
      TileSelectionState(TileSelectionState::Result::Rendered);
  addTileToRender(result, tile, tileSse);
  addTileToLoadQueue(
      context,
      frameState,
      tile,
      TileLoadPriorityGroup::Normal,
      tilePriority);
  return createTraversalDetailsForSingleTile(frameState, tile);
}

TraversalDetails renderInnerTile(
    const TilesetFrameState& frameState,
    Tile& tile,
    double tileSse,
    ViewUpdateResult& result) {
  addCurrentTileDescendantsToTilesFadingOutIfPreviouslyRendered(
      frameState.viewGroup,
      tile,
      result);
  frameState.viewGroup.getTraversalState().currentState() =
      TileSelectionState(TileSelectionState::Result::Rendered);
  addTileToRender(result, tile, tileSse);
  return createTraversalDetailsForSingleTile(frameState, tile);
}

/**
 * @brief When called on an additive-refined tile, queues it for load and adds
 * it to the render list.
 *
 * For replacement-refined tiles, this method does nothing and returns false.
 *
 * @param context The tile selection context.
 * @param frameState The current frame state.
 * @param tile The tile to potentially load and render.
 * @param result The current view update result.
 * @param tilePriority The load priority of this tile.
 * priority.
 * @param tileSse The screen space error of this tile.
 * @param queuedForLoad True if this tile has already been queued for loading.
 * @return true The additive-refined tile was queued for load and added to the
 * render list.
 * @return false The non-additive-refined tile was ignored.
 */
bool loadAndRenderAdditiveRefinedTile(
    const TileSelectionContext& context,
    const TilesetFrameState& frameState,
    Tile& tile,
    ViewUpdateResult& result,
    double tilePriority,
    double tileSse,
    bool queuedForLoad) {
  // If this tile uses additive refinement, we need to render this tile in
  // addition to its children.
  if (tile.getRefine() == TileRefine::Add) {
    addTileToRender(result, tile, tileSse);
    if (!queuedForLoad) {
      addTileToLoadQueue(
          context,
          frameState,
          tile,
          TileLoadPriorityGroup::Normal,
          tilePriority);
    }
    return true;
  }
  return false;
}

bool kickDescendantsAndRenderTile(
    const TileSelectionContext& context,
    const TilesetFrameState& frameState,
    Tile& tile,
    ViewUpdateResult& result,
    TraversalDetails& traversalDetails,
    size_t firstRenderedDescendantIndex,
    const TilesetViewGroup::LoadQueueCheckpoint& loadQueueBeforeChildren,
    bool queuedForLoad,
    double tilePriority,
    double tileSse) {
  // Mark all visited descendants of this tile as kicked.
  TilesetViewGroup::TraversalState& traversalState =
      frameState.viewGroup.getTraversalState();

  traversalState.forEachCurrentDescendant(
      [](const Tile::Pointer& /*pTile*/, TileSelectionState& selectionState) {
        selectionState.kick();
      });

  // If any kicked tiles were rendered last frame, add them to the
  // tilesFadingOut. This is unlikely! It would imply that a tile rendered last
  // frame has suddenly become unrenderable, and therefore eligible for kicking.
  //
  // In general, it's possible that a Tile previously traversed has been deleted
  // completely, so we have to be careful about dereferencing the Tile pointers
  // given to the callback below. However, we can be certain that a Tile that
  // was rendered last frame has _not_ been deleted yet.
  traversalState.forEachPreviousDescendant(
      [&result](
          const Tile::Pointer& pTile,
          const TileSelectionState& previousState) {
        addToTilesFadingOutIfPreviouslyRendered(
            previousState.getResult(),
            *pTile,
            result);
      });

  // Remove all descendants from the render list and add this tile.
  std::vector<Tile::ConstPointer>& renderList = result.tilesToRenderThisFrame;
  std::vector<double>& sseList = result.tileScreenSpaceErrorThisFrame;
  renderList.erase(
      renderList.begin() +
          static_cast<std::vector<Tile*>::iterator::difference_type>(
              firstRenderedDescendantIndex),
      renderList.end());
  sseList.erase(
      sseList.begin() +
          static_cast<std::vector<double>::iterator::difference_type>(
              firstRenderedDescendantIndex),
      sseList.end());

  if (tile.getRefine() != TileRefine::Add) {
    addTileToRender(result, tile, tileSse);
  }

  traversalState.currentState() =
      TileSelectionState(TileSelectionState::Result::Rendered);

  // If we're waiting on heaps of descendants, the above will take too long. So
  // in that case, load this tile INSTEAD of loading any of the descendants, and
  // tell the up-level we're only waiting on this tile. Keep doing this until we
  // actually manage to render this tile.
  // Make sure we don't end up waiting on a tile that will _never_ be
  // renderable.
  TileSelectionState::Result lastFrameSelectionState =
      getPreviousState(frameState.viewGroup, tile).getResult();
  const bool wasRenderedLastFrame =
      lastFrameSelectionState == TileSelectionState::Result::Rendered;
  const bool isRenderable = tile.isRenderable();
  const bool wasReallyRenderedLastFrame = wasRenderedLastFrame && isRenderable;

  if (!wasReallyRenderedLastFrame &&
      traversalDetails.notYetRenderableCount >
          context.options.loadingDescendantLimit &&
      !tile.isExternalContent() && !tile.getUnconditionallyRefine()) {

    // Remove all descendants from the load queues.
    result.tilesKicked += static_cast<uint32_t>(
        frameState.viewGroup.restoreTileLoadQueueCheckpoint(
            loadQueueBeforeChildren));

    if (!queuedForLoad) {
      addTileToLoadQueue(
          context,
          frameState,
          tile,
          TileLoadPriorityGroup::Normal,
          tilePriority);
    }

    traversalDetails.notYetRenderableCount = tile.isRenderable() ? 0 : 1;
    queuedForLoad = true;
  }

  traversalDetails.allAreRenderable = isRenderable;
  traversalDetails.anyWereRenderedLastFrame = wasReallyRenderedLastFrame;
  return queuedForLoad;
}

TraversalDetails visitVisibleChildrenNearToFar(
    const TileSelectionContext& context,
    const TilesetFrameState& frameState,
    uint32_t depth,
    bool ancestorMeetsSse,
    Tile& tile,
    ViewUpdateResult& result) {
  TraversalDetails traversalDetails;
  // TODO: actually visit near-to-far, rather than in order of occurrence.
  for (Tile& child : tile.getChildren()) {
    const TraversalDetails childTraversal = visitTileIfNeeded(
        context,
        frameState,
        depth + 1,
        ancestorMeetsSse,
        child,
        result);
    traversalDetails.allAreRenderable &= childTraversal.allAreRenderable;
    traversalDetails.anyWereRenderedLastFrame |=
        childTraversal.anyWereRenderedLastFrame;
    traversalDetails.notYetRenderableCount +=
        childTraversal.notYetRenderableCount;
  }
  return traversalDetails;
}

// Visits a tile for possible rendering. When we call this function with a tile:
//   * The tile has previously been determined to be visible.
//   * Its parent tile does _not_ meet the SSE (unless ancestorMeetsSse=true,
//   see comments below).
//   * The tile may or may not be renderable.
//   * The tile has not yet been added to a load queue.
TraversalDetails visitTile(
    const TileSelectionContext& context,
    const TilesetFrameState& frameState,
    uint32_t depth,
    bool meetsSse,
    bool ancestorMeetsSse, // Careful: May be modified before being passed to
                           // children!
    Tile& tile,
    double tilePriority,
    double tileSse,
    ViewUpdateResult& result) {

  TilesetViewGroup::TraversalState& traversalState =
      frameState.viewGroup.getTraversalState();

  ++result.tilesVisited;
  result.maxDepthVisited = glm::max(result.maxDepthVisited, depth);

  // If this is a leaf tile, just render it (it's already been deemed visible).
  if (isLeaf(tile)) {
    return renderLeaf(context, frameState, tile, tilePriority, tileSse, result);
  }

  const bool unconditionallyRefine = tile.getUnconditionallyRefine();
  const bool refineForSse = !meetsSse && !ancestorMeetsSse;

  // Determine whether to REFINE or RENDER. Note that even if this tile is
  // initially marked for RENDER here, it may later switch to REFINE as a
  // result of `mustContinueRefiningToDeeperTiles`.
  VisitTileAction action = (unconditionallyRefine || refineForSse)
                               ? VisitTileAction::Refine
                               : VisitTileAction::Render;

  TileSelectionState lastFrameSelectionState =
      getPreviousState(frameState.viewGroup, tile);
  TileSelectionState::Result lastFrameSelectionResult =
      lastFrameSelectionState.getResult();

  // If occlusion culling is enabled, we may not want to refine for two
  // reasons:
  // - The tile is known to be occluded, so don't refine further.
  // - The tile was not previously refined and the occlusion state for this
  //   tile is not known yet, but will be known in the next several frames. If
  //   delayRefinementForOcclusion is enabled, we will wait until the tile has
  //   valid occlusion info to decide to refine. This might save us from
  //   kicking off descendant loads that we later find to be unnecessary.
  bool tileLastRefined =
      lastFrameSelectionResult == TileSelectionState::Result::Refined;
  bool childLastRefined = false;
  traversalState.forEachPreviousChild(
      [&](const Tile::Pointer& /*pTile*/, const TileSelectionState& state) {
        if (state.getResult() == TileSelectionState::Result::Refined) {
          childLastRefined = true;
        }
      });

  // If this tile and a child were both refined last frame, this tile does not
  // need occlusion results.
  bool shouldCheckOcclusion = context.options.enableOcclusionCulling &&
                              action == VisitTileAction::Refine &&
                              !unconditionallyRefine &&
                              (!tileLastRefined || !childLastRefined);

  if (shouldCheckOcclusion) {
    TileOcclusionState occlusion = checkOcclusion(context, tile);
    if (occlusion == TileOcclusionState::Occluded) {
      ++result.tilesOccluded;
      action = VisitTileAction::Render;
      meetsSse = true;
    } else if (
        occlusion == TileOcclusionState::OcclusionUnavailable &&
        context.options.delayRefinementForOcclusion &&
        lastFrameSelectionState.getOriginalResult() !=
            TileSelectionState::Result::Refined) {
      ++result.tilesWaitingForOcclusionResults;
      action = VisitTileAction::Render;
      meetsSse = true;
    }
  }

  bool queuedForLoad = false;

  if (action == VisitTileAction::Render) {
    // This tile meets the screen-space error requirement, so we'd like to
    // render it, if we can.
    bool mustRefine =
        mustContinueRefiningToDeeperTiles(tile, lastFrameSelectionState);
    if (mustRefine) {
      action = VisitTileAction::Refine;
      // Loading this tile is very important, because a number of deeper,
      // higher-detail tiles are being rendered in its stead, so we want to load
      // it with high priority. However, if `ancestorMeetsSse` is set, then our
      // parent tile is in the exact same situation, and loading this tile with
      // high priority would compete with that one. We should prefer the parent
      // because it is closest to the actual desired LOD and because up the tree
      // there can only be fewer tiles that need loading.
      if (!ancestorMeetsSse) {
        addTileToLoadQueue(
            context,
            frameState,
            tile,
            TileLoadPriorityGroup::Urgent,
            tilePriority);
        queuedForLoad = true;
      }
      // Fall through to REFINE, but mark this tile as already meeting the
      // required SSE.
      ancestorMeetsSse = true;
    } else {
      // Render this tile and return without visiting children.
      // Only load this tile if it (not just an ancestor) meets the SSE.
      if (!ancestorMeetsSse) {
        addTileToLoadQueue(
            context,
            frameState,
            tile,
            TileLoadPriorityGroup::Normal,
            tilePriority);
      }
      return renderInnerTile(frameState, tile, tileSse, result);
    }
  }

  // Refine!
  queuedForLoad = loadAndRenderAdditiveRefinedTile(
                      context,
                      frameState,
                      tile,
                      result,
                      tilePriority,
                      tileSse,
                      queuedForLoad) ||
                  queuedForLoad;

  const size_t firstRenderedDescendantIndex =
      result.tilesToRenderThisFrame.size();
  TilesetViewGroup::LoadQueueCheckpoint loadQueueBeforeChildren =
      frameState.viewGroup.saveTileLoadQueueCheckpoint();

  TraversalDetails traversalDetails = visitVisibleChildrenNearToFar(
      context,
      frameState,
      depth,
      ancestorMeetsSse,
      tile,
      result);

  // Zero or more descendant tiles were added to the render list.
  // The traversalDetails tell us what happened while visiting the children.

  // Descendants will be kicked if any are not ready to render yet and none
  // were rendered last frame.
  bool kickDueToNonReadyDescendant = !traversalDetails.allAreRenderable &&
                                     !traversalDetails.anyWereRenderedLastFrame;

  // Descendants may also be kicked if this tile was rendered last frame and
  // has not finished fading in yet.
  const TileRenderContent* pRenderContent =
      tile.getContent().getRenderContent();
  bool kickDueToTileFadingIn =
      context.options.enableLodTransitionPeriod &&
      context.options.kickDescendantsWhileFadingIn &&
      lastFrameSelectionResult == TileSelectionState::Result::Rendered &&
      pRenderContent && pRenderContent->getLodTransitionFadePercentage() < 1.0f;

  // Only kick the descendants of this tile if it is renderable, or if we've
  // exceeded the loadingDescendantLimit. It's pointless to kick the descendants
  // of a tile that is not yet loaded, because it means we will still have a
  // hole, and quite possibly a bigger one.
  bool wantToKick = kickDueToNonReadyDescendant || kickDueToTileFadingIn;
  bool willKick = wantToKick && (traversalDetails.notYetRenderableCount >
                                     context.options.loadingDescendantLimit ||
                                 tile.isRenderable());

  if (willKick) {
    // Kick all descendants out of the render list and render this tile instead
    // Continue to load them though!
    queuedForLoad = kickDescendantsAndRenderTile(
        context,
        frameState,
        tile,
        result,
        traversalDetails,
        firstRenderedDescendantIndex,
        loadQueueBeforeChildren,
        queuedForLoad,
        tilePriority,
        tileSse);
  } else {
    if (tile.getRefine() != TileRefine::Add) {
      addCurrentTileToTilesFadingOutIfPreviouslyRendered(
          frameState.viewGroup,
          tile,
          result);
    }
    traversalState.currentState() =
        TileSelectionState(TileSelectionState::Result::Refined);
  }

  if (context.options.preloadAncestors && !queuedForLoad) {
    addTileToLoadQueue(
        context,
        frameState,
        tile,
        TileLoadPriorityGroup::Preload,
        tilePriority);
  }

  return traversalDetails;
}

// Visits a tile for possible rendering. When we call this function with a tile:
//   * It is not yet known whether the tile is visible.
//   * Its parent tile does _not_ meet the SSE (unless ancestorMeetsSse=true,
//   see comments below).
//   * The tile may or may not be renderable.
//   * The tile has not yet been added to a load queue.
TraversalDetails visitTileIfNeeded(
    const TileSelectionContext& context,
    const TilesetFrameState& frameState,
    uint32_t depth,
    bool ancestorMeetsSse,
    Tile& tile,
    ViewUpdateResult& result) {

  TilesetViewGroup::TraversalState& traversalState =
      frameState.viewGroup.getTraversalState();
  traversalState.beginNode(&tile);

  computeDistances(tile, frameState.frustums, context.scratchDistances);
  double tilePriority =
      computeTilePriority(tile, frameState.frustums, context.scratchDistances);

  if (frameState.tileStateUpdater) {
    frameState.tileStateUpdater(tile);
  }

  CullResult cullResult{};

  // Culling with children bounds will give us incorrect results with Add
  // refinement, but is a useful optimization for Replace refinement.
  bool cullWithChildrenBounds =
      tile.getRefine() == TileRefine::Replace && !tile.getChildren().empty();
  for (Tile& child : tile.getChildren()) {
    if (child.getUnconditionallyRefine()) {
      cullWithChildrenBounds = false;
      break;
    }
  }

  // TODO: add cullWithChildrenBounds to the tile excluder interface?
  for (const std::shared_ptr<ITileExcluder>& pExcluder :
       context.options.excluders) {
    if (pExcluder->shouldExclude(tile)) {
      cullResult.culled = true;
      cullResult.shouldVisit = false;
      break;
    }
  }

  // TODO: abstract culling stages into composable interface?
  frustumCull(context, frameState, tile, cullWithChildrenBounds, cullResult);
  fogCull(context, frameState, cullResult);

  if (!cullResult.shouldVisit && tile.getUnconditionallyRefine()) {
    // Unconditionally refined tiles must always be visited in forbidHoles
    // mode, because we need to load this tile's descendants before we can
    // render any of its siblings. An unconditionally refined root tile must be
    // visited as well, otherwise we won't load anything at all.
    if ((context.options.forbidHoles &&
         tile.getRefine() == TileRefine::Replace) ||
        tile.getParent() == nullptr) {
      cullResult.shouldVisit = true;
    }
  }

  if (!cullResult.shouldVisit) {
    addCurrentTileAndDescendantsToTilesFadingOutIfPreviouslyRendered(
        frameState.viewGroup,
        tile,
        result);

    frameState.viewGroup.getTraversalState().currentState() =
        TileSelectionState(TileSelectionState::Result::Culled);
    ++result.tilesCulled;

    TraversalDetails traversalDetails{};

    if (context.options.forbidHoles &&
        tile.getRefine() == TileRefine::Replace) {
      // In order to prevent holes, we need to load this tile and also not
      // render any siblings until it is ready. We don't actually need to
      // render it, though.
      addTileToLoadQueue(
          context,
          frameState,
          tile,
          TileLoadPriorityGroup::Normal,
          tilePriority);
      traversalDetails = createTraversalDetailsForSingleTile(frameState, tile);
    } else if (context.options.preloadSiblings) {
      addTileToLoadQueue(
          context,
          frameState,
          tile,
          TileLoadPriorityGroup::Preload,
          tilePriority);
    }

    traversalState.finishNode(&tile);
    return traversalDetails;
  }

  if (cullResult.culled) {
    ++result.culledTilesVisited;
  }

  double tileSse = computeSse(context, frameState, tile);
  bool meetsSse = meetsSseThreshold(context, tileSse, cullResult.culled);

  TraversalDetails details = visitTile(
      context,
      frameState,
      depth,
      meetsSse,
      ancestorMeetsSse,
      tile,
      tilePriority,
      tileSse,
      result);

  traversalState.finishNode(&tile);
  return details;
}

} // anonymous namespace

} // namespace Cesium3DTilesSelection
