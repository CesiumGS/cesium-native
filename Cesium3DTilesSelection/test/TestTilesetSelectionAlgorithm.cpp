#include "Cesium3DTilesSelection/Tileset.h"
#include "Cesium3DTilesSelection/ViewState.h"
#include "Cesium3DTilesSelection/registerAllTileContentTypes.h"
#include "SimpleAssetAccessor.h"
#include "SimpleAssetRequest.h"
#include "SimpleAssetResponse.h"
#include "SimplePrepareRendererResource.h"
#include "SimpleTaskProcessor.h"

#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumUtility/Math.h>

#include <catch2/catch.hpp>
#include <glm/mat4x4.hpp>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>

using namespace CesiumAsync;
using namespace Cesium3DTilesSelection;
using namespace CesiumGeospatial;
using namespace CesiumUtility;

static std::vector<std::byte> readFile(const std::filesystem::path& fileName) {
  std::ifstream file(fileName, std::ios::binary | std::ios::ate);
  REQUIRE(file);

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<std::byte> buffer(static_cast<size_t>(size));
  file.read(reinterpret_cast<char*>(buffer.data()), size);

  return buffer;
}

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
      verticalFieldOfView);

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
      verticalFieldOfView);
}

static ViewState zoomToTileset(const Tileset& tileset) {
  const Tile* root = tileset.getRootTile();
  REQUIRE(root != nullptr);

  return zoomToTile(*root);
}

TEST_CASE("Test replace refinement for render") {
  Cesium3DTilesSelection::registerAllTileContentTypes();

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
  const Tile* root = tileset.getRootTile();
  REQUIRE(root->getState() == TileLoadState::ContentLoading);
  for (const auto& child : root->getChildren()) {
    REQUIRE(child.getState() == TileLoadState::Unloaded);
  }

  SECTION("No refinement happen when tile meet SSE") {
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
        viewState.getVerticalFieldOfView());

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

      REQUIRE(result.tilesVisited == 1);
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

  SECTION("Root doesn't meet sse but has to be rendered because children "
          "cannot be rendered") {
    // we should forbid hole first to let the checks below happen
    tileset.getOptions().forbidHoles = true;

    SECTION("Children cannot be rendered because of no response") {
      // remove one of children completed response to mock network error
      mockAssetAccessor->mockCompletedRequests["ll.b3dm"]->pResponse = nullptr;
      mockAssetAccessor->mockCompletedRequests["lr.b3dm"]->pResponse = nullptr;
      mockAssetAccessor->mockCompletedRequests["ul.b3dm"]->pResponse = nullptr;
      mockAssetAccessor->mockCompletedRequests["ur.b3dm"]->pResponse = nullptr;
    }

    SECTION("Children cannot be rendered because response has an failed status "
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

      REQUIRE(result.tilesVisited == 5);
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

      REQUIRE(result.tilesVisited == 5);
      REQUIRE(result.workerThreadTileLoadQueueLength == 0);
      REQUIRE(result.tilesCulled == 0);
      REQUIRE(result.culledTilesVisited == 0);
    }
  }

  SECTION("Parent meets sse but not renderable") {
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
        viewState.getVerticalFieldOfView());

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

      REQUIRE(result.tilesVisited == 6);
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

      REQUIRE(result.tilesVisited == 6);
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
          viewState.getVerticalFieldOfView());

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

      REQUIRE(result.tilesVisited == 5);
      REQUIRE(result.workerThreadTileLoadQueueLength == 0);
      REQUIRE(result.tilesCulled == 0);
      REQUIRE(result.culledTilesVisited == 0);
    }
  }

  SECTION("Child should be chosen when parent doesn't meet SSE") {
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

      REQUIRE(result.tilesVisited == 5);
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

      REQUIRE(result.tilesVisited == 5);
      REQUIRE(result.workerThreadTileLoadQueueLength == 0);
      REQUIRE(result.tilesCulled == 0);
      REQUIRE(result.culledTilesVisited == 0);
    }
  }
}

TEST_CASE("Test additive refinement") {
  Cesium3DTilesSelection::registerAllTileContentTypes();

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
  const Tile* root = tileset.getRootTile();
  REQUIRE(root->getState() == TileLoadState::ContentLoading);
  REQUIRE(root->getChildren().size() == 0);

  SECTION("Load external tilesets") {
    ViewState viewState = zoomToTileset(tileset);

    // 1st frame. Root will get rendered first and 5 of its children are loading
    // since they meet sse
    {
      ViewUpdateResult result = tileset.updateView({viewState});

      // root is rendered first
      REQUIRE(root->getState() == TileLoadState::Done);
      REQUIRE(!doesTileMeetSSE(viewState, *root, tileset));
      REQUIRE(root->getChildren().size() == 1);

      // root's children don't have content loading right now, so only root get
      // rendered
      const Tile& parentB3DM = root->getChildren().front();
      REQUIRE(parentB3DM.getState() == TileLoadState::ContentLoading);
      REQUIRE(!doesTileMeetSSE(viewState, parentB3DM, tileset));
      REQUIRE(parentB3DM.getChildren().size() == 4);

      for (const Tile& child : parentB3DM.getChildren()) {
        REQUIRE(child.getState() == TileLoadState::ContentLoading);
        REQUIRE(doesTileMeetSSE(viewState, child, tileset));
      }

      REQUIRE(result.tilesToRenderThisFrame.size() == 1);
      REQUIRE(result.tilesToRenderThisFrame.front() == root);

      REQUIRE(result.tilesFadingOut.size() == 0);

      REQUIRE(result.tilesVisited == 6);
      REQUIRE(result.workerThreadTileLoadQueueLength == 5);
      REQUIRE(result.tilesCulled == 0);
      REQUIRE(result.culledTilesVisited == 0);
    }

    // 2nd frame
    {
      ViewUpdateResult result = tileset.updateView({viewState});

      // root is rendered first
      REQUIRE(root->getState() == TileLoadState::Done);
      REQUIRE(!doesTileMeetSSE(viewState, *root, tileset));
      REQUIRE(root->getChildren().size() == 1);

      // root's children don't have content loading right now, so only root get
      // rendered
      const Tile& parentB3DM = root->getChildren().front();
      REQUIRE(parentB3DM.getState() == TileLoadState::Done);
      REQUIRE(!doesTileMeetSSE(viewState, parentB3DM, tileset));
      REQUIRE(parentB3DM.getChildren().size() == 4);

      for (const Tile& child : parentB3DM.getChildren()) {
        REQUIRE(child.getState() == TileLoadState::Done);

        if (*std::get_if<std::string>(&child.getTileID()) !=
            "tileset3/tileset3.json") {
          REQUIRE(doesTileMeetSSE(viewState, child, tileset));
        } else {
          // external tilesets get unconditionally refined
          REQUIRE(root->getUnconditionallyRefine());

          // expect the children to meet sse and begin loading the content
          REQUIRE(child.getChildren().size() == 1);
          REQUIRE(
              doesTileMeetSSE(viewState, child.getChildren().front(), tileset));
          REQUIRE(
              child.getChildren().front().getState() ==
              TileLoadState::ContentLoading);
        }
      }

      REQUIRE(result.tilesToRenderThisFrame.size() == 2);
      REQUIRE(result.tilesToRenderThisFrame[0] == root);
      REQUIRE(result.tilesToRenderThisFrame[1] == &parentB3DM);

      REQUIRE(result.tilesFadingOut.size() == 0);

      REQUIRE(result.tilesVisited == 7);
      REQUIRE(result.workerThreadTileLoadQueueLength == 1);
      REQUIRE(result.tilesCulled == 0);
      REQUIRE(result.culledTilesVisited == 0);
    }

    // 3rd frame. All the children finish loading. All should be rendered now
    {
      ViewUpdateResult result = tileset.updateView({viewState});

      REQUIRE(result.tilesToRenderThisFrame.size() == 7);

      REQUIRE(result.tilesFadingOut.size() == 0);

      REQUIRE(result.tilesVisited == 7);
      REQUIRE(result.workerThreadTileLoadQueueLength == 0);
      REQUIRE(result.tilesCulled == 0);
      REQUIRE(result.culledTilesVisited == 0);
    }
  }
}

TEST_CASE("Render any tiles even when one of children can't be rendered for "
          "additive refinement") {
  Cesium3DTilesSelection::registerAllTileContentTypes();

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

  Tile* root = tileset.getRootTile();
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

    REQUIRE(result.tilesToRenderThisFrame.size() == 1);
    REQUIRE(result.tilesFadingOut.size() == 0);
    REQUIRE(result.tilesVisited == 4);
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

    REQUIRE(result.tilesToRenderThisFrame.size() == 4);
    REQUIRE(result.tilesFadingOut.size() == 0);
    REQUIRE(result.tilesVisited == 4);
    REQUIRE(result.workerThreadTileLoadQueueLength == 0);
    REQUIRE(result.tilesCulled == 0);
    REQUIRE(result.culledTilesVisited == 0);
  }
}

TEST_CASE("Test multiple frustums") {
  Cesium3DTilesSelection::registerAllTileContentTypes();

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
  const Tile* root = tileset.getRootTile();
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
      viewState.getVerticalFieldOfView());

  SECTION("The frustum with the highest SSE should be used for deciding to "
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

      REQUIRE(result.tilesVisited == 5);
      REQUIRE(result.workerThreadTileLoadQueueLength == 0);
      REQUIRE(result.tilesCulled == 0);
    }
  }

  SECTION("Tiles should be culled if all the cameras agree") {

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
        0.5 * zoomToTileViewState.getVerticalFieldOfView());

    zoomInPosition = zoomToTileViewState.getPosition() +
                     glm::dvec3(15.0, 0, 0) +
                     zoomToTileViewState.getDirection() * 243.0;
    ViewState zoomInViewState2 = ViewState::create(
        zoomInPosition,
        zoomToTileViewState.getDirection(),
        zoomToTileViewState.getUp(),
        zoomToTileViewState.getViewportSize(),
        0.5 * zoomToTileViewState.getHorizontalFieldOfView(),
        0.5 * zoomToTileViewState.getVerticalFieldOfView());

    // frame 3 & 4
    {
      tileset.updateView({zoomInViewState1, zoomInViewState2});
      ViewUpdateResult result =
          tileset.updateView({zoomInViewState1, zoomInViewState2});

      // check result
      // The grand child and the second child are the only ones rendered.
      // The third and fourth children of the root are culled.
      REQUIRE(result.tilesToRenderThisFrame.size() == 2);
      REQUIRE(result.tilesVisited == 4);
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

  const Tile* pRoot = tileset.getRootTile();
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
          10.0));
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
          TileLoadResult::createFailedResult(nullptr));
    }

    virtual TileChildrenResult createTileChildren(const Tile&) override {
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

  // On the first update, we should render the root tile, even though nothing is
  // loaded yet.
  initializeTileset(tileset);
  CHECK(
      tileset.getRootTile()->getLastSelectionState().getResult(
          tileset.getRootTile()->getLastSelectionState().getFrameNumber()) ==
      TileSelectionState::Result::Rendered);

  // On the third update, the grandchild load will still be pending.
  // But the child is unconditionally refined, so we should render the root
  // instead of the child.
  initializeTileset(tileset);
  initializeTileset(tileset);
  const Tile& child = tileset.getRootTile()->getChildren()[0];
  const Tile& grandchild = child.getChildren()[0];
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
  SECTION("With default settings") {
    runUnconditionallyRefinedTestCase(TilesetOptions());
  }

  SECTION("With forbidHoles enabled") {
    TilesetOptions options{};
    options.forbidHoles = true;
    runUnconditionallyRefinedTestCase(options);
  }
}
