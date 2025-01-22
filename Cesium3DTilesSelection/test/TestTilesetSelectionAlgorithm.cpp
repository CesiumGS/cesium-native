#include "SimplePrepareRendererResource.h"

#include <Cesium3DTiles/GroupMetadata.h>
#include <Cesium3DTiles/MetadataQuery.h>
#include <Cesium3DTiles/Schema.h>
#include <Cesium3DTilesContent/registerAllTileContentTypes.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileContent.h>
#include <Cesium3DTilesSelection/TileLoadResult.h>
#include <Cesium3DTilesSelection/Tileset.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/ViewState.h>
#include <Cesium3DTilesSelection/ViewUpdateResult.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/Promise.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/S2CellBoundingVolume.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumNativeTests/SimpleAssetResponse.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/readFile.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>
#include <glm/exponential.hpp>
#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace CesiumAsync;
using namespace Cesium3DTilesSelection;
using namespace CesiumGeospatial;
using namespace CesiumUtility;
using namespace CesiumNativeTests;

static bool doesTileMeetSSE(
    const ViewState& viewState,
    const Tile& tile,
    const Tileset& tileset) {
  double distance = glm::sqrt(viewState.computeDistanceSquaredToBoundingVolume(
      tile.getBoundingVolume()));
  double sse =
      viewState.computeScreenSpaceError(tile.getGeometricError(), distance);
  return sse < tileset.getOptions().maximumScreenSpaceError;
}

static void initializeTileset(Tileset& tileset) {
  // create a random view state so that we can able to load the tileset first
  const Ellipsoid& ellipsoid = Ellipsoid::WGS84;
  Cartographic viewPositionCartographic{
      Math::degreesToRadians(118.0),
      Math::degreesToRadians(32.0),
      200.0};
  Cartographic viewFocusCartographic{
      viewPositionCartographic.longitude + Math::degreesToRadians(0.5),
      viewPositionCartographic.latitude + Math::degreesToRadians(0.5),
      0.0};
  glm::dvec3 viewPosition =
      ellipsoid.cartographicToCartesian(viewPositionCartographic);
  glm::dvec3 viewFocus =
      ellipsoid.cartographicToCartesian(viewFocusCartographic);
  glm::dvec3 viewUp{0.0, 0.0, 1.0};
  glm::dvec2 viewPortSize{500.0, 500.0};
  double aspectRatio = viewPortSize.x / viewPortSize.y;
  double horizontalFieldOfView = Math::degreesToRadians(60.0);
  double verticalFieldOfView =
      std::atan(std::tan(horizontalFieldOfView * 0.5) / aspectRatio) * 2.0;
  ViewState viewState = ViewState::create(
      viewPosition,
      glm::normalize(viewFocus - viewPosition),
      viewUp,
      viewPortSize,
      horizontalFieldOfView,
      verticalFieldOfView,
      Ellipsoid::WGS84);

  tileset.updateView({viewState});
}

static ViewState zoomToTile(const Tile& tile) {
  const BoundingRegion* region =
      std::get_if<BoundingRegion>(&tile.getBoundingVolume());
  REQUIRE(region != nullptr);

  const GlobeRectangle& rectangle = region->getRectangle();
  double maxHeight = region->getMaximumHeight();
  Cartographic center = rectangle.computeCenter();
  Cartographic corner = rectangle.getNorthwest();
  corner.height = maxHeight;

  const Ellipsoid& ellipsoid = Ellipsoid::WGS84;
  glm::dvec3 viewPosition = ellipsoid.cartographicToCartesian(corner);
  glm::dvec3 viewFocus = ellipsoid.cartographicToCartesian(center);
  glm::dvec3 viewUp{0.0, 0.0, 1.0};
  glm::dvec2 viewPortSize{500.0, 500.0};
  double aspectRatio = viewPortSize.x / viewPortSize.y;
  double horizontalFieldOfView = Math::degreesToRadians(60.0);
  double verticalFieldOfView =
      std::atan(std::tan(horizontalFieldOfView * 0.5) / aspectRatio) * 2.0;
  return ViewState::create(
      viewPosition,
      glm::normalize(viewFocus - viewPosition),
      viewUp,
      viewPortSize,
      horizontalFieldOfView,
      verticalFieldOfView,
      Ellipsoid::WGS84);
}

static ViewState zoomToTileset(const Tileset& tileset) {
  const Tile* root = tileset.getRootTile();
  REQUIRE(root != nullptr);

  return zoomToTile(*root);
}

TEST_CASE("Test replace refinement for render") {
  Cesium3DTilesContent::registerAllTileContentTypes();

  // initialize REPLACE tileset
  //
  //				   parent.b3dm
  //
  // ll.b3dm		lr.b3dm		ul.b3dm		ur.b3dm
  //
  // ll_ll.b3dm
  //
  std::filesystem::path testDataPath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testDataPath = testDataPath / "ReplaceTileset";
  std::vector<std::string> files{
      "tileset.json",
      "parent.b3dm",
      "ll.b3dm",
      "lr.b3dm",
      "ul.b3dm",
      "ur.b3dm",
      "ll_ll.b3dm",
  };

  std::map<std::string, std::shared_ptr<SimpleAssetRequest>>
      mockCompletedRequests;
  for (const auto& file : files) {
    std::unique_ptr<SimpleAssetResponse> mockCompletedResponse =
        std::make_unique<SimpleAssetResponse>(
            static_cast<uint16_t>(200),
            "doesn't matter",
            CesiumAsync::HttpHeaders{},
            readFile(testDataPath / file));
    mockCompletedRequests.insert(
        {file,
         std::make_shared<SimpleAssetRequest>(
             "GET",
             file,
             CesiumAsync::HttpHeaders{},
             std::move(mockCompletedResponse))});
  }

  std::shared_ptr<SimpleAssetAccessor> mockAssetAccessor =
      std::make_shared<SimpleAssetAccessor>(std::move(mockCompletedRequests));
  TilesetExternals tilesetExternals{
      mockAssetAccessor,
      std::make_shared<SimplePrepareRendererResource>(),
      AsyncSystem(std::make_shared<SimpleTaskProcessor>()),
      nullptr};

  // create tileset and call updateView() to give it a chance to load
  Tileset tileset(tilesetExternals, "tileset.json");
  initializeTileset(tileset);

  // check the tiles status
  const Tile* pTilesetJson = tileset.getRootTile();
  REQUIRE(pTilesetJson);
  REQUIRE(pTilesetJson->getChildren().size() == 1);

  const Tile* root = &pTilesetJson->getChildren()[0];

  REQUIRE(root->getState() == TileLoadState::ContentLoading);
  for (const auto& child : root->getChildren()) {
    REQUIRE(child.getState() == TileLoadState::Unloaded);
  }

  SUBCASE("No refinement happen when tile meet SSE") {
    // Zoom to tileset. Expect the root will not meet sse in this configure
    ViewState viewState = zoomToTileset(tileset);

    // Zoom out from the tileset a little bit to make sure the root meet sse
    glm::dvec3 zoomOutPosition =
        viewState.getPosition() - viewState.getDirection() * 2500.0;
    ViewState zoomOutViewState = ViewState::create(
        zoomOutPosition,
        viewState.getDirection(),
        viewState.getUp(),
        viewState.getViewportSize(),
        viewState.getHorizontalFieldOfView(),
        viewState.getVerticalFieldOfView(),
        Ellipsoid::WGS84);

    // Check 1st and 2nd frame. Root should meet sse and render. No transitions
    // are expected here
    for (int frame = 0; frame < 2; ++frame) {
      ViewUpdateResult result = tileset.updateView({zoomOutViewState});

      // Check tile state. Ensure root meet sse
      REQUIRE(root->getState() == TileLoadState::Done);
      REQUIRE(doesTileMeetSSE(zoomOutViewState, *root, tileset));
      for (const auto& child : root->getChildren()) {
        REQUIRE(child.getState() == TileLoadState::Unloaded);
      }

      // check result
      REQUIRE(result.tilesToRenderThisFrame.size() == 1);
      REQUIRE(result.tilesToRenderThisFrame.front() == root);

      REQUIRE(result.tilesFadingOut.size() == 0);

      REQUIRE(result.tilesVisited == 2);
      REQUIRE(result.workerThreadTileLoadQueueLength == 0);
      REQUIRE(result.mainThreadTileLoadQueueLength == 0);
      REQUIRE(result.tilesCulled == 0);
      REQUIRE(result.culledTilesVisited == 0);

      // check children state are still unloaded
      for (const auto& child : root->getChildren()) {
        REQUIRE(child.getState() == TileLoadState::Unloaded);
      }
    }
  }

  SUBCASE("Root doesn't meet sse but has to be rendered because children "
          "cannot be rendered") {
    // we should forbid hole first to let the checks below happen
    tileset.getOptions().forbidHoles = true;

    SUBCASE("Children cannot be rendered because of no response") {
      // remove one of children completed response to mock network error
      mockAssetAccessor->mockCompletedRequests["ll.b3dm"]->pResponse = nullptr;
      mockAssetAccessor->mockCompletedRequests["lr.b3dm"]->pResponse = nullptr;
      mockAssetAccessor->mockCompletedRequests["ul.b3dm"]->pResponse = nullptr;
      mockAssetAccessor->mockCompletedRequests["ur.b3dm"]->pResponse = nullptr;
    }

    SUBCASE("Children cannot be rendered because response has an failed status "
            "code") {
      // remove one of children completed response to mock network error
      mockAssetAccessor->mockCompletedRequests["ll.b3dm"]
          ->pResponse->mockStatusCode = 404;
      mockAssetAccessor->mockCompletedRequests["lr.b3dm"]
          ->pResponse->mockStatusCode = 404;
      mockAssetAccessor->mockCompletedRequests["ul.b3dm"]
          ->pResponse->mockStatusCode = 404;
      mockAssetAccessor->mockCompletedRequests["ur.b3dm"]
          ->pResponse->mockStatusCode = 404;
    }

    ViewState viewState = zoomToTileset(tileset);

    // 1st frame. Root doesn't meet sse, so it goes to children. But because
    // children haven't started loading, root should be rendered.
    {
      ViewUpdateResult result = tileset.updateView({viewState});

      // Check tile state. Ensure root doesn't meet sse, but children does.
      // Children begin loading as well
      REQUIRE(root->getState() == TileLoadState::Done);
      REQUIRE(!doesTileMeetSSE(viewState, *root, tileset));
      for (const auto& child : root->getChildren()) {
        REQUIRE(child.getState() == TileLoadState::ContentLoading);
        REQUIRE(doesTileMeetSSE(viewState, child, tileset));
      }

      // check result
      REQUIRE(result.tilesToRenderThisFrame.size() == 1);
      REQUIRE(result.tilesToRenderThisFrame.front() == root);

      REQUIRE(result.tilesFadingOut.size() == 0);

      REQUIRE(result.tilesVisited == 6);
      REQUIRE(result.workerThreadTileLoadQueueLength == 4);
      REQUIRE(result.tilesCulled == 0);
      REQUIRE(result.culledTilesVisited == 0);
    }

    // 2nd frame. Because children receive failed response, so they will be
    // rendered as empty tiles.
    {
      ViewUpdateResult result = tileset.updateView({viewState});

      // Check tile state. Ensure root doesn't meet sse, but children does
      REQUIRE(root->getState() == TileLoadState::Done);
      REQUIRE(!doesTileMeetSSE(viewState, *root, tileset));
      for (const auto& child : root->getChildren()) {
        REQUIRE(child.getState() == TileLoadState::Failed);
        REQUIRE(doesTileMeetSSE(viewState, child, tileset));
      }

      // check result
      REQUIRE(result.tilesToRenderThisFrame.size() == 4);

      REQUIRE(result.tilesFadingOut.size() == 1);

      REQUIRE(result.tilesVisited == 6);
      REQUIRE(result.workerThreadTileLoadQueueLength == 0);
      REQUIRE(result.tilesCulled == 0);
      REQUIRE(result.culledTilesVisited == 0);
    }
  }

  SUBCASE("Parent meets sse but not renderable") {
    // Zoom to tileset. Expect the root will not meet sse in this configure
    ViewState viewState = zoomToTileset(tileset);
    glm::dvec3 zoomInPosition =
        viewState.getPosition() + viewState.getDirection() * 200.0;
    ViewState zoomInViewState = ViewState::create(
        zoomInPosition,
        viewState.getDirection(),
        viewState.getUp(),
        viewState.getViewportSize(),
        viewState.getHorizontalFieldOfView(),
        viewState.getVerticalFieldOfView(),
        Ellipsoid::WGS84);

    // remove the ll.b3dm (one of the root's children) request to replicate
    // network failure
    mockAssetAccessor->mockCompletedRequests["ll.b3dm"]->pResponse = nullptr;

    // 1st frame. Root doesn't meet sse, but none of the children finish
    // loading. So we will render root
    {
      ViewUpdateResult result = tileset.updateView({zoomInViewState});

      // check tiles status. All the children should have loading status
      REQUIRE(root->getState() == TileLoadState::Done);
      REQUIRE(!doesTileMeetSSE(zoomInViewState, *root, tileset));
      for (const auto& child : root->getChildren()) {
        REQUIRE(child.getState() == TileLoadState::ContentLoading);
      }

      const Tile& ll = root->getChildren().front();
      REQUIRE(!doesTileMeetSSE(zoomInViewState, ll, tileset));

      const Tile& ll_ll = ll.getChildren().front();
      REQUIRE(ll_ll.getState() == TileLoadState::ContentLoading);
      REQUIRE(doesTileMeetSSE(zoomInViewState, ll_ll, tileset));

      // check result
      REQUIRE(result.tilesToRenderThisFrame.size() == 1);
      REQUIRE(result.tilesToRenderThisFrame.front() == root);

      REQUIRE(result.tilesFadingOut.size() == 0);

      REQUIRE(result.tilesVisited == 7);
      REQUIRE(result.workerThreadTileLoadQueueLength == 5);
      REQUIRE(result.tilesCulled == 0);
      REQUIRE(result.culledTilesVisited == 0);
    }

    // 2nd frame. All the children finish loading, so they are ready to be
    // rendered (except ll.b3dm tile since it doesn't meet sse)
    {
      ViewUpdateResult result = tileset.updateView({zoomInViewState});

      // check tiles status. All the children should have loading status
      REQUIRE(root->getState() == TileLoadState::Done);
      REQUIRE(!doesTileMeetSSE(zoomInViewState, *root, tileset));

      // the first child of root isn't rendered because it doesn't meet sse. It
      // will be refined to its child which is ready to be rendered
      const Tile& ll = root->getChildren().front();
      REQUIRE(ll.getState() == TileLoadState::Failed);
      REQUIRE(!doesTileMeetSSE(zoomInViewState, ll, tileset));

      const Tile& ll_ll = ll.getChildren().front();
      REQUIRE(ll_ll.getState() == TileLoadState::Done);
      REQUIRE(doesTileMeetSSE(zoomInViewState, ll_ll, tileset));

      // the rest of the root's children are ready to be rendered since they all
      // meet sse
      for (size_t i = 1; i < root->getChildren().size(); ++i) {
        const Tile& child = root->getChildren()[i];
        REQUIRE(child.getState() == TileLoadState::Done);
        REQUIRE(doesTileMeetSSE(zoomInViewState, child, tileset));
      }

      // check result
      REQUIRE(result.tilesToRenderThisFrame.size() == 4);
      REQUIRE(result.tilesToRenderThisFrame[0] == &ll_ll);
      REQUIRE(result.tilesToRenderThisFrame[1] == &root->getChildren()[1]);
      REQUIRE(result.tilesToRenderThisFrame[2] == &root->getChildren()[2]);
      REQUIRE(result.tilesToRenderThisFrame[3] == &root->getChildren()[3]);

      REQUIRE(result.tilesFadingOut.size() == 1);

      REQUIRE(result.tilesVisited == 7);
      REQUIRE(result.workerThreadTileLoadQueueLength == 0);
      REQUIRE(result.tilesCulled == 0);
      REQUIRE(result.culledTilesVisited == 0);
    }

    // 3d frame. Zoom out the camera so that ll.b3dm tile will meet sse.
    // However, since its content is failed to load and in the last frame it is
    // refined, its child will be rendered instead to prevent loss of detail
    {
      glm::dvec3 zoomOutPosition =
          viewState.getPosition() - viewState.getDirection() * 100.0;
      ViewState zoomOutViewState = ViewState::create(
          zoomOutPosition,
          viewState.getDirection(),
          viewState.getUp(),
          viewState.getViewportSize(),
          viewState.getHorizontalFieldOfView(),
          viewState.getVerticalFieldOfView(),
          Ellipsoid::WGS84);

      ViewUpdateResult result = tileset.updateView({zoomOutViewState});

      // check tiles status. All the children should have loading status
      REQUIRE(root->getState() == TileLoadState::Done);
      REQUIRE(!doesTileMeetSSE(zoomOutViewState, *root, tileset));

      // The first child of root isn't rendered because it is failed to load
      // even though it meets sse. We will still render it even it's just an
      // empty hole
      const Tile& ll = root->getChildren().front();
      REQUIRE(ll.getState() == TileLoadState::Failed);
      REQUIRE(doesTileMeetSSE(zoomOutViewState, ll, tileset));

      const Tile& ll_ll = ll.getChildren().front();
      REQUIRE(ll_ll.getState() == TileLoadState::Done);

      // the rest of the root's children are ready to be rendered since they all
      // meet sse
      for (size_t i = 1; i < root->getChildren().size(); ++i) {
        const Tile& child = root->getChildren()[i];
        REQUIRE(child.getState() == TileLoadState::Done);
        REQUIRE(doesTileMeetSSE(zoomOutViewState, child, tileset));
      }

      // check result
      REQUIRE(result.tilesToRenderThisFrame.size() == 4);
      REQUIRE(result.tilesToRenderThisFrame[0] == &ll);
      REQUIRE(result.tilesToRenderThisFrame[1] == &root->getChildren()[1]);
      REQUIRE(result.tilesToRenderThisFrame[2] == &root->getChildren()[2]);
      REQUIRE(result.tilesToRenderThisFrame[3] == &root->getChildren()[3]);

      REQUIRE(result.tilesFadingOut.size() == 1);

      REQUIRE(result.tilesVisited == 6);
      REQUIRE(result.workerThreadTileLoadQueueLength == 0);
      REQUIRE(result.tilesCulled == 0);
      REQUIRE(result.culledTilesVisited == 0);
    }
  }

  SUBCASE("Child should be chosen when parent doesn't meet SSE") {
    ViewState viewState = zoomToTileset(tileset);

    // 1st frame. Root doesn't meet sse and children does. However, because
    // none of the children are loaded, root will be rendered instead and
    // children transition from unloaded to loading in the mean time
    {
      ViewUpdateResult result = tileset.updateView({viewState});

      // Check tile state. Ensure root doesn't meet sse, but children does
      REQUIRE(root->getState() == TileLoadState::Done);
      REQUIRE(!doesTileMeetSSE(viewState, *root, tileset));
      for (const auto& child : root->getChildren()) {
        REQUIRE(child.getState() == TileLoadState::ContentLoading);
        REQUIRE(doesTileMeetSSE(viewState, child, tileset));
      }

      // check result
      REQUIRE(result.tilesToRenderThisFrame.size() == 1);
      REQUIRE(result.tilesToRenderThisFrame.front() == root);

      REQUIRE(result.tilesFadingOut.size() == 0);

      REQUIRE(result.tilesVisited == 6);
      REQUIRE(result.workerThreadTileLoadQueueLength == 4);
      REQUIRE(result.tilesCulled == 0);
      REQUIRE(result.culledTilesVisited == 0);
    }

    // 2nd frame. Children are finished loading and ready to be rendered. Root
    // shouldn't be rendered in this frame
    {
      ViewUpdateResult result = tileset.updateView({viewState});

      // check tile states
      REQUIRE(root->getState() == TileLoadState::Done);
      for (const auto& child : root->getChildren()) {
        REQUIRE(child.getState() == TileLoadState::Done);
      }

      // check result
      REQUIRE(result.tilesToRenderThisFrame.size() == 4);
      for (const auto& child : root->getChildren()) {
        REQUIRE(
            std::find(
                result.tilesToRenderThisFrame.begin(),
                result.tilesToRenderThisFrame.end(),
                &child) != result.tilesToRenderThisFrame.end());
      }

      REQUIRE(result.tilesFadingOut.size() == 1);

      REQUIRE(result.tilesVisited == 6);
      REQUIRE(result.workerThreadTileLoadQueueLength == 0);
      REQUIRE(result.tilesCulled == 0);
      REQUIRE(result.culledTilesVisited == 0);
    }
  }
}

TEST_CASE("Test additive refinement") {
  Cesium3DTilesContent::registerAllTileContentTypes();

  std::filesystem::path testDataPath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testDataPath = testDataPath / "AddTileset";
  std::vector<std::string> files{
      "tileset.json",
      "tileset2.json",
      "parent.b3dm",
      "lr.b3dm",
      "ul.b3dm",
      "ur.b3dm",
      "tileset3/tileset3.json",
      "tileset3/ll.b3dm"};

  std::map<std::string, std::shared_ptr<SimpleAssetRequest>>
      mockCompletedRequests;
  for (const auto& file : files) {
    std::unique_ptr<SimpleAssetResponse> mockCompletedResponse =
        std::make_unique<SimpleAssetResponse>(
            static_cast<uint16_t>(200),
            "doesn't matter",
            CesiumAsync::HttpHeaders{},
            readFile(testDataPath / file));
    mockCompletedRequests.insert(
        {file,
         std::make_shared<SimpleAssetRequest>(
             "GET",
             file,
             CesiumAsync::HttpHeaders{},
             std::move(mockCompletedResponse))});
  }

  std::shared_ptr<SimpleAssetAccessor> mockAssetAccessor =
      std::make_shared<SimpleAssetAccessor>(std::move(mockCompletedRequests));
  TilesetExternals tilesetExternals{
      mockAssetAccessor,
      std::make_shared<SimplePrepareRendererResource>(),
      AsyncSystem(std::make_shared<SimpleTaskProcessor>()),
      nullptr};

  // create tileset and call updateView() to give it a chance to load
  Tileset tileset(tilesetExternals, "tileset.json");
  initializeTileset(tileset);

  // root is external tileset. Since its content is loading, we won't know if it
  // has children or not
  const Tile* pTilesetJson = tileset.getRootTile();
  REQUIRE(pTilesetJson);
  REQUIRE(pTilesetJson->getChildren().size() == 1);

  const Tile* root = &pTilesetJson->getChildren()[0];
  REQUIRE(root->getState() == TileLoadState::ContentLoading);
  REQUIRE(root->getChildren().size() == 0);

  SUBCASE("Load external tilesets") {
    ViewState viewState = zoomToTileset(tileset);

    // 1st frame. Root, its child, and its four grandchildren will all be
    // rendered because they meet SSE, even though they're not loaded yet.
    {
      ViewUpdateResult result = tileset.updateView({viewState});

      const std::vector<Tile*>& ttr = result.tilesToRenderThisFrame;
      REQUIRE(ttr.size() == 7);

      REQUIRE(root->getState() == TileLoadState::Done);
      REQUIRE(!doesTileMeetSSE(viewState, *root, tileset));
      REQUIRE(root->getChildren().size() == 1);
      REQUIRE(std::find(ttr.begin(), ttr.end(), pTilesetJson) != ttr.end());
      REQUIRE(std::find(ttr.begin(), ttr.end(), root) != ttr.end());

      const Tile& parentB3DM = root->getChildren().front();
      REQUIRE(parentB3DM.getState() == TileLoadState::ContentLoading);
      REQUIRE(!doesTileMeetSSE(viewState, parentB3DM, tileset));
      REQUIRE(parentB3DM.getChildren().size() == 4);
      REQUIRE(std::find(ttr.begin(), ttr.end(), &parentB3DM) != ttr.end());

      for (const Tile& child : parentB3DM.getChildren()) {
        REQUIRE(child.getState() == TileLoadState::ContentLoading);
        REQUIRE(doesTileMeetSSE(viewState, child, tileset));
        REQUIRE(std::find(ttr.begin(), ttr.end(), &child) != ttr.end());
      }

      REQUIRE(result.tilesFadingOut.size() == 0);

      REQUIRE(result.tilesVisited == 7);
      REQUIRE(result.workerThreadTileLoadQueueLength == 5);
      REQUIRE(result.tilesCulled == 0);
      REQUIRE(result.culledTilesVisited == 0);
    }

    // 2nd frame
    {
      ViewUpdateResult result = tileset.updateView({viewState});

      const std::vector<Tile*>& ttr = result.tilesToRenderThisFrame;
      REQUIRE(ttr.size() == 8);

      // root is done loading and rendered.
      REQUIRE(root->getState() == TileLoadState::Done);
      REQUIRE(!doesTileMeetSSE(viewState, *root, tileset));
      REQUIRE(root->getChildren().size() == 1);
      REQUIRE(std::find(ttr.begin(), ttr.end(), root) != ttr.end());

      // root's child is done loading and rendered, too.
      const Tile& parentB3DM = root->getChildren().front();
      REQUIRE(parentB3DM.getState() == TileLoadState::Done);
      REQUIRE(!doesTileMeetSSE(viewState, parentB3DM, tileset));
      REQUIRE(parentB3DM.getChildren().size() == 4);
      REQUIRE(std::find(ttr.begin(), ttr.end(), &parentB3DM) != ttr.end());

      for (const Tile& child : parentB3DM.getChildren()) {
        // parentB3DM's children are all done loading and are rendered.
        REQUIRE(child.getState() == TileLoadState::Done);
        REQUIRE(std::find(ttr.begin(), ttr.end(), &child) != ttr.end());

        if (*std::get_if<std::string>(&child.getTileID()) !=
            "tileset3/tileset3.json") {
          REQUIRE(doesTileMeetSSE(viewState, child, tileset));
        } else {
          // external tilesets get unconditionally refined
          REQUIRE(root->getUnconditionallyRefine());

          // expect the children to meet sse and begin loading the content,
          // while also getting rendered.
          REQUIRE(child.getChildren().size() == 1);
          REQUIRE(
              doesTileMeetSSE(viewState, child.getChildren().front(), tileset));
          REQUIRE(
              child.getChildren().front().getState() ==
              TileLoadState::ContentLoading);
          REQUIRE(
              std::find(ttr.begin(), ttr.end(), &child.getChildren().front()) !=
              ttr.end());
        }
      }

      REQUIRE(result.tilesFadingOut.size() == 0);

      REQUIRE(result.tilesVisited == 8);
      REQUIRE(result.workerThreadTileLoadQueueLength == 1);
      REQUIRE(result.tilesCulled == 0);
      REQUIRE(result.culledTilesVisited == 0);
    }

    // 3rd frame. All the children finish loading. All should be rendered now
    {
      ViewUpdateResult result = tileset.updateView({viewState});

      REQUIRE(result.tilesToRenderThisFrame.size() == 8);

      REQUIRE(result.tilesFadingOut.size() == 0);

      REQUIRE(result.tilesVisited == 8);
      REQUIRE(result.workerThreadTileLoadQueueLength == 0);
      REQUIRE(result.tilesCulled == 0);
      REQUIRE(result.culledTilesVisited == 0);
    }
  }
}

TEST_CASE("Render any tiles even when one of children can't be rendered for "
          "additive refinement") {
  Cesium3DTilesContent::registerAllTileContentTypes();

  std::filesystem::path testDataPath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testDataPath = testDataPath / "ErrorChildrenAddTileset";
  std::vector<std::string> files{
      "tileset.json",
      "parent.b3dm",
      "error_lr.b3dm",
      "ul.b3dm",
      "ur.b3dm",
  };

  std::map<std::string, std::shared_ptr<SimpleAssetRequest>>
      mockCompletedRequests;
  for (const auto& file : files) {
    std::unique_ptr<SimpleAssetResponse> mockCompletedResponse =
        std::make_unique<SimpleAssetResponse>(
            static_cast<uint16_t>(200),
            "doesn't matter",
            CesiumAsync::HttpHeaders{},
            readFile(testDataPath / file));
    mockCompletedRequests.insert(
        {file,
         std::make_shared<SimpleAssetRequest>(
             "GET",
             file,
             CesiumAsync::HttpHeaders{},
             std::move(mockCompletedResponse))});
  }

  std::shared_ptr<SimpleAssetAccessor> mockAssetAccessor =
      std::make_shared<SimpleAssetAccessor>(std::move(mockCompletedRequests));
  TilesetExternals tilesetExternals{
      mockAssetAccessor,
      std::make_shared<SimplePrepareRendererResource>(),
      AsyncSystem(std::make_shared<SimpleTaskProcessor>()),
      nullptr};

  // create tileset and call updateView() to give it a chance to load
  Tileset tileset(tilesetExternals, "tileset.json");
  initializeTileset(tileset);
  ViewState viewState = zoomToTileset(tileset);

  Tile* pTilesetJson = tileset.getRootTile();
  REQUIRE(pTilesetJson);
  REQUIRE(pTilesetJson->getChildren().size() == 1);
  Tile* root = &pTilesetJson->getChildren()[0];

  REQUIRE(!doesTileMeetSSE(viewState, *root, tileset));
  REQUIRE(root->getState() == TileLoadState::ContentLoading);
  REQUIRE(root->getChildren().size() == 3);

  // 1st frame. Root doesn't meet sse, so load children. But they are
  // non-renderable, so render root only
  {
    ViewUpdateResult result = tileset.updateView({viewState});

    for (const Tile& child : root->getChildren()) {
      CHECK(child.getState() == TileLoadState::ContentLoading);
    }

    REQUIRE(result.tilesToRenderThisFrame.size() == 2);
    REQUIRE(result.tilesFadingOut.size() == 0);
    REQUIRE(result.tilesVisited == 5);
    REQUIRE(result.workerThreadTileLoadQueueLength == 3);
    REQUIRE(result.tilesCulled == 0);
    REQUIRE(result.culledTilesVisited == 0);
  }

  // 2nd frame. Root doesn't meet sse, so load children. Even one of the
  // children is failed, render all of them even there is a hole
  {
    ViewUpdateResult result = tileset.updateView({viewState});

    REQUIRE(root->isRenderable());

    // first child will have failed empty content, but other children
    const auto& children = root->getChildren();
    REQUIRE(children[0].getState() == TileLoadState::Failed);
    REQUIRE(children[0].isRenderable());
    for (const Tile& child : children.subspan(1)) {
      REQUIRE(child.getState() == TileLoadState::Done);
      REQUIRE(child.isRenderable());
    }

    REQUIRE(result.tilesToRenderThisFrame.size() == 5);
    REQUIRE(result.tilesFadingOut.size() == 0);
    REQUIRE(result.tilesVisited == 5);
    REQUIRE(result.workerThreadTileLoadQueueLength == 0);
    REQUIRE(result.tilesCulled == 0);
    REQUIRE(result.culledTilesVisited == 0);
  }
}

TEST_CASE("Test multiple frustums") {
  Cesium3DTilesContent::registerAllTileContentTypes();

  std::filesystem::path testDataPath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testDataPath = testDataPath / "ReplaceTileset";
  std::vector<std::string> files{
      "tileset.json",
      "parent.b3dm",
      "ll.b3dm",
      "lr.b3dm",
      "ul.b3dm",
      "ur.b3dm",
      "ll_ll.b3dm",
  };

  std::map<std::string, std::shared_ptr<SimpleAssetRequest>>
      mockCompletedRequests;
  for (const auto& file : files) {
    std::unique_ptr<SimpleAssetResponse> mockCompletedResponse =
        std::make_unique<SimpleAssetResponse>(
            static_cast<uint16_t>(200),
            "doesn't matter",
            CesiumAsync::HttpHeaders{},
            readFile(testDataPath / file));
    mockCompletedRequests.insert(
        {file,
         std::make_shared<SimpleAssetRequest>(
             "GET",
             file,
             CesiumAsync::HttpHeaders{},
             std::move(mockCompletedResponse))});
  }

  std::shared_ptr<SimpleAssetAccessor> mockAssetAccessor =
      std::make_shared<SimpleAssetAccessor>(std::move(mockCompletedRequests));
  TilesetExternals tilesetExternals{
      mockAssetAccessor,
      std::make_shared<SimplePrepareRendererResource>(),
      AsyncSystem(std::make_shared<SimpleTaskProcessor>()),
      nullptr};

  // create tileset and call updateView() to give it a chance to load
  Tileset tileset(tilesetExternals, "tileset.json");
  initializeTileset(tileset);

  // check the tiles status
  const Tile* pTilesetJson = tileset.getRootTile();
  REQUIRE(pTilesetJson);
  REQUIRE(pTilesetJson->getChildren().size() == 1);
  const Tile* root = &pTilesetJson->getChildren()[0];
  REQUIRE(root->getState() == TileLoadState::ContentLoading);
  for (const auto& child : root->getChildren()) {
    REQUIRE(child.getState() == TileLoadState::Unloaded);
  }

  // Zoom to tileset. Expect the root will not meet sse in this configure
  ViewState viewState = zoomToTileset(tileset);

  // Zoom out from the tileset a little bit to make sure the root meets sse
  glm::dvec3 zoomOutPosition =
      viewState.getPosition() - viewState.getDirection() * 2500.0;
  ViewState zoomOutViewState = ViewState::create(
      zoomOutPosition,
      viewState.getDirection(),
      viewState.getUp(),
      viewState.getViewportSize(),
      viewState.getHorizontalFieldOfView(),
      viewState.getVerticalFieldOfView(),
      Ellipsoid::WGS84);

  SUBCASE("The frustum with the highest SSE should be used for deciding to "
          "refine") {

    // frame 1
    {
      ViewUpdateResult result =
          tileset.updateView({viewState, zoomOutViewState});

      // Check tile state. Ensure root meets sse for only the zoomed out
      // ViewState
      REQUIRE(root->getState() == TileLoadState::Done);
      REQUIRE(!doesTileMeetSSE(viewState, *root, tileset));
      REQUIRE(doesTileMeetSSE(zoomOutViewState, *root, tileset));
      for (const auto& child : root->getChildren()) {
        REQUIRE(child.getState() == TileLoadState::ContentLoading);
      }

      // check result
      REQUIRE(result.tilesToRenderThisFrame.size() == 1);
      REQUIRE(result.workerThreadTileLoadQueueLength == 4);
      REQUIRE(result.tilesToRenderThisFrame.front() == root);
    }

    // frame 2
    {
      ViewUpdateResult result =
          tileset.updateView({viewState, zoomOutViewState});

      // Check tile state. Ensure root meets sse for only the zoomed out
      // ViewState
      REQUIRE(root->getState() == TileLoadState::Done);
      REQUIRE(!doesTileMeetSSE(viewState, *root, tileset));
      REQUIRE(doesTileMeetSSE(zoomOutViewState, *root, tileset));
      for (const auto& child : root->getChildren()) {
        REQUIRE(child.getState() == TileLoadState::Done);
      }

      // check result
      REQUIRE(result.tilesToRenderThisFrame.size() == 4);

      REQUIRE(result.tilesFadingOut.size() == 1);
      REQUIRE(*result.tilesFadingOut.begin() == root);

      REQUIRE(result.tilesVisited == 6);
      REQUIRE(result.workerThreadTileLoadQueueLength == 0);
      REQUIRE(result.tilesCulled == 0);
    }
  }

  SUBCASE("Tiles should be culled if all the cameras agree") {

    REQUIRE(root->getChildren().size() == 4);
    const Tile& firstChild = root->getChildren()[0];
    const Tile& secondChild = root->getChildren()[1];
    REQUIRE(firstChild.getChildren().size() == 1);
    const Tile& grandChild = firstChild.getChildren()[0];

    ViewState zoomToTileViewState = zoomToTile(firstChild);

    // Expected to only contain the grand child
    // (child of the first child of the root).
    glm::dvec3 zoomInPosition = zoomToTileViewState.getPosition() +
                                zoomToTileViewState.getDirection() * 250.0;
    ViewState zoomInViewState1 = ViewState::create(
        zoomInPosition,
        zoomToTileViewState.getDirection(),
        zoomToTileViewState.getUp(),
        zoomToTileViewState.getViewportSize(),
        0.5 * zoomToTileViewState.getHorizontalFieldOfView(),
        0.5 * zoomToTileViewState.getVerticalFieldOfView(),
        Ellipsoid::WGS84);

    zoomInPosition = zoomToTileViewState.getPosition() +
                     glm::dvec3(15.0, 0, 0) +
                     zoomToTileViewState.getDirection() * 243.0;
    ViewState zoomInViewState2 = ViewState::create(
        zoomInPosition,
        zoomToTileViewState.getDirection(),
        zoomToTileViewState.getUp(),
        zoomToTileViewState.getViewportSize(),
        0.5 * zoomToTileViewState.getHorizontalFieldOfView(),
        0.5 * zoomToTileViewState.getVerticalFieldOfView(),
        Ellipsoid::WGS84);

    // frame 3 & 4
    {
      tileset.updateView({zoomInViewState1, zoomInViewState2});
      ViewUpdateResult result =
          tileset.updateView({zoomInViewState1, zoomInViewState2});

      // check result
      // The grand child and the second child are the only ones rendered.
      // The third and fourth children of the root are culled.
      REQUIRE(result.tilesToRenderThisFrame.size() == 2);
      REQUIRE(result.tilesVisited == 5);
      REQUIRE(
          std::find(
              result.tilesToRenderThisFrame.begin(),
              result.tilesToRenderThisFrame.end(),
              &grandChild) != result.tilesToRenderThisFrame.end());
      REQUIRE(
          std::find(
              result.tilesToRenderThisFrame.begin(),
              result.tilesToRenderThisFrame.end(),
              &secondChild) != result.tilesToRenderThisFrame.end());
      REQUIRE(result.tilesCulled == 2);
    }
  }
}

TEST_CASE("Can load example tileset.json from 3DTILES_bounding_volume_S2 "
          "documentation") {
  std::string s = R"(
      {
        "asset": {
          "version": "1.0"
        },
        "geometricError": 1000000,
        "extensionsUsed": [
          "3DTILES_bounding_volume_S2"
        ],
        "extensionsRequired": [
          "3DTILES_bounding_volume_S2"
        ],
        "root": {
          "boundingVolume": {
            "extensions": {
              "3DTILES_bounding_volume_S2": {
                "token": "3",
                "minimumHeight": 0,
                "maximumHeight": 1000000
              }
            }
          },
          "refine": "REPLACE",
          "geometricError": 50000,
          "children": [
            {
              "boundingVolume": {
                "extensions": {
                  "3DTILES_bounding_volume_S2": {
                    "token": "2c",
                    "minimumHeight": 0,
                    "maximumHeight": 500000
                  }
                }
              },
              "refine": "REPLACE",
              "geometricError": 500000,
              "children": [
                {
                  "boundingVolume": {
                    "extensions": {
                      "3DTILES_bounding_volume_S2": {
                        "token": "2f",
                        "minimumHeight": 0,
                        "maximumHeight": 250000
                      }
                    }
                  },
                  "refine": "REPLACE",
                  "geometricError": 250000,
                  "children": [
                    {
                      "boundingVolume": {
                        "extensions": {
                          "3DTILES_bounding_volume_S2": {
                            "token": "2ec",
                            "minimumHeight": 0,
                            "maximumHeight": 125000
                          }
                        }
                      },
                      "refine": "REPLACE",
                      "geometricError": 125000
                    }
                  ]
                }
              ]
            }
          ]
        }
      })";

  std::map<std::string, std::shared_ptr<SimpleAssetRequest>>
      mockCompletedRequests;

  const std::byte* pStart = reinterpret_cast<const std::byte*>(s.c_str());

  std::unique_ptr<SimpleAssetResponse> mockCompletedResponse =
      std::make_unique<SimpleAssetResponse>(
          static_cast<uint16_t>(200),
          "doesn't matter",
          CesiumAsync::HttpHeaders{},
          std::vector<std::byte>(pStart, pStart + s.size()));
  mockCompletedRequests.insert(
      {"tileset.json",
       std::make_shared<SimpleAssetRequest>(
           "GET",
           "tileset.json",
           CesiumAsync::HttpHeaders{},
           std::move(mockCompletedResponse))});

  std::shared_ptr<SimpleAssetAccessor> mockAssetAccessor =
      std::make_shared<SimpleAssetAccessor>(std::move(mockCompletedRequests));
  TilesetExternals tilesetExternals{
      mockAssetAccessor,
      std::make_shared<SimplePrepareRendererResource>(),
      AsyncSystem(std::make_shared<SimpleTaskProcessor>()),
      nullptr};

  // create tileset and wait for it to load.
  Tileset tileset(tilesetExternals, "tileset.json");
  while (!tileset.getRootTile()) {
    tilesetExternals.asyncSystem.dispatchMainThreadTasks();
  }

  const Tile* pTilesetJson = tileset.getRootTile();
  REQUIRE(pTilesetJson);
  REQUIRE(pTilesetJson->getChildren().size() == 1);
  const Tile* pRoot = &pTilesetJson->getChildren()[0];

  const S2CellBoundingVolume* pS2 =
      std::get_if<S2CellBoundingVolume>(&pRoot->getBoundingVolume());
  REQUIRE(pS2);

  CHECK(pS2->getCellID().toToken() == "3");
  CHECK(pS2->getMinimumHeight() == 0.0);
  CHECK(pS2->getMaximumHeight() == 1000000.0);

  REQUIRE(pRoot->getChildren().size() == 1);
  const Tile* pChild = &pRoot->getChildren()[0];
  const S2CellBoundingVolume* pS2Child =
      std::get_if<S2CellBoundingVolume>(&pChild->getBoundingVolume());
  REQUIRE(pS2Child);

  CHECK(pS2Child->getCellID().toToken() == "2c");
  CHECK(pS2Child->getMinimumHeight() == 0.0);
  CHECK(pS2Child->getMaximumHeight() == 500000.0);

  REQUIRE(pChild->getChildren().size() == 1);
  const Tile* pGrandchild = &pChild->getChildren()[0];
  const S2CellBoundingVolume* pS2Grandchild =
      std::get_if<S2CellBoundingVolume>(&pGrandchild->getBoundingVolume());
  REQUIRE(pS2Grandchild);

  CHECK(pS2Grandchild->getCellID().toToken() == "2f");
  CHECK(pS2Grandchild->getMinimumHeight() == 0.0);
  CHECK(pS2Grandchild->getMaximumHeight() == 250000.0);

  REQUIRE(pGrandchild->getChildren().size() == 1);
  const Tile* pGreatGrandchild = &pGrandchild->getChildren()[0];
  const S2CellBoundingVolume* pS2GreatGrandchild =
      std::get_if<S2CellBoundingVolume>(&pGreatGrandchild->getBoundingVolume());
  REQUIRE(pS2GreatGrandchild);

  CHECK(pS2GreatGrandchild->getCellID().toToken() == "2ec");
  CHECK(pS2GreatGrandchild->getMinimumHeight() == 0.0);
  CHECK(pS2GreatGrandchild->getMaximumHeight() == 125000.0);

  REQUIRE(pGreatGrandchild->getChildren().empty());
}

TEST_CASE("Makes metadata available once root tile is loaded") {
  Cesium3DTilesContent::registerAllTileContentTypes();

  std::filesystem::path testDataPath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testDataPath = testDataPath / "WithMetadata";
  std::vector<std::string> files{
      "tileset.json",
      "external-tileset.json",
      "parent.b3dm",
      "ll.b3dm",
      "lr.b3dm",
      "ul.b3dm",
      "ur.b3dm"};

  std::map<std::string, std::shared_ptr<SimpleAssetRequest>>
      mockCompletedRequests;
  for (const auto& file : files) {
    std::unique_ptr<SimpleAssetResponse> mockCompletedResponse =
        std::make_unique<SimpleAssetResponse>(
            static_cast<uint16_t>(200),
            "doesn't matter",
            CesiumAsync::HttpHeaders{},
            readFile(testDataPath / file));
    mockCompletedRequests.insert(
        {file,
         std::make_shared<SimpleAssetRequest>(
             "GET",
             file,
             CesiumAsync::HttpHeaders{},
             std::move(mockCompletedResponse))});
  }

  std::shared_ptr<SimpleAssetAccessor> mockAssetAccessor =
      std::make_shared<SimpleAssetAccessor>(std::move(mockCompletedRequests));
  TilesetExternals tilesetExternals{
      mockAssetAccessor,
      std::make_shared<SimplePrepareRendererResource>(),
      AsyncSystem(std::make_shared<SimpleTaskProcessor>()),
      nullptr};

  // create tileset and call updateView() to give it a chance to load
  Tileset tileset(tilesetExternals, "tileset.json");
  initializeTileset(tileset);

  Tile* pRoot = tileset.getRootTile();
  REQUIRE(pRoot);

  TileExternalContent* pExternal = pRoot->getContent().getExternalContent();
  REQUIRE(pExternal);

  const TilesetMetadata& metadata = pExternal->metadata;
  const std::optional<Cesium3DTiles::Schema>& schema = metadata.schema;
  REQUIRE(schema);
  CHECK(schema->id == "foo");
}

TEST_CASE("Makes metadata available on external tilesets") {
  Cesium3DTilesContent::registerAllTileContentTypes();

  std::filesystem::path testDataPath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testDataPath = testDataPath / "WithMetadata";
  std::vector<std::string> files{
      "tileset.json",
      "external-tileset.json",
      "parent.b3dm",
      "ll.b3dm",
      "lr.b3dm",
      "ul.b3dm",
      "ur.b3dm"};

  std::map<std::string, std::shared_ptr<SimpleAssetRequest>>
      mockCompletedRequests;
  for (const auto& file : files) {
    std::unique_ptr<SimpleAssetResponse> mockCompletedResponse =
        std::make_unique<SimpleAssetResponse>(
            static_cast<uint16_t>(200),
            "doesn't matter",
            CesiumAsync::HttpHeaders{},
            readFile(testDataPath / file));
    mockCompletedRequests.insert(
        {file,
         std::make_shared<SimpleAssetRequest>(
             "GET",
             file,
             CesiumAsync::HttpHeaders{},
             std::move(mockCompletedResponse))});
  }

  std::shared_ptr<SimpleAssetAccessor> mockAssetAccessor =
      std::make_shared<SimpleAssetAccessor>(std::move(mockCompletedRequests));
  TilesetExternals tilesetExternals{
      mockAssetAccessor,
      std::make_shared<SimplePrepareRendererResource>(),
      AsyncSystem(std::make_shared<SimpleTaskProcessor>()),
      nullptr};

  // create tileset and call updateView() to give it a chance to load
  Tileset tileset(tilesetExternals, "tileset.json");
  initializeTileset(tileset);

  Tile* pTilesetJson = tileset.getRootTile();
  REQUIRE(pTilesetJson);
  REQUIRE(pTilesetJson->getChildren().size() == 1);

  Tile* pRoot = &pTilesetJson->getChildren()[0];
  REQUIRE(pRoot);
  REQUIRE(pRoot->getChildren().size() == 5);
  Tile* pExternal = &pRoot->getChildren()[4];

  TileExternalContent* pExternalContent = nullptr;

  for (int i = 0; i < 10 && pExternalContent == nullptr; ++i) {
    ViewState zoomToTileViewState = zoomToTile(*pExternal);
    tileset.updateView({zoomToTileViewState});
    pExternalContent = pExternal->getContent().getExternalContent();
  }

  REQUIRE(pExternalContent);

  REQUIRE(pExternalContent->metadata.groups.size() == 2);
  CHECK(pExternalContent->metadata.groups[0].classProperty == "someClass");
  CHECK(pExternalContent->metadata.groups[1].classProperty == "someClass");
}

TEST_CASE("Allows access to material variants") {
  Cesium3DTilesContent::registerAllTileContentTypes();

  std::filesystem::path testDataPath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testDataPath = testDataPath / "MaterialVariants";
  std::vector<std::string> files{"tileset.json", "parent.b3dm"};

  std::map<std::string, std::shared_ptr<SimpleAssetRequest>>
      mockCompletedRequests;
  for (const auto& file : files) {
    std::unique_ptr<SimpleAssetResponse> mockCompletedResponse =
        std::make_unique<SimpleAssetResponse>(
            static_cast<uint16_t>(200),
            "doesn't matter",
            CesiumAsync::HttpHeaders{},
            readFile(testDataPath / file));
    mockCompletedRequests.insert(
        {file,
         std::make_shared<SimpleAssetRequest>(
             "GET",
             file,
             CesiumAsync::HttpHeaders{},
             std::move(mockCompletedResponse))});
  }

  std::shared_ptr<SimpleAssetAccessor> mockAssetAccessor =
      std::make_shared<SimpleAssetAccessor>(std::move(mockCompletedRequests));
  TilesetExternals tilesetExternals{
      mockAssetAccessor,
      std::make_shared<SimplePrepareRendererResource>(),
      AsyncSystem(std::make_shared<SimpleTaskProcessor>()),
      nullptr};

  // create tileset and call updateView() to give it a chance to load
  Tileset tileset(tilesetExternals, "tileset.json");
  initializeTileset(tileset);

  const TilesetMetadata* pMetadata = tileset.getMetadata();
  REQUIRE(pMetadata);
  REQUIRE(pMetadata->schema);
  REQUIRE(pMetadata->metadata);

  std::optional<Cesium3DTiles::FoundMetadataProperty> found1 =
      Cesium3DTiles::MetadataQuery::findFirstPropertyWithSemantic(
          *pMetadata->schema,
          *pMetadata->metadata,
          "MATERIAL_VARIANTS");
  REQUIRE(found1);
  CHECK(found1->classIdentifier == "MaterialVariants");
  CHECK(found1->classDefinition.properties.size() == 1);
  CHECK(found1->propertyIdentifier == "material_variants");
  CHECK(
      found1->propertyDefinition.description ==
      "Names of material variants to be expected in the glTF assets");
  REQUIRE(found1->propertyValue.isArray());

  std::vector<std::string> variants =
      found1->propertyValue.getArrayOfStrings("");
  REQUIRE(variants.size() == 4);
  CHECK(variants[0] == "RGB");
  CHECK(variants[1] == "RRR");
  CHECK(variants[2] == "GGG");
  CHECK(variants[3] == "BBB");

  std::vector<std::vector<std::string>> variantsByGroup;
  for (const Cesium3DTiles::GroupMetadata& group : pMetadata->groups) {
    std::optional<Cesium3DTiles::FoundMetadataProperty> found2 =
        Cesium3DTiles::MetadataQuery::findFirstPropertyWithSemantic(
            *pMetadata->schema,
            group,
            "MATERIAL_VARIANTS");
    REQUIRE(found2);
    REQUIRE(found2->propertyValue.isArray());

    variantsByGroup.emplace_back(found2->propertyValue.getArrayOfStrings(""));
  }

  std::vector<std::vector<std::string>> expected = {
      {"RGB", "RRR"},
      {"GGG", "BBB"}};

  CHECK(variantsByGroup == expected);
}

TEST_CASE("Allows access to material variants in an external schema") {
  Cesium3DTilesContent::registerAllTileContentTypes();

  std::filesystem::path testDataPath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testDataPath = testDataPath / "MaterialVariants";
  std::vector<std::string> files{
      "tileset-external-schema.json",
      "schema.json",
      "parent.b3dm"};

  std::map<std::string, std::shared_ptr<SimpleAssetRequest>>
      mockCompletedRequests;
  for (const auto& file : files) {
    std::unique_ptr<SimpleAssetResponse> mockCompletedResponse =
        std::make_unique<SimpleAssetResponse>(
            static_cast<uint16_t>(200),
            "doesn't matter",
            CesiumAsync::HttpHeaders{},
            readFile(testDataPath / file));
    mockCompletedRequests.insert(
        {file,
         std::make_shared<SimpleAssetRequest>(
             "GET",
             file,
             CesiumAsync::HttpHeaders{},
             std::move(mockCompletedResponse))});
  }

  std::shared_ptr<SimpleAssetAccessor> mockAssetAccessor =
      std::make_shared<SimpleAssetAccessor>(std::move(mockCompletedRequests));
  TilesetExternals tilesetExternals{
      mockAssetAccessor,
      std::make_shared<SimplePrepareRendererResource>(),
      AsyncSystem(std::make_shared<SimpleTaskProcessor>()),
      nullptr};

  Tileset tileset(tilesetExternals, "tileset-external-schema.json");

  // getMetadata returns nullptr before the root tile is loaded.
  CHECK(tileset.getMetadata() == nullptr);

  bool wasCalled = false;
  tileset.loadMetadata().thenInMainThread(
      [&wasCalled](const TilesetMetadata* pMetadata) {
        wasCalled = true;
        REQUIRE(pMetadata);
        REQUIRE(pMetadata->schema);
        REQUIRE(pMetadata->metadata);

        std::optional<Cesium3DTiles::FoundMetadataProperty> found1 =
            Cesium3DTiles::MetadataQuery::findFirstPropertyWithSemantic(
                *pMetadata->schema,
                *pMetadata->metadata,
                "MATERIAL_VARIANTS");
        REQUIRE(found1);
        CHECK(found1->classIdentifier == "MaterialVariants");
        CHECK(found1->classDefinition.properties.size() == 1);
        CHECK(found1->propertyIdentifier == "material_variants");
        CHECK(
            found1->propertyDefinition.description ==
            "Names of material variants to be expected in the glTF assets");
        REQUIRE(found1->propertyValue.isArray());

        std::vector<std::string> variants =
            found1->propertyValue.getArrayOfStrings("");
        REQUIRE(variants.size() == 4);
        CHECK(variants[0] == "RGB");
        CHECK(variants[1] == "RRR");
        CHECK(variants[2] == "GGG");
        CHECK(variants[3] == "BBB");

        std::vector<std::vector<std::string>> variantsByGroup;
        for (const Cesium3DTiles::GroupMetadata& group : pMetadata->groups) {
          std::optional<Cesium3DTiles::FoundMetadataProperty> found2 =
              Cesium3DTiles::MetadataQuery::findFirstPropertyWithSemantic(
                  *pMetadata->schema,
                  group,
                  "MATERIAL_VARIANTS");
          REQUIRE(found2);
          REQUIRE(found2->propertyValue.isArray());
          variantsByGroup.emplace_back(
              found2->propertyValue.getArrayOfStrings(""));
        }

        std::vector<std::vector<std::string>> expected = {
            {"RGB", "RRR"},
            {"GGG", "BBB"}};

        CHECK(variantsByGroup == expected);
      });

  CHECK(!wasCalled);
  initializeTileset(tileset);
  CHECK(wasCalled);
}

TEST_CASE("Future from loadSchema rejects if schemaUri can't be loaded") {
  Cesium3DTilesContent::registerAllTileContentTypes();

  std::filesystem::path testDataPath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testDataPath = testDataPath / "MaterialVariants";
  std::vector<std::string> files{"tileset-external-schema.json", "parent.b3dm"};

  std::map<std::string, std::shared_ptr<SimpleAssetRequest>>
      mockCompletedRequests;
  for (const auto& file : files) {
    std::unique_ptr<SimpleAssetResponse> mockCompletedResponse =
        std::make_unique<SimpleAssetResponse>(
            static_cast<uint16_t>(200),
            "doesn't matter",
            CesiumAsync::HttpHeaders{},
            readFile(testDataPath / file));
    mockCompletedRequests.insert(
        {file,
         std::make_shared<SimpleAssetRequest>(
             "GET",
             file,
             CesiumAsync::HttpHeaders{},
             std::move(mockCompletedResponse))});
  }

  mockCompletedRequests.insert(
      {"schema.json",
       std::make_shared<SimpleAssetRequest>(
           "GET",
           "schema.json",
           CesiumAsync::HttpHeaders{},
           std::make_unique<SimpleAssetResponse>(
               static_cast<uint16_t>(404),
               "doesn't matter",
               CesiumAsync::HttpHeaders{},
               std::vector<std::byte>()))});

  std::shared_ptr<SimpleAssetAccessor> mockAssetAccessor =
      std::make_shared<SimpleAssetAccessor>(std::move(mockCompletedRequests));
  TilesetExternals tilesetExternals{
      mockAssetAccessor,
      std::make_shared<SimplePrepareRendererResource>(),
      AsyncSystem(std::make_shared<SimpleTaskProcessor>()),
      nullptr};

  Tileset tileset(tilesetExternals, "tileset-external-schema.json");

  // getMetadata returns nullptr before the root tile is loaded.
  CHECK(tileset.getMetadata() == nullptr);

  bool wasResolved = false;
  bool wasRejected = false;
  tileset.loadMetadata()
      .thenInMainThread(
          [&wasResolved](const TilesetMetadata*) { wasResolved = true; })
      .catchInMainThread([&wasRejected](const std::exception& exception) {
        CHECK(std::string(exception.what()).find("") != std::string::npos);
        wasRejected = true;
      });

  CHECK(!wasResolved);
  CHECK(!wasRejected);

  initializeTileset(tileset);
  CHECK(!wasResolved);
  CHECK(wasRejected);
}

namespace {

void runUnconditionallyRefinedTestCase(const TilesetOptions& options) {
  class CustomContentLoader : public TilesetContentLoader {
  public:
    Tile* _pRootTile = nullptr;
    std::optional<Promise<TileLoadResult>> _grandchildPromise;

    std::unique_ptr<Tile> createRootTile() {
      auto pRootTile = std::make_unique<Tile>(this);
      this->_pRootTile = pRootTile.get();

      pRootTile->setTileID(CesiumGeometry::QuadtreeTileID(0, 0, 0));

      Cartographic center = Cartographic::fromDegrees(118.0, 32.0, 0.0);
      pRootTile->setBoundingVolume(CesiumGeospatial::BoundingRegion(
          GlobeRectangle(
              center.longitude - 0.001,
              center.latitude - 0.001,
              center.longitude + 0.001,
              center.latitude + 0.001),
          0.0,
          10.0,
          Ellipsoid::WGS84));
      pRootTile->setGeometricError(100000000000.0);

      Tile child(this);
      child.setTileID(CesiumGeometry::QuadtreeTileID(1, 0, 0));
      child.setBoundingVolume(pRootTile->getBoundingVolume());
      child.setGeometricError(1e100);

      std::vector<Tile> children;
      children.emplace_back(std::move(child));

      pRootTile->createChildTiles(std::move(children));

      Tile grandchild(this);
      grandchild.setTileID(CesiumGeometry::QuadtreeTileID(1, 0, 0));
      grandchild.setBoundingVolume(pRootTile->getBoundingVolume());
      grandchild.setGeometricError(0.1);

      std::vector<Tile> grandchildren;
      grandchildren.emplace_back(std::move(grandchild));
      pRootTile->getChildren()[0].createChildTiles(std::move(grandchildren));

      return pRootTile;
    }

    virtual CesiumAsync::Future<TileLoadResult>
    loadTileContent(const TileLoadInput& input) override {
      if (&input.tile == this->_pRootTile) {
        TileLoadResult result{};
        result.contentKind = CesiumGltf::Model();
        return input.asyncSystem.createResolvedFuture(std::move(result));
      } else if (input.tile.getParent() == this->_pRootTile) {
        TileLoadResult result{};
        result.contentKind = TileEmptyContent();
        return input.asyncSystem.createResolvedFuture(std::move(result));
      } else if (
          input.tile.getParent() != nullptr &&
          input.tile.getParent()->getParent() == this->_pRootTile) {
        this->_grandchildPromise =
            input.asyncSystem.createPromise<TileLoadResult>();
        return this->_grandchildPromise->getFuture();
      }

      return input.asyncSystem.createResolvedFuture(
          TileLoadResult::createFailedResult(input.pAssetAccessor, nullptr));
    }

    virtual TileChildrenResult createTileChildren(
        const Tile&,
        const CesiumGeospatial::Ellipsoid&) override {
      return TileChildrenResult{{}, TileLoadResultState::Failed};
    }
  };

  TilesetExternals tilesetExternals{
      nullptr,
      std::make_shared<SimplePrepareRendererResource>(),
      AsyncSystem(std::make_shared<SimpleTaskProcessor>()),
      nullptr};

  auto pCustomLoader = std::make_unique<CustomContentLoader>();
  CustomContentLoader* pRawLoader = pCustomLoader.get();

  Tileset tileset(
      tilesetExternals,
      std::move(pCustomLoader),
      pRawLoader->createRootTile(),
      options);

  // On the first update, we should refine down to the grandchild tile, even
  // though no tiles are loaded yet.
  initializeTileset(tileset);
  const Tile& child = tileset.getRootTile()->getChildren()[0];
  const Tile& grandchild = child.getChildren()[0];
  CHECK(
      tileset.getRootTile()->getLastSelectionState().getResult(
          tileset.getRootTile()->getLastSelectionState().getFrameNumber()) ==
      TileSelectionState::Result::Refined);
  CHECK(
      child.getLastSelectionState().getResult(
          tileset.getRootTile()->getLastSelectionState().getFrameNumber()) ==
      TileSelectionState::Result::Refined);
  CHECK(
      grandchild.getLastSelectionState().getResult(
          tileset.getRootTile()->getLastSelectionState().getFrameNumber()) ==
      TileSelectionState::Result::Rendered);

  // After the third update, the root and child tiles have been loaded, while
  // the grandchild has not. But the child is unconditionally refined, so we
  // can't render that one. Therefore the root tile should be rendered, after
  // the child and grandchild are kicked.
  initializeTileset(tileset);
  initializeTileset(tileset);
  CHECK(
      tileset.getRootTile()->getLastSelectionState().getResult(
          tileset.getRootTile()->getLastSelectionState().getFrameNumber()) ==
      TileSelectionState::Result::Rendered);
  CHECK(
      child.getLastSelectionState().getResult(
          child.getLastSelectionState().getFrameNumber()) !=
      TileSelectionState::Result::Rendered);
  CHECK(
      grandchild.getLastSelectionState().getResult(
          grandchild.getLastSelectionState().getFrameNumber()) !=
      TileSelectionState::Result::Rendered);

  REQUIRE(pRawLoader->_grandchildPromise);

  // Once the grandchild is loaded, it should be rendered instead.
  TileLoadResult result{};
  result.contentKind = CesiumGltf::Model();
  pRawLoader->_grandchildPromise->resolve(std::move(result));

  initializeTileset(tileset);

  CHECK(
      grandchild.getLastSelectionState().getResult(
          grandchild.getLastSelectionState().getFrameNumber()) ==
      TileSelectionState::Result::Rendered);
}

} // namespace

TEST_CASE("An unconditionally-refined tile is not rendered") {
  SUBCASE("With default settings") {
    runUnconditionallyRefinedTestCase(TilesetOptions());
  }

  SUBCASE("With forbidHoles enabled") {
    TilesetOptions options{};
    options.forbidHoles = true;
    runUnconditionallyRefinedTestCase(options);
  }
}

TEST_CASE("Additive-refined tiles are added to the tilesFadingOut array") {
  Cesium3DTilesContent::registerAllTileContentTypes();

  std::filesystem::path testDataPath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testDataPath = testDataPath / "AdditiveThreeLevels";
  std::vector<std::string> files{"tileset.json", "content.b3dm"};

  std::map<std::string, std::shared_ptr<SimpleAssetRequest>>
      mockCompletedRequests;
  for (const auto& file : files) {
    std::unique_ptr<SimpleAssetResponse> mockCompletedResponse =
        std::make_unique<SimpleAssetResponse>(
            static_cast<uint16_t>(200),
            "doesn't matter",
            CesiumAsync::HttpHeaders{},
            readFile(testDataPath / file));
    mockCompletedRequests.insert(
        {file,
         std::make_shared<SimpleAssetRequest>(
             "GET",
             file,
             CesiumAsync::HttpHeaders{},
             std::move(mockCompletedResponse))});
  }

  std::shared_ptr<SimpleAssetAccessor> mockAssetAccessor =
      std::make_shared<SimpleAssetAccessor>(std::move(mockCompletedRequests));
  TilesetExternals tilesetExternals{
      mockAssetAccessor,
      std::make_shared<SimplePrepareRendererResource>(),
      AsyncSystem(std::make_shared<SimpleTaskProcessor>()),
      nullptr};

  // create tileset and call updateView() to give it a chance to load
  Tileset tileset(tilesetExternals, "tileset.json");
  initializeTileset(tileset);

  // Load until complete
  ViewUpdateResult updateResult;
  ViewState viewState = zoomToTileset(tileset);
  while (tileset.getNumberOfTilesLoaded() == 0 ||
         tileset.computeLoadProgress() < 100.0f) {
    updateResult = tileset.updateView({viewState});
  }

  // All three tiles (plus the tileset.json) should be rendered.
  CHECK(updateResult.tilesToRenderThisFrame.size() == 4);

  // Zoom way out
  std::optional<Cartographic> position = viewState.getPositionCartographic();
  REQUIRE(position);
  position->height += 100000;

  ViewState zoomedOut = ViewState::create(
      Ellipsoid::WGS84.cartographicToCartesian(*position),
      viewState.getDirection(),
      viewState.getUp(),
      viewState.getViewportSize(),
      viewState.getHorizontalFieldOfView(),
      viewState.getVerticalFieldOfView(),
      Ellipsoid::WGS84);
  updateResult = tileset.updateView({zoomedOut});

  // Only the root tile (plus the tileset.json) is visible now, and the other
  // two are fading out.
  CHECK(updateResult.tilesToRenderThisFrame.size() == 2);
  CHECK(updateResult.tilesFadingOut.size() == 2);
}
