#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTiles/ViewState.h"
#include "Cesium3DTiles/registerAllTileContentTypes.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumUtility/Math.h"
#include "SimpleAssetAccessor.h"
#include "SimpleAssetRequest.h"
#include "SimpleAssetResponse.h"
#include "SimplePrepareRendererResource.h"
#include "SimpleTaskProcessor.h"
#include "catch2/catch.hpp"
#include "glm/mat4x4.hpp"
#include <cstddef>
#include <filesystem>
#include <fstream>

using namespace CesiumAsync;
using namespace Cesium3DTiles;
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

  tileset.updateView(viewState);
}

static ViewState zoomToTileset(const Tileset& tileset) {
  const Tile* root = tileset.getRootTile();
  REQUIRE(root != nullptr);

  const BoundingRegion* region =
      std::get_if<BoundingRegion>(&root->getBoundingVolume());
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

TEST_CASE("Test replace refinement for render") {
  Cesium3DTiles::registerAllTileContentTypes();

  // initialize REPLACE tileset
  //
  //				   parent.b3dm
  //
  // ll.b3dm		lr.b3dm		ul.b3dm		ur.b3dm
  //
  // ll_ll.b3dm
  //
  std::filesystem::path testDataPath = Cesium3DTiles_TEST_DATA_DIR;
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
      std::make_shared<SimpleTaskProcessor>(),
      nullptr};

  // create tileset and call updateView() to give it a chance to load
  Tileset tileset(tilesetExternals, "tileset.json");
  initializeTileset(tileset);

  // check the tiles status
  const Tile* root = tileset.getRootTile();
  REQUIRE(root->getState() == Tile::LoadState::ContentLoading);
  for (const auto& child : root->getChildren()) {
    REQUIRE(child.getState() == Tile::LoadState::Unloaded);
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
      ViewUpdateResult result = tileset.updateView(zoomOutViewState);

      // Check tile state. Ensure root meet sse
      REQUIRE(root->getState() == Tile::LoadState::Done);
      REQUIRE(doesTileMeetSSE(zoomOutViewState, *root, tileset));
      for (const auto& child : root->getChildren()) {
        REQUIRE(child.getState() == Tile::LoadState::Unloaded);
      }

      // check result
      REQUIRE(result.tilesToRenderThisFrame.size() == 1);
      REQUIRE(result.tilesToRenderThisFrame.front() == root);

      REQUIRE(result.tilesToNoLongerRenderThisFrame.size() == 0);

      REQUIRE(result.tilesVisited == 1);
      REQUIRE(result.tilesLoadingMediumPriority == 0);
      REQUIRE(result.tilesCulled == 0);
      REQUIRE(result.culledTilesVisited == 0);

      // check children state are still unloaded
      for (const auto& child : root->getChildren()) {
        REQUIRE(child.getState() == Tile::LoadState::Unloaded);
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
      ViewUpdateResult result = tileset.updateView(viewState);

      // Check tile state. Ensure root doesn't meet sse, but children does.
      // Children begin loading as well
      REQUIRE(root->getState() == Tile::LoadState::Done);
      REQUIRE(!doesTileMeetSSE(viewState, *root, tileset));
      for (const auto& child : root->getChildren()) {
        REQUIRE(child.getState() == Tile::LoadState::ContentLoading);
        REQUIRE(doesTileMeetSSE(viewState, child, tileset));
      }

      // check result
      REQUIRE(result.tilesToRenderThisFrame.size() == 1);
      REQUIRE(result.tilesToRenderThisFrame.front() == root);

      REQUIRE(result.tilesToNoLongerRenderThisFrame.size() == 0);

      REQUIRE(result.tilesVisited == 1);
      REQUIRE(result.tilesLoadingMediumPriority == 4);
      REQUIRE(result.tilesCulled == 0);
      REQUIRE(result.culledTilesVisited == 0);
    }

    // 2nd frame. Because children receive failed response, so they can't be
    // rendered. Root should be rendered instead. Children should have failed
    // load states
    {
      ViewUpdateResult result = tileset.updateView(viewState);

      // Check tile state. Ensure root doesn't meet sse, but children does
      REQUIRE(root->getState() == Tile::LoadState::Done);
      REQUIRE(!doesTileMeetSSE(viewState, *root, tileset));
      for (const auto& child : root->getChildren()) {
        REQUIRE(child.getState() == Tile::LoadState::FailedTemporarily);
        REQUIRE(doesTileMeetSSE(viewState, child, tileset));
      }

      // check result
      REQUIRE(result.tilesToRenderThisFrame.size() == 1);
      REQUIRE(result.tilesToRenderThisFrame.front() == root);

      REQUIRE(result.tilesToNoLongerRenderThisFrame.size() == 0);

      REQUIRE(result.tilesVisited == 1);
      REQUIRE(result.tilesLoadingLowPriority == 0);
      REQUIRE(result.tilesLoadingMediumPriority == 0);
      REQUIRE(result.tilesLoadingHighPriority == 0);
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
      ViewUpdateResult result = tileset.updateView(zoomInViewState);

      // check tiles status. All the children should have loading status
      REQUIRE(root->getState() == Tile::LoadState::Done);
      REQUIRE(!doesTileMeetSSE(zoomInViewState, *root, tileset));
      for (const auto& child : root->getChildren()) {
        REQUIRE(child.getState() == Tile::LoadState::ContentLoading);
      }

      const Tile& ll = root->getChildren().front();
      REQUIRE(!doesTileMeetSSE(zoomInViewState, ll, tileset));

      const Tile& ll_ll = ll.getChildren().front();
      REQUIRE(ll_ll.getState() == Tile::LoadState::ContentLoading);
      REQUIRE(doesTileMeetSSE(zoomInViewState, ll_ll, tileset));

      // check result
      REQUIRE(result.tilesToRenderThisFrame.size() == 1);
      REQUIRE(result.tilesToRenderThisFrame.front() == root);

      REQUIRE(result.tilesToNoLongerRenderThisFrame.size() == 0);

      REQUIRE(result.tilesVisited == 6);
      REQUIRE(result.tilesLoadingLowPriority == 1);
      REQUIRE(result.tilesLoadingMediumPriority == 4);
      REQUIRE(result.tilesLoadingHighPriority == 0);
      REQUIRE(result.tilesCulled == 0);
      REQUIRE(result.culledTilesVisited == 0);
    }

    // 2nd frame. All the children finish loading, so they are ready to be
    // rendered (except ll.b3dm tile since it doesn't meet sse)
    {
      ViewUpdateResult result = tileset.updateView(zoomInViewState);

      // check tiles status. All the children should have loading status
      REQUIRE(root->getState() == Tile::LoadState::Done);
      REQUIRE(!doesTileMeetSSE(zoomInViewState, *root, tileset));

      // the first child of root isn't rendered because it doesn't meet sse. It
      // will be refined to its child which is ready to be rendered
      const Tile& ll = root->getChildren().front();
      REQUIRE(ll.getState() == Tile::LoadState::Failed);
      REQUIRE(!doesTileMeetSSE(zoomInViewState, ll, tileset));

      const Tile& ll_ll = ll.getChildren().front();
      REQUIRE(ll_ll.getState() == Tile::LoadState::Done);
      REQUIRE(doesTileMeetSSE(zoomInViewState, ll_ll, tileset));

      // the rest of the root's children are ready to be rendered since they all
      // meet sse
      for (size_t i = 1; i < root->getChildren().size(); ++i) {
        const Tile& child = root->getChildren()[i];
        REQUIRE(child.getState() == Tile::LoadState::Done);
        REQUIRE(doesTileMeetSSE(zoomInViewState, child, tileset));
      }

      // check result
      REQUIRE(result.tilesToRenderThisFrame.size() == 4);
      REQUIRE(result.tilesToRenderThisFrame[0] == &ll_ll);
      REQUIRE(result.tilesToRenderThisFrame[1] == &root->getChildren()[1]);
      REQUIRE(result.tilesToRenderThisFrame[2] == &root->getChildren()[2]);
      REQUIRE(result.tilesToRenderThisFrame[3] == &root->getChildren()[3]);

      REQUIRE(result.tilesToNoLongerRenderThisFrame.size() == 1);
      REQUIRE(result.tilesToNoLongerRenderThisFrame.front() == root);

      REQUIRE(result.tilesVisited == 6);
      REQUIRE(result.tilesLoadingLowPriority == 0);
      REQUIRE(result.tilesLoadingMediumPriority == 0);
      REQUIRE(result.tilesLoadingHighPriority == 0);
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

      ViewUpdateResult result = tileset.updateView(zoomOutViewState);

      // check tiles status. All the children should have loading status
      REQUIRE(root->getState() == Tile::LoadState::Done);
      REQUIRE(!doesTileMeetSSE(zoomOutViewState, *root, tileset));

      // The first child of root isn't rendered because it is failed to load
      // even though it meets sse. Use its child instead since the child is
      // rendered last frame
      const Tile& ll = root->getChildren().front();
      REQUIRE(ll.getState() == Tile::LoadState::Failed);
      REQUIRE(doesTileMeetSSE(zoomOutViewState, ll, tileset));

      const Tile& ll_ll = ll.getChildren().front();
      REQUIRE(ll_ll.getState() == Tile::LoadState::Done);

      // the rest of the root's children are ready to be rendered since they all
      // meet sse
      for (size_t i = 1; i < root->getChildren().size(); ++i) {
        const Tile& child = root->getChildren()[i];
        REQUIRE(child.getState() == Tile::LoadState::Done);
        REQUIRE(doesTileMeetSSE(zoomOutViewState, child, tileset));
      }

      // check result
      REQUIRE(result.tilesToRenderThisFrame.size() == 4);
      REQUIRE(result.tilesToRenderThisFrame[0] == &ll_ll);
      REQUIRE(result.tilesToRenderThisFrame[1] == &root->getChildren()[1]);
      REQUIRE(result.tilesToRenderThisFrame[2] == &root->getChildren()[2]);
      REQUIRE(result.tilesToRenderThisFrame[3] == &root->getChildren()[3]);

      REQUIRE(result.tilesToNoLongerRenderThisFrame.size() == 0);

      REQUIRE(result.tilesVisited == 6);
      REQUIRE(result.tilesLoadingLowPriority == 0);
      REQUIRE(result.tilesLoadingMediumPriority == 0);
      REQUIRE(result.tilesLoadingHighPriority == 0);
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
      ViewUpdateResult result = tileset.updateView(viewState);

      // Check tile state. Ensure root doesn't meet sse, but children does
      REQUIRE(root->getState() == Tile::LoadState::Done);
      REQUIRE(!doesTileMeetSSE(viewState, *root, tileset));
      for (const auto& child : root->getChildren()) {
        REQUIRE(child.getState() == Tile::LoadState::ContentLoading);
        REQUIRE(doesTileMeetSSE(viewState, child, tileset));
      }

      // check result
      REQUIRE(result.tilesToRenderThisFrame.size() == 1);
      REQUIRE(result.tilesToRenderThisFrame.front() == root);

      REQUIRE(result.tilesToNoLongerRenderThisFrame.size() == 0);

      REQUIRE(result.tilesVisited == 5);
      REQUIRE(result.tilesLoadingLowPriority == 0);
      REQUIRE(result.tilesLoadingMediumPriority == 4);
      REQUIRE(result.tilesLoadingHighPriority == 0);
      REQUIRE(result.tilesCulled == 0);
      REQUIRE(result.culledTilesVisited == 0);
    }

    // 2nd frame. Children are finished loading and ready to be rendered. Root
    // shouldn't be rendered in this frame
    {
      ViewUpdateResult result = tileset.updateView(viewState);

      // check tile states
      REQUIRE(root->getState() == Tile::LoadState::Done);
      for (const auto& child : root->getChildren()) {
        REQUIRE(child.getState() == Tile::LoadState::Done);
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

      REQUIRE(result.tilesToNoLongerRenderThisFrame.size() == 1);
      REQUIRE(result.tilesToNoLongerRenderThisFrame.front() == root);

      REQUIRE(result.tilesVisited == 5);
      REQUIRE(result.tilesLoadingLowPriority == 0);
      REQUIRE(result.tilesLoadingMediumPriority == 0);
      REQUIRE(result.tilesLoadingHighPriority == 0);
      REQUIRE(result.tilesCulled == 0);
      REQUIRE(result.culledTilesVisited == 0);
    }
  }
}

TEST_CASE("Test additive refinement") {
  Cesium3DTiles::registerAllTileContentTypes();

  std::filesystem::path testDataPath = Cesium3DTiles_TEST_DATA_DIR;
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
      std::make_shared<SimpleTaskProcessor>(),
      nullptr};

  // create tileset and call updateView() to give it a chance to load
  Tileset tileset(tilesetExternals, "tileset.json");
  initializeTileset(tileset);

  // root is external tileset. Since its content is loading, we won't know if it
  // has children or not
  const Tile* root = tileset.getRootTile();
  REQUIRE(root->getState() == Tile::LoadState::ContentLoading);
  REQUIRE(root->getChildren().size() == 0);

  SECTION("Load external tilesets") {
    ViewState viewState = zoomToTileset(tileset);

    // 1st frame. Root will get rendered first and 5 of its children are loading
    // since they meet sse
    {
      ViewUpdateResult result = tileset.updateView(viewState);

      // root is rendered first
      REQUIRE(root->getState() == Tile::LoadState::Done);
      REQUIRE(!doesTileMeetSSE(viewState, *root, tileset));
      REQUIRE(root->getChildren().size() == 1);

      // root's children don't have content loading right now, so only root get
      // rendered
      const Tile& parentB3DM = root->getChildren().front();
      REQUIRE(parentB3DM.getState() == Tile::LoadState::ContentLoading);
      REQUIRE(!doesTileMeetSSE(viewState, parentB3DM, tileset));
      REQUIRE(parentB3DM.getChildren().size() == 4);

      for (const Tile& child : parentB3DM.getChildren()) {
        REQUIRE(child.getState() == Tile::LoadState::ContentLoading);
        REQUIRE(doesTileMeetSSE(viewState, child, tileset));
      }

      REQUIRE(result.tilesToRenderThisFrame.size() == 1);
      REQUIRE(result.tilesToRenderThisFrame.front() == root);

      REQUIRE(result.tilesToNoLongerRenderThisFrame.size() == 0);

      REQUIRE(result.tilesVisited == 6);
      REQUIRE(result.tilesLoadingLowPriority == 0);
      REQUIRE(result.tilesLoadingMediumPriority == 5);
      REQUIRE(result.tilesLoadingHighPriority == 0);
      REQUIRE(result.tilesCulled == 0);
      REQUIRE(result.culledTilesVisited == 0);
    }

    // 2nd frame
    {
      ViewUpdateResult result = tileset.updateView(viewState);

      // root is rendered first
      REQUIRE(root->getState() == Tile::LoadState::Done);
      REQUIRE(!doesTileMeetSSE(viewState, *root, tileset));
      REQUIRE(root->getChildren().size() == 1);

      // root's children don't have content loading right now, so only root get
      // rendered
      const Tile& parentB3DM = root->getChildren().front();
      REQUIRE(parentB3DM.getState() == Tile::LoadState::Done);
      REQUIRE(!doesTileMeetSSE(viewState, parentB3DM, tileset));
      REQUIRE(parentB3DM.getChildren().size() == 4);

      for (const Tile& child : parentB3DM.getChildren()) {
        REQUIRE(child.getState() == Tile::LoadState::Done);

        if (*std::get_if<std::string>(&child.getTileID()) !=
            "tileset3/tileset3.json") {
          REQUIRE(doesTileMeetSSE(viewState, child, tileset));
        } else {
          // external tileset has always geometric error over 999999, so it
          // won't meet sse
          REQUIRE(!doesTileMeetSSE(viewState, child, tileset));

          // expect the children to meet sse and begin loading the content
          REQUIRE(child.getChildren().size() == 1);
          REQUIRE(
              doesTileMeetSSE(viewState, child.getChildren().front(), tileset));
          REQUIRE(
              child.getChildren().front().getState() ==
              Tile::LoadState::ContentLoading);
        }
      }

      REQUIRE(result.tilesToRenderThisFrame.size() == 2);
      REQUIRE(result.tilesToRenderThisFrame[0] == root);
      REQUIRE(result.tilesToRenderThisFrame[1] == &parentB3DM);

      REQUIRE(result.tilesToNoLongerRenderThisFrame.size() == 0);

      REQUIRE(result.tilesVisited == 7);
      REQUIRE(result.tilesLoadingLowPriority == 0);
      REQUIRE(result.tilesLoadingMediumPriority == 1);
      REQUIRE(result.tilesLoadingHighPriority == 0);
      REQUIRE(result.tilesCulled == 0);
      REQUIRE(result.culledTilesVisited == 0);
    }

    // 3rd frame. All the children finish loading. All should be rendered now
    {
      ViewUpdateResult result = tileset.updateView(viewState);

      REQUIRE(result.tilesToRenderThisFrame.size() == 7);

      REQUIRE(result.tilesToNoLongerRenderThisFrame.size() == 0);

      REQUIRE(result.tilesVisited == 7);
      REQUIRE(result.tilesLoadingLowPriority == 0);
      REQUIRE(result.tilesLoadingMediumPriority == 0);
      REQUIRE(result.tilesLoadingHighPriority == 0);
      REQUIRE(result.tilesCulled == 0);
      REQUIRE(result.culledTilesVisited == 0);
    }
  }
}
