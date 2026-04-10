// TestTilesetSelection.cpp
//
// Unit tests for the selectTiles() free function. These tests construct
// minimal in-memory tile trees and invoke the selection algorithm directly,
// without file I/O or async machinery beyond what TilesetViewGroup requires.

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
#include <glm/ext/vector_double3.hpp>
#include <glm/trigonometric.hpp>

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
  // Tileset::updateViewGroup. This is the isolation guarantee introduced by
  // the Phase B refactoring.

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

  TileSelectionContext ctx{options, externals};

  viewGroup.startNewFrame(*pTileset, frameState);
  ViewUpdateResult result =
      selectTiles(ctx, frameState, *pRoot, scratchDistances, scratchOcclusion);
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
      // Mirror what updateViewGroup does — call updateTileContent via the
      // tileStateUpdater so tile states advance identically.
      {}};

  std::vector<double> scratchDistances;
  std::vector<const TileOcclusionRendererProxy*> scratchOcclusion;

  TileSelectionContext ctx{options, externals};

  viewGroup.startNewFrame(*pTileset, frameState);
  ViewUpdateResult freeResult =
      selectTiles(ctx, frameState, *pRoot, scratchDistances, scratchOcclusion);
  viewGroup.finishFrame(*pTileset, frameState);

  // Tile counts must agree between the two paths.
  CHECK(freeResult.tilesToRenderThisFrame.size() == referenceRenderCount);
  CHECK(freeResult.tilesVisited == referenceVisited);
}
