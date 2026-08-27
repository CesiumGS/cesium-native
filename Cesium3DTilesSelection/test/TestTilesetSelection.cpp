// Unit tests for the selectTiles() free function.

#include "SimplePrepareRendererResource.h"

#include <Cesium3DTilesSelection/EllipsoidTilesetLoader.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/TileLoadResult.h>
#include <Cesium3DTilesSelection/TileRefine.h>
#include <Cesium3DTilesSelection/Tileset.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/TilesetFrameState.h>
#include <Cesium3DTilesSelection/TilesetOptions.h>
#include <Cesium3DTilesSelection/TilesetSelection.h>
#include <Cesium3DTilesSelection/TilesetViewGroup.h>
#include <Cesium3DTilesSelection/ViewState.h>
#include <Cesium3DTilesSelection/ViewUpdateResult.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>
#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/trigonometric.hpp>

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace Cesium3DTilesSelection;
using namespace CesiumAsync;
using namespace CesiumGeospatial;
using namespace CesiumNativeTests;
using namespace CesiumUtility;

namespace {

// Minimal in-memory loader — always reports tiles as empty, no children.
class EmptyLoader : public TilesetContentLoader {
public:
  Future<TileLoadResult> loadTileContent(const TileLoadInput& input) override {
    return input.asyncSystem.createResolvedFuture(TileLoadResult{
        .contentKind = TileEmptyContent(),
        .glTFUpAxis = CesiumGeometry::Axis::Z,
        .updatedBoundingVolume = std::nullopt,
        .updatedContentBoundingVolume = std::nullopt,
        .rasterOverlayDetails = std::nullopt,
        .pAssetAccessor = input.pAssetAccessor,
        .pCompletedRequest = nullptr,
        .tileInitializer = {},
        .state = TileLoadResultState::Success,
        .ellipsoid = CesiumGeospatial::Ellipsoid::WGS84});
  }

  TileChildrenResult createTileChildren(
      const Tile& /* tile */,
      const Ellipsoid& /* ellipsoid */) override {
    return TileChildrenResult{{}, TileLoadResultState::Success};
  }
};

// Build a minimal TilesetExternals with no asset accessor.
TilesetExternals makeExternals() {
  return TilesetExternals{
      .pAssetAccessor = std::make_shared<SimpleAssetAccessor>(
          std::map<std::string, std::shared_ptr<SimpleAssetRequest>>{}),
      .pPrepareRendererResources =
          std::make_shared<SimplePrepareRendererResource>(),
      .asyncSystem = AsyncSystem(std::make_shared<SimpleTaskProcessor>()),
      .pCreditSystem = std::make_shared<CreditSystem>(),
  };
}

// Build a ViewState looking at the root of a tileset from far away so SSE
// is low and the root tile meets the threshold.
ViewState makeFarViewState() {
  const Ellipsoid& ellipsoid = Ellipsoid::WGS84;
  // Camera above 0N 0E at 15 000 km — far enough that a global-scale tile
  // easily meets any reasonable SSE threshold.
  Cartographic camCarto{0.0, 0.0, 15'000'000.0};
  glm::dvec3 camPos = ellipsoid.cartographicToCartesian(camCarto);
  glm::dvec3 camDir = glm::normalize(-camPos);
  glm::dvec3 camUp{0.0, 0.0, 1.0};
  glm::dvec2 viewport{1280.0, 720.0};
  double hFov = Math::degreesToRadians(60.0);
  double vFov = 2.0 * std::atan(std::tan(hFov * 0.5) / (1280.0 / 720.0));
  return ViewState(camPos, camDir, camUp, viewport, hFov, vFov, ellipsoid);
}

} // namespace

TEST_CASE("selectTiles is callable as a free function") {
  // Verify that selectTiles() can be invoked directly without going through
  // Tileset::updateViewGroup.

  TilesetExternals externals = makeExternals();
  TilesetOptions options;
  options.maximumScreenSpaceError = 16.0;

  // Use EllipsoidTilesetLoader so we get a loaded root tile without I/O.
  auto pTileset = EllipsoidTilesetLoader::createTileset(externals, options);
  REQUIRE(pTileset != nullptr);

  // Let the tileset initialise (root tile creation, etc.)
  externals.asyncSystem.dispatchMainThreadTasks();
  pTileset->loadTiles();
  externals.asyncSystem.dispatchMainThreadTasks();
  pTileset->loadTiles();

  Tile* pRoot = const_cast<Tile*>(pTileset->getRootTile());
  REQUIRE(pRoot != nullptr);

  ViewState viewState = makeFarViewState();
  std::vector<ViewState> frustums{viewState};

  std::vector<double> fogDensities(1, 0.0);
  TilesetViewGroup& viewGroup = pTileset->getDefaultViewGroup();

  TilesetFrameState frameState{
      viewGroup,
      frustums,
      std::move(fogDensities),
      // No tileStateUpdater needed — tiles are already loaded.
      {}};

  std::vector<double> scratchDistances;
  std::vector<const TileOcclusionRendererProxy*> scratchOcclusion;

  TileSelectionContext ctx{
      options,
      externals,
      scratchDistances,
      scratchOcclusion};

  viewGroup.startNewFrame(*pTileset, frameState);
  ViewUpdateResult result;
  selectTiles(ctx, frameState, *pRoot, result);
  viewGroup.finishFrame(*pTileset, frameState);

  // From far away the root tile (or its immediate children) should be
  // selected; there must be at least one tile to render.
  CHECK(result.tilesToRenderThisFrame.size() >= 1);
  // No tiles should have been kicked.
  CHECK(result.tilesKicked == 0);
}

TEST_CASE("selectTiles result matches updateViewGroup result") {
  // Run both the selectTiles() free function and Tileset::updateViewGroup on
  // the same tileset in the same frame configuration and verify they produce
  // identical render lists.

  TilesetExternals externals = makeExternals();
  TilesetOptions options;
  options.maximumScreenSpaceError = 16.0;

  auto pTileset = EllipsoidTilesetLoader::createTileset(externals, options);
  REQUIRE(pTileset != nullptr);

  // Warm up the tileset.
  ViewState viewState = makeFarViewState();
  for (int i = 0; i < 3; ++i) {
    externals.asyncSystem.dispatchMainThreadTasks();
    pTileset->updateViewGroup(pTileset->getDefaultViewGroup(), {viewState});
    externals.asyncSystem.dispatchMainThreadTasks();
    pTileset->loadTiles();
  }

  Tile* pRoot = const_cast<Tile*>(pTileset->getRootTile());
  REQUIRE(pRoot != nullptr);

  // Run via updateViewGroup (the established path)
  ViewUpdateResult referenceResult =
      pTileset->updateViewGroup(pTileset->getDefaultViewGroup(), {viewState});

  size_t referenceRenderCount = referenceResult.tilesToRenderThisFrame.size();
  uint32_t referenceVisited = referenceResult.tilesVisited;

  // Run via selectTiles() directly (the new path)
  std::vector<double> fogDensities(1, 0.0);
  TilesetViewGroup& viewGroup = pTileset->getDefaultViewGroup();

  std::vector<ViewState> frustums{viewState};
  TilesetFrameState frameState{
      viewGroup,
      frustums,
      std::move(fogDensities),
      // No tileStateUpdater needed — tiles are already loaded.
      {}};

  std::vector<double> scratchDistances;
  std::vector<const TileOcclusionRendererProxy*> scratchOcclusion;

  TileSelectionContext ctx{
      options,
      externals,
      scratchDistances,
      scratchOcclusion};

  viewGroup.startNewFrame(*pTileset, frameState);
  ViewUpdateResult freeResult;
  selectTiles(ctx, frameState, *pRoot, freeResult);
  viewGroup.finishFrame(*pTileset, frameState);

  // Tile counts must agree between the two paths.
  CHECK(freeResult.tilesToRenderThisFrame.size() == referenceRenderCount);
  CHECK(freeResult.tilesVisited == referenceVisited);
}

namespace {

const Cartographic DefaultCameraPosition{0.0, 0.0, 100'000.0};

ViewState makeViewState(
    double horizontalFieldOfViewDegrees,
    bool lookAwayFromGlobe = false,
    const Cartographic& cameraPosition = DefaultCameraPosition) {
  const Ellipsoid& ellipsoid = Ellipsoid::WGS84;
  const glm::dvec3 position =
      ellipsoid.cartographicToCartesian(cameraPosition);
  const glm::dvec3 direction =
      glm::normalize(lookAwayFromGlobe ? position : -position);
  const glm::dvec3 up{0.0, 0.0, 1.0};
  const glm::dvec2 viewport{1920.0, 1080.0};

  const double horizontalFieldOfView =
      Math::degreesToRadians(horizontalFieldOfViewDegrees);
  const double verticalFieldOfView =
      2.0 * std::atan(
                std::tan(horizontalFieldOfView * 0.5) /
                (viewport.x / viewport.y));

  return ViewState(
      position,
      direction,
      up,
      viewport,
      horizontalFieldOfView,
      verticalFieldOfView,
      ellipsoid);
}

// Reproduces what selectTiles computes for one view, so a recorded error can be
// attributed to the view it came from.
double screenSpaceErrorFor(const ViewState& view, const Tile& tile) {
  const double distance = glm::sqrt(glm::max(
      view.computeDistanceSquaredToBoundingVolume(tile.getBoundingVolume()),
      0.0));
  return view.computeScreenSpaceError(tile.getGeometricError(), distance);
}

// EllipsoidTilesetLoader creates children on demand, so refinement descends one
// level per frame. Callers assert the depth reached, so a fixture that stops
// descending fails loudly instead of silently weakening the test.
ViewUpdateResult selectAfterLoading(
    const std::vector<ViewState>& frustums,
    const TilesetOptions& options) {
  TilesetExternals externals = makeExternals();
  auto pTileset = EllipsoidTilesetLoader::createTileset(externals, options);
  REQUIRE(pTileset != nullptr);

  ViewUpdateResult result;
  for (int i = 0; i < 16; ++i) {
    externals.asyncSystem.dispatchMainThreadTasks();
    result =
        pTileset->updateViewGroup(pTileset->getDefaultViewGroup(), frustums);
    externals.asyncSystem.dispatchMainThreadTasks();
    pTileset->loadTiles();
  }
  return result;
}

// Every recorded error must be one of the two views' own, and each view must
// drive at least one tile: if one view drives all of them, the other's
// visibility is being ignored.
void checkErrorsAreAttributableToBothViews(
    const ViewUpdateResult& result,
    const ViewState& first,
    const ViewState& second) {
  REQUIRE(
      result.tileScreenSpaceErrorThisFrame.size() ==
      result.tilesToRenderThisFrame.size());
  REQUIRE(result.maxDepthVisited > 4);

  size_t firstDriven = 0;
  size_t secondDriven = 0;
  for (size_t i = 0; i < result.tilesToRenderThisFrame.size(); ++i) {
    const Tile& tile = *result.tilesToRenderThisFrame[i];
    const double firstSse = screenSpaceErrorFor(first, tile);
    const double secondSse = screenSpaceErrorFor(second, tile);
    if (firstSse == secondSse) {
      continue;
    }

    const double recorded = result.tileScreenSpaceErrorThisFrame[i];
    CHECK((recorded == firstSse || recorded == secondSse));
    firstDriven += recorded == firstSse;
    secondDriven += recorded == secondSse;
  }

  REQUIRE(firstDriven > 0);
  REQUIRE(secondDriven > 0);
}

} // namespace

TEST_CASE("A view cannot drive refinement of tiles it can't see") {
  TilesetOptions options;
  options.maximumScreenSpaceError = 16.0;
  options.renderTilesUnderCamera = false;

  const ViewState wide = makeViewState(40.0);
  const ViewState narrow = makeViewState(1.0);

  checkErrorsAreAttributableToBothViews(
      selectAfterLoading({wide, narrow}, options),
      wide,
      narrow);
}
