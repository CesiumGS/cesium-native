#include "LayerJsonTerrainLoader.h"
#include <Cesium3DTilesSelection/registerAllTileContentTypes.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumUtility/Math.h>
#include "SimplePrepareRendererResource.h"
#include "SimpleAssetAccessor.h"
#include "SimpleAssetRequest.h"
#include "SimpleAssetResponse.h"
#include "SimpleTaskProcessor.h"
#include "readFile.h"
#include <catch2/catch.hpp>
#include <filesystem>

using namespace Cesium3DTilesSelection;
using namespace CesiumGeospatial;
using namespace CesiumGeometry;
using namespace CesiumAsync;
using namespace CesiumUtility;

namespace {
std::filesystem::path testDataPath = Cesium3DTilesSelection_TEST_DATA_DIR;

std::shared_ptr<SimpleAssetRequest>
createMockAssetRequest(const std::filesystem::path& requestContentPath) {
  auto pMockResponse = std::make_unique<SimpleAssetResponse>(
      static_cast<uint16_t>(200),
      "doesn't matter",
      CesiumAsync::HttpHeaders{},
      readFile(requestContentPath));
  auto pMockRequest = std::make_shared<SimpleAssetRequest>(
      "GET",
      "doesn't matter",
      CesiumAsync::HttpHeaders{},
      std::move(pMockResponse));

  return pMockRequest;
}
} // namespace

TEST_CASE("Test layer json terrain loader") {
  Cesium3DTilesSelection::registerAllTileContentTypes();

  auto pMockedAssetAccessor = std::make_shared<SimpleAssetAccessor>(
      std::map<std::string, std::shared_ptr<SimpleAssetRequest>>{});

  auto pMockedPrepareRendererResources =
      std::make_shared<SimplePrepareRendererResource>();

  CesiumAsync::AsyncSystem asyncSystem{std::make_shared<SimpleTaskProcessor>()};

  auto pMockedCreditSystem = std::make_shared<CreditSystem>();

  TilesetExternals externals{
      pMockedAssetAccessor,
      pMockedPrepareRendererResources,
      asyncSystem,
      pMockedCreditSystem};

  SECTION("Create layer json loader") {
    auto layerJsonPath =
        testDataPath / "CesiumTerrainTileJson" / "QuantizedMesh.tile.json";
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"layer.json", createMockAssetRequest(layerJsonPath)});

    auto loaderFuture = LayerJsonTerrainLoader::createLoader(
        externals,
        {},
        "layer.json",
        {},
        true);

    asyncSystem.dispatchMainThreadTasks();

    auto loaderResult = loaderFuture.wait();
    CHECK(loaderResult.pLoader);
    CHECK(loaderResult.pRootTile);

    // check root tile
    const Tile& rootTile = *loaderResult.pRootTile;
    const auto& rootLooseRegion = std::get<BoundingRegionWithLooseFittingHeights>(
        rootTile.getBoundingVolume());
    const auto& rootRegion = rootLooseRegion.getBoundingRegion();
    CHECK(rootTile.isEmptyContent());
    CHECK(rootTile.getUnconditionallyRefine());
    CHECK(rootTile.getRefine() == TileRefine::Replace);
    CHECK(rootRegion.getRectangle().getWest() == Approx(-Math::OnePi));
    CHECK(rootRegion.getRectangle().getEast() == Approx(Math::OnePi));
    CHECK(rootRegion.getRectangle().getSouth() == Approx(-Math::PiOverTwo));
    CHECK(rootRegion.getRectangle().getNorth() == Approx(Math::PiOverTwo));
    CHECK(rootRegion.getMinimumHeight() == -1000.0);
    CHECK(rootRegion.getMaximumHeight() == 9000.0);

    // check children
    const auto& tileChildren = rootTile.getChildren();
    CHECK(tileChildren.size() == 2);

    const Tile& tile_0_0_0 = tileChildren[0];
    const auto& looseRegion_0_0_0 =
        std::get<BoundingRegionWithLooseFittingHeights>(
            tile_0_0_0.getBoundingVolume());
    const auto& region_0_0_0 = looseRegion_0_0_0.getBoundingRegion();
    CHECK(std::get<QuadtreeTileID>(tile_0_0_0.getTileID()) == QuadtreeTileID(0, 0, 0));
    CHECK(tile_0_0_0.getGeometricError() == Approx(616538.71824));
    CHECK(region_0_0_0.getRectangle().getWest() == Approx(-Math::OnePi));
    CHECK(region_0_0_0.getRectangle().getEast() == Approx(0.0));
    CHECK(region_0_0_0.getRectangle().getSouth() == Approx(-Math::PiOverTwo));
    CHECK(region_0_0_0.getRectangle().getNorth() == Approx(Math::PiOverTwo));
    CHECK(region_0_0_0.getMinimumHeight() == -1000.0);
    CHECK(region_0_0_0.getMaximumHeight() == 9000.0);

    const Tile& tile_0_1_0 = tileChildren[1];
    const auto& looseRegion_0_1_0 =
        std::get<BoundingRegionWithLooseFittingHeights>(
            tile_0_1_0.getBoundingVolume());
    const auto& region_0_1_0 = looseRegion_0_1_0.getBoundingRegion();
    CHECK(std::get<QuadtreeTileID>(tile_0_1_0.getTileID()) == QuadtreeTileID(0, 1, 0));
    CHECK(tile_0_1_0.getGeometricError() == Approx(616538.71824));
    CHECK(region_0_1_0.getRectangle().getWest() == Approx(0.0));
    CHECK(region_0_1_0.getRectangle().getEast() == Approx(Math::OnePi));
    CHECK(region_0_1_0.getRectangle().getSouth() == Approx(-Math::PiOverTwo));
    CHECK(region_0_1_0.getRectangle().getNorth() == Approx(Math::PiOverTwo));
    CHECK(region_0_1_0.getMinimumHeight() == -1000.0);
    CHECK(region_0_1_0.getMaximumHeight() == 9000.0);
  }
}
