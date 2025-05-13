#include <Cesium3DTilesSelection/EllipsoidTilesetLoader.h>
#include <Cesium3DTilesSelection/Tileset.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>

#include <variant>

using namespace Cesium3DTilesSelection;
using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumNativeTests;
using namespace CesiumUtility;

namespace {

// Get the maximum level of any of the tiles in the list
uint32_t getMaxLevel(const std::vector<Tile::Pointer>& tiles) {
  uint32_t result = 0;

  for (const Tile::Pointer& pTile : tiles) {
    const QuadtreeTileID* pID =
        std::get_if<QuadtreeTileID>(&pTile->getTileID());
    if (pID != nullptr) {
      result = std::max(result, pID->level);
    }
  }

  return result;
}

} // namespace

TEST_CASE("TilesetViewGroup") {
  SUBCASE("used with Tileset") {
    TilesetExternals externals{
        .pAssetAccessor = std::make_shared<SimpleAssetAccessor>(
            std::map<std::string, std::shared_ptr<SimpleAssetRequest>>()),
        .pPrepareRendererResources = nullptr,
        .asyncSystem = AsyncSystem(std::make_shared<SimpleTaskProcessor>()),
        .pCreditSystem = std::make_shared<CreditSystem>(),
    };

    TilesetOptions options;
    options.credit = "Yay!";
    auto pTileset = EllipsoidTilesetLoader::createTileset(externals, options);

    SUBCASE("can view the globe at different LODs") {
      std::vector<ViewState> farFrustums{ViewState(
          glm::dvec3(Ellipsoid::WGS84.getMaximumRadius() * 1.2, 0.0, 0.0),
          glm::dvec3(-1.0, 0.0, 0.0),
          glm::dvec3(0.0, 0.0, 1.0),
          glm::dvec2(1024.0, 1024.0),
          Math::degreesToRadians(60.0),
          Math::degreesToRadians(60.0))};

      std::vector<ViewState> nearFrustums{ViewState(
          glm::dvec3(Ellipsoid::WGS84.getMaximumRadius() * 1.1, 0.0, 0.0),
          glm::dvec3(-1.0, 0.0, 0.0),
          glm::dvec3(0.0, 0.0, 1.0),
          glm::dvec2(1024.0, 1024.0),
          Math::degreesToRadians(60.0),
          Math::degreesToRadians(60.0))};

      TilesetViewGroup farGroup, nearGroup;

      while (farGroup.getPreviousLoadProgressPercentage() < 100.0f ||
             nearGroup.getPreviousLoadProgressPercentage() < 100.0f) {
        pTileset->updateViewGroup(farGroup, farFrustums, 0.0);
        pTileset->updateViewGroup(nearGroup, nearFrustums, 0.0);
        pTileset->loadTiles();
        externals.asyncSystem.dispatchMainThreadTasks();
      }

      std::vector<Tile::Pointer> farViewTiles;

      {
        const ViewUpdateResult& farResult = farGroup.getViewUpdateResult();
        const ViewUpdateResult& nearResult = nearGroup.getViewUpdateResult();

        CHECK(
            farResult.tilesToRenderThisFrame.size() !=
            nearResult.tilesToRenderThisFrame.size());
        CHECK(
            getMaxLevel(farResult.tilesToRenderThisFrame) <
            getMaxLevel(nearResult.tilesToRenderThisFrame));

        farViewTiles = farResult.tilesToRenderThisFrame;
      }

      {
        const ViewUpdateResult& farResult =
            pTileset->updateViewGroup(farGroup, farFrustums, 0.0);
        const ViewUpdateResult& nearResult =
            pTileset->updateViewGroup(nearGroup, nearFrustums, 0.0);

        CHECK(farResult.tilesFadingOut.empty());
        CHECK(nearResult.tilesFadingOut.empty());
      }

      {
        // Move the near view in closer.
        nearFrustums.clear();
        nearFrustums.emplace_back(ViewState(
            glm::dvec3(Ellipsoid::WGS84.getMaximumRadius() * 1.08, 0.0, 0.0),
            glm::dvec3(-1.0, 0.0, 0.0),
            glm::dvec3(0.0, 0.0, 1.0),
            glm::dvec2(1024.0, 1024.0),
            Math::degreesToRadians(60.0),
            Math::degreesToRadians(60.0)));

        const ViewUpdateResult& farResult =
            pTileset->updateViewGroup(farGroup, farFrustums, 0.0);
        const ViewUpdateResult& nearResult =
            pTileset->updateViewGroup(nearGroup, nearFrustums, 0.0);

        // The far view shouldn't change
        CHECK(farResult.tilesFadingOut.empty());
        CHECK(farViewTiles == farResult.tilesToRenderThisFrame);

        // The near view should have some tile selection changes.
        CHECK(!nearResult.tilesFadingOut.empty());
        CHECK(
            getMaxLevel(farResult.tilesToRenderThisFrame) <
            getMaxLevel(nearResult.tilesToRenderThisFrame));
      }
    }
  }
}
