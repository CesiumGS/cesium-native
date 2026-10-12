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

class GroupSelectorDelegate : public ViewStateMeasureDelegate {
public:
  GroupSelectorDelegate(int64_t group_) : group(group_) {}

  bool isContentVisible(const ViewState&, const Tile& tile) const override {
    return !tile.getGroup() || *tile.getGroup() == group;
  }

  int64_t group;
};

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
  ViewState viewState = ViewState(
      viewPosition,
      glm::normalize(viewFocus - viewPosition),
      viewUp,
      viewPortSize,
      horizontalFieldOfView,
      verticalFieldOfView,
      Ellipsoid::WGS84);
  auto pLayerDelegate = std::make_shared<GroupSelectorDelegate>(1);
  viewState.setMeasureDelegate(pLayerDelegate);
  tileset.getExternals().asyncSystem.dispatchMainThreadTasks();
  tileset.updateViewGroup(tileset.getDefaultViewGroup(), {viewState});
  tileset.getExternals().asyncSystem.dispatchMainThreadTasks();
  tileset.loadTiles();
}

static ViewState zoomToTile(const Tile& tile, int64_t group) {
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
  auto pLayerDelegate = std::make_shared<GroupSelectorDelegate>(group);
  ViewState result{
    viewPosition,
    glm::normalize(viewFocus - viewPosition),
    viewUp,
    viewPortSize,
    horizontalFieldOfView,
    verticalFieldOfView,
    Ellipsoid::WGS84};
  result.setMeasureDelegate(pLayerDelegate);
  return result;
}

static ViewState zoomToTileset(const Tileset& tileset, int64_t group) {
  const Tile* root = tileset.getRootTile();
  REQUIRE(root != nullptr);

  return zoomToTile(*root, group);
}

TEST_CASE("Test additive refinement with groups") {
  Cesium3DTilesContent::registerAllTileContentTypes();

  std::filesystem::path testDataPath = Cesium3DTilesSelection_TEST_DATA_DIR;
  testDataPath = testDataPath / "Groups";
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

  const Tile* groupsRoot = &pTilesetJson->getChildren()[0];
  REQUIRE(groupsRoot->getChildren().size() == 2);
  REQUIRE(groupsRoot->getState() == TileLoadState::ContentLoading);
  
  const Tile* root = &groupsRoot->getChildren()[1];
  REQUIRE(root->getChildren().size() == 0);
  REQUIRE(root->getState() == TileLoadState::Unloaded);

  // First, test that the single tile will load.
  SUBCASE("Load tile in group 0") {
    ViewState viewState = zoomToTileset(tileset, 0);
    {
      tilesetExternals.asyncSystem.dispatchMainThreadTasks();
      ViewUpdateResult result =
          tileset.updateViewGroup(tileset.getDefaultViewGroup(), {viewState});
      tilesetExternals.asyncSystem.dispatchMainThreadTasks();
      tileset.loadTiles();
      const std::vector<Tile::ConstPointer>& ttr =
          result.tilesToRenderThisFrame;
      REQUIRE(ttr.size() == 1);
    }
  }
}
