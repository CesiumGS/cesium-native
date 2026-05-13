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
  bool allAreRenderable = true;
  bool anyWereRenderedLastFrame = false;
  uint32_t notYetRenderableCount = 0;
};

struct CullResult {
  bool shouldVisit = true;
  bool culled = false;
};

enum class VisitTileAction { Render, Refine };

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
  std::optional<GlobeRectangle> maybeRectangle =
      estimateGlobeRectangle(boundingVolume, ellipsoid);
  if (position && maybeRectangle) {
    return maybeRectangle->contains(position.value());
  }
  return false;
}

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
    const TilesetFrameState& frameState,
    double sse,
    bool culled) noexcept {
  return culled ? !context.options.enforceCulledScreenSpaceError ||
                      sse < context.options.culledScreenSpaceError
                : sse < context.options.maximumScreenSpaceError;
}

bool isLeaf(const Tile& tile) noexcept { return tile.getChildren().empty(); }

bool mustContinueRefiningToDeeperTiles(
    const Tile& tile,
    const TileSelectionState& lastFrameSelectionState) noexcept {
  const TileSelectionState::Result originalResult =
      lastFrameSelectionState.getOriginalResult();
  return originalResult == TileSelectionState::Result::Refined &&
         !tile.isRenderable();
}

TraversalDetails visitTile(
    const TileSelectionContext& context,
    const TilesetFrameState& frameState,
    uint32_t depth,
    bool meetsSse,
    bool ancestorMeetsSse,
    Tile& tile,
    double tilePriority,
    double tileSse,
    ViewUpdateResult& result);

TraversalDetails visitVisibleChildrenNearToFar(
    const TileSelectionContext& context,
    const TilesetFrameState& frameState,
    uint32_t depth,
    bool ancestorMeetsSse,
    Tile& tile,
    ViewUpdateResult& result);

// Frustum and fog culling

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
      return;
    }
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
    return;
  }

  cullResult.culled = true;
  if (context.options.enableFrustumCulling) {
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

  bool isFogCulled = true;
  for (size_t i = 0; i < frustums.size(); ++i) {
    if (isVisibleInFog(distances[i], fogDensities[i])) {
      isFogCulled = false;
      break;
    }
  }

  if (isFogCulled) {
    cullResult.culled = true;
    if (context.options.enableFogCulling) {
      cullResult.shouldVisit = false;
    }
  }
}

TileOcclusionState checkOcclusion(
    const TileSelectionContext& context,
    const TilesetFrameState& frameState,
    const Tile& tile) {
  const std::shared_ptr<TileOcclusionRendererProxyPool>& pOcclusionPool =
      context.externals.pTileOcclusionProxyPool;
  if (!pOcclusionPool) {
    return TileOcclusionState::NotOccluded;
  }

  const TileOcclusionRendererProxy* pOcclusion =
      pOcclusionPool->fetchOcclusionProxyForTile(tile);
  if (!pOcclusion) {
    return TileOcclusionState::NotOccluded;
  }

  switch (static_cast<TileOcclusionState>(pOcclusion->getOcclusionState())) {
  case TileOcclusionState::OcclusionUnavailable:
    return TileOcclusionState::OcclusionUnavailable;
  case TileOcclusionState::Occluded:
    return TileOcclusionState::Occluded;
  case TileOcclusionState::NotOccluded:
    if (tile.getChildren().empty()) {
      return TileOcclusionState::NotOccluded;
    }
    break;
  }

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
      return TileOcclusionState::NotOccluded;
    }
    childOcclusionProxies.push_back(pChildProxy);
  }

  for (const TileOcclusionRendererProxy* pChildProxy : childOcclusionProxies) {
    if (pChildProxy->getOcclusionState() == TileOcclusionState::NotOccluded) {
      return TileOcclusionState::NotOccluded;
    }
  }
  for (const TileOcclusionRendererProxy* pChildProxy : childOcclusionProxies) {
    if (pChildProxy->getOcclusionState() ==
        TileOcclusionState::OcclusionUnavailable) {
      return TileOcclusionState::OcclusionUnavailable;
    }
  }

  return TileOcclusionState::Occluded;
}

TraversalDetails createTraversalDetailsForSingleTile(
    const TileSelectionContext& context,
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
  return createTraversalDetailsForSingleTile(context, frameState, tile);
}

TraversalDetails renderInnerTile(
    const TileSelectionContext& context,
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
  return createTraversalDetailsForSingleTile(context, frameState, tile);
}

bool loadAndRenderAdditiveRefinedTile(
    const TileSelectionContext& context,
    const TilesetFrameState& frameState,
    Tile& tile,
    ViewUpdateResult& result,
    double tilePriority,
    double tileSse,
    bool queuedForLoad) {
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

  TilesetViewGroup::TraversalState& traversalState =
      frameState.viewGroup.getTraversalState();

  traversalState.forEachCurrentDescendant(
      [](const Tile::Pointer& /*pTile*/, TileSelectionState& selectionState) {
        selectionState.kick();
      });

  traversalState.forEachPreviousDescendant(
      [&result](
          const Tile::Pointer& pTile,
          const TileSelectionState& previousState) {
        addToTilesFadingOutIfPreviouslyRendered(
            previousState.getResult(),
            *pTile,
            result);
      });

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

TraversalDetails visitTile(
    const TileSelectionContext& context,
    const TilesetFrameState& frameState,
    uint32_t depth,
    bool meetsSse,
    bool ancestorMeetsSse,
    Tile& tile,
    double tilePriority,
    double tileSse,
    ViewUpdateResult& result) {

  TilesetViewGroup::TraversalState& traversalState =
      frameState.viewGroup.getTraversalState();

  ++result.tilesVisited;
  result.maxDepthVisited = glm::max(result.maxDepthVisited, depth);

  if (isLeaf(tile)) {
    return renderLeaf(context, frameState, tile, tilePriority, tileSse, result);
  }

  const bool unconditionallyRefine = tile.getUnconditionallyRefine();
  const bool refineForSse = !meetsSse && !ancestorMeetsSse;

  VisitTileAction action = (unconditionallyRefine || refineForSse)
                               ? VisitTileAction::Refine
                               : VisitTileAction::Render;

  TileSelectionState lastFrameSelectionState =
      getPreviousState(frameState.viewGroup, tile);
  TileSelectionState::Result lastFrameSelectionResult =
      lastFrameSelectionState.getResult();

  bool tileLastRefined =
      lastFrameSelectionResult == TileSelectionState::Result::Refined;
  bool childLastRefined = false;
  traversalState.forEachPreviousChild(
      [&](const Tile::Pointer& /*pTile*/, const TileSelectionState& state) {
        if (state.getResult() == TileSelectionState::Result::Refined) {
          childLastRefined = true;
        }
      });

  bool shouldCheckOcclusion = context.options.enableOcclusionCulling &&
                              action == VisitTileAction::Refine &&
                              !unconditionallyRefine &&
                              (!tileLastRefined || !childLastRefined);

  if (shouldCheckOcclusion) {
    TileOcclusionState occlusion = checkOcclusion(context, frameState, tile);
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
    bool mustRefine =
        mustContinueRefiningToDeeperTiles(tile, lastFrameSelectionState);
    if (mustRefine) {
      action = VisitTileAction::Refine;
      if (!ancestorMeetsSse) {
        addTileToLoadQueue(
            context,
            frameState,
            tile,
            TileLoadPriorityGroup::Urgent,
            tilePriority);
        queuedForLoad = true;
      }
      ancestorMeetsSse = true;
    } else {
      if (!ancestorMeetsSse) {
        addTileToLoadQueue(
            context,
            frameState,
            tile,
            TileLoadPriorityGroup::Normal,
            tilePriority);
      }
      return renderInnerTile(context, frameState, tile, tileSse, result);
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

  const TileRenderContent* pRenderContent =
      tile.getContent().getRenderContent();
  bool kickDueToNonReadyDescendant = !traversalDetails.allAreRenderable &&
                                     !traversalDetails.anyWereRenderedLastFrame;
  bool kickDueToTileFadingIn =
      context.options.enableLodTransitionPeriod &&
      context.options.kickDescendantsWhileFadingIn &&
      lastFrameSelectionResult == TileSelectionState::Result::Rendered &&
      pRenderContent && pRenderContent->getLodTransitionFadePercentage() < 1.0f;

  bool wantToKick = kickDueToNonReadyDescendant || kickDueToTileFadingIn;
  bool willKick = wantToKick && (traversalDetails.notYetRenderableCount >
                                     context.options.loadingDescendantLimit ||
                                 tile.isRenderable());

  if (willKick) {
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

  bool cullWithChildrenBounds =
      tile.getRefine() == TileRefine::Replace && !tile.getChildren().empty();
  for (Tile& child : tile.getChildren()) {
    if (child.getUnconditionallyRefine()) {
      cullWithChildrenBounds = false;
      break;
    }
  }

  for (const std::shared_ptr<ITileExcluder>& pExcluder :
       context.options.excluders) {
    if (pExcluder->shouldExclude(tile)) {
      cullResult.culled = true;
      cullResult.shouldVisit = false;
      break;
    }
  }

  frustumCull(context, frameState, tile, cullWithChildrenBounds, cullResult);
  fogCull(context, frameState, cullResult);

  if (!cullResult.shouldVisit && tile.getUnconditionallyRefine()) {
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
      addTileToLoadQueue(
          context,
          frameState,
          tile,
          TileLoadPriorityGroup::Normal,
          tilePriority);
      traversalDetails =
          createTraversalDetailsForSingleTile(context, frameState, tile);
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
  bool meetsSse =
      meetsSseThreshold(context, frameState, tileSse, cullResult.culled);

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
