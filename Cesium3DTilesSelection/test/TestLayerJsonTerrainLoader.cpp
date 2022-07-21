#include "LayerJsonTerrainLoader.h"
#include "SimpleAssetAccessor.h"
#include "SimpleAssetRequest.h"
#include "SimpleAssetResponse.h"
#include "SimplePrepareRendererResource.h"
#include "SimpleTaskProcessor.h"
#include "readFile.h"

#include <Cesium3DTilesSelection/registerAllTileContentTypes.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumUtility/Math.h>

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
      requestContentPath.filename().string(),
      CesiumAsync::HttpHeaders{},
      std::move(pMockResponse));

  return pMockRequest;
}
} // namespace

TEST_CASE("Test create layer json terrain loader") {
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

    // check tiling scheme
    const auto& tilingScheme = loaderResult.pLoader->getTilingScheme();
    CHECK(tilingScheme.getRootTilesX() == 2);
    CHECK(tilingScheme.getRootTilesY() == 1);

    // check projection
    const auto& projection = loaderResult.pLoader->getProjection();
    CHECK(std::holds_alternative<GeographicProjection>(projection));

    // check layer
    const auto& layers = loaderResult.pLoader->getLayers();
    CHECK(layers.size() == 1);
    CHECK(layers[0].version == "1.0.0");
    CHECK(layers[0].tileTemplateUrls.size() == 1);
    CHECK(layers[0].tileTemplateUrls[0] == "{z}/{x}/{y}.terrain?v={version}");
    CHECK(layers[0].availabilityLevels == -1);

    // check root tile
    const Tile& rootTile = *loaderResult.pRootTile;
    const auto& rootLooseRegion =
        std::get<BoundingRegionWithLooseFittingHeights>(
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
    CHECK(
        std::get<QuadtreeTileID>(tile_0_0_0.getTileID()) ==
        QuadtreeTileID(0, 0, 0));
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
    CHECK(
        std::get<QuadtreeTileID>(tile_0_1_0.getTileID()) ==
        QuadtreeTileID(0, 1, 0));
    CHECK(tile_0_1_0.getGeometricError() == Approx(616538.71824));
    CHECK(region_0_1_0.getRectangle().getWest() == Approx(0.0));
    CHECK(region_0_1_0.getRectangle().getEast() == Approx(Math::OnePi));
    CHECK(region_0_1_0.getRectangle().getSouth() == Approx(-Math::PiOverTwo));
    CHECK(region_0_1_0.getRectangle().getNorth() == Approx(Math::PiOverTwo));
    CHECK(region_0_1_0.getMinimumHeight() == -1000.0);
    CHECK(region_0_1_0.getMaximumHeight() == 9000.0);
  }

  SECTION("Load error layer json with empty tiles array") {
    auto layerJsonPath =
        testDataPath / "CesiumTerrainTileJson" / "EmptyTilesArray.tile.json";
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
    CHECK(!loaderResult.pLoader);
    CHECK(!loaderResult.pRootTile);
    CHECK(loaderResult.errors.errors.size() == 1);
    CHECK(
        loaderResult.errors.errors[0] ==
        "Layer Json does not specify any tile URL templates");
  }

  SECTION("Load error layer json with no tiles field") {
    auto layerJsonPath =
        testDataPath / "CesiumTerrainTileJson" / "NoTiles.tile.json";
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
    CHECK(!loaderResult.pLoader);
    CHECK(!loaderResult.pRootTile);
    CHECK(loaderResult.errors.errors.size() == 1);
    CHECK(
        loaderResult.errors.errors[0] ==
        "Layer Json does not specify any tile URL templates");
  }

  SECTION("Load layer json with metadataAvailability field") {
    auto layerJsonPath = testDataPath / "CesiumTerrainTileJson" /
                         "MetadataAvailability.tile.json";
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
    CHECK(!loaderResult.errors);

    CHECK(std::holds_alternative<GeographicProjection>(
        loaderResult.pLoader->getProjection()));

    const auto& layers = loaderResult.pLoader->getLayers();
    CHECK(layers.size() == 1);
    CHECK(layers[0].version == "1.33.0");
    CHECK(
        layers[0].tileTemplateUrls.front() ==
        "{z}/{x}/{y}.terrain?v={version}&extensions=octvertexnormals-metadata");
    CHECK(layers[0].loadedSubtrees.size() == 2);
    CHECK(layers[0].availabilityLevels == 10);
  }

  SECTION("Load layer json with OctVertexNormals extension") {
    auto layerJsonPath =
        testDataPath / "CesiumTerrainTileJson" / "OctVertexNormals.tile.json";
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
    CHECK(!loaderResult.errors);

    CHECK(std::holds_alternative<GeographicProjection>(
        loaderResult.pLoader->getProjection()));

    const auto& layers = loaderResult.pLoader->getLayers();
    CHECK(layers.size() == 1);
    CHECK(layers[0].version == "1.0.0");
    CHECK(
        layers[0].tileTemplateUrls.front() ==
        "{z}/{x}/{y}.terrain?v={version}&extensions=octvertexnormals");
    CHECK(layers[0].loadedSubtrees.empty());
    CHECK(layers[0].availabilityLevels == -1);

    CHECK(
        layers[0].contentAvailability.isTileAvailable(QuadtreeTileID(0, 0, 0)));
    CHECK(
        layers[0].contentAvailability.isTileAvailable(QuadtreeTileID(0, 1, 0)));
    CHECK(
        layers[0].contentAvailability.isTileAvailable(QuadtreeTileID(1, 1, 0)));
    CHECK(
        layers[0].contentAvailability.isTileAvailable(QuadtreeTileID(1, 3, 1)));
  }

  SECTION("Load multiple layers") {
    auto layerJsonPath =
        testDataPath / "CesiumTerrainTileJson" / "ParentUrl.tile.json";
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"layer.json", createMockAssetRequest(layerJsonPath)});

    auto parentJsonPath =
        testDataPath / "CesiumTerrainTileJson" / "Parent.tile.json";
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"./Parent/layer.json", createMockAssetRequest(parentJsonPath)});

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
    CHECK(!loaderResult.errors);

    const auto& layers = loaderResult.pLoader->getLayers();
    CHECK(layers.size() == 2);

    CHECK(layers[0].baseUrl == "ParentUrl.tile.json");
    CHECK(layers[0].version == "1.0.0");
    CHECK(layers[0].tileTemplateUrls.size() == 1);
    CHECK(
        layers[0].tileTemplateUrls.front() ==
        "{z}/{x}/{y}.terrain?v={version}");

    CHECK(layers[1].baseUrl == "Parent.tile.json");
    CHECK(layers[1].version == "1.1.0");
    CHECK(layers[1].tileTemplateUrls.size() == 1);
    CHECK(
        layers[1].tileTemplateUrls.front() ==
        "{z}/{x}/{y}.terrain?v={version}");
  }

  SECTION("Load layer json with partial availability") {
    auto layerJsonPath = testDataPath / "CesiumTerrainTileJson" /
                         "PartialAvailability.tile.json";
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

    const auto& layers = loaderResult.pLoader->getLayers();
    CHECK(layers.size() == 1);
    CHECK(
        layers[0].contentAvailability.isTileAvailable(QuadtreeTileID(2, 1, 0)));
    CHECK(!layers[0].contentAvailability.isTileAvailable(
        QuadtreeTileID(2, 0, 0)));
  }

  SECTION("Load layer json with attribution") {
    auto layerJsonPath =
        testDataPath / "CesiumTerrainTileJson" / "WithAttribution.tile.json";
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
    CHECK(loaderResult.credits.size() == 1);
    CHECK(
        loaderResult.credits.front().creditText ==
        "This amazing data is courtesy The Amazing Data Source!");
  }

  SECTION("Load layer json with watermask") {
    auto layerJsonPath =
        testDataPath / "CesiumTerrainTileJson" / "WaterMask.tile.json";
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"layer.json", createMockAssetRequest(layerJsonPath)});

    TilesetContentOptions options;
    options.enableWaterMask = true;
    auto loaderFuture = LayerJsonTerrainLoader::createLoader(
        externals,
        options,
        "layer.json",
        {},
        true);

    asyncSystem.dispatchMainThreadTasks();

    auto loaderResult = loaderFuture.wait();
    CHECK(loaderResult.pLoader);
    CHECK(loaderResult.pRootTile);

    const auto& layers = loaderResult.pLoader->getLayers();
    CHECK(layers.size() == 1);
    CHECK(layers[0].tileTemplateUrls.size() == 1);
    CHECK(
        layers[0].tileTemplateUrls[0] ==
        "{z}/{x}/"
        "{y}.terrain?v={version}&extensions=octvertexnormals-watermask");
  }
}

TEST_CASE("Test load layer json tile content") {
  Cesium3DTilesSelection::registerAllTileContentTypes();

  auto pMockedAssetAccessor = std::make_shared<SimpleAssetAccessor>(
      std::map<std::string, std::shared_ptr<SimpleAssetRequest>>{});

  CesiumAsync::AsyncSystem asyncSystem{std::make_shared<SimpleTaskProcessor>()};

  GeographicProjection projection;
  auto quadtreeRectangleProjected = projection.project(GeographicProjection::MAXIMUM_GLOBE_RECTANGLE);

  QuadtreeTilingScheme tilingScheme{quadtreeRectangleProjected, 2, 1};

  const uint32_t maxZoom = 10;

  CesiumGeometry::QuadtreeRectangleAvailability contentAvailability{
      tilingScheme,
      maxZoom};


  SECTION("Load tile when layer have availabilityLevels field") {
    // create loader
    std::vector<LayerJsonTerrainLoader::Layer> layers;
    layers.emplace_back(
        "layer.json",
        "1.0.0",
        std::vector<std::string>{"{level}.{x}.{y}/{version}.terrain"},
        std::move(contentAvailability),
        maxZoom,
        10);

    LayerJsonTerrainLoader loader{tilingScheme, projection, std::move(layers)};

    // mock tile content request
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"0.0.0/1.0.0.terrain",
         createMockAssetRequest(
             testDataPath / "CesiumTerrainTileJson" / "tile.metadataavailability.terrain")});

    // try to request tile
    Tile tile(&loader);
    tile.setTileID(QuadtreeTileID(0, 0, 0));
    tile.setBoundingVolume(BoundingRegionWithLooseFittingHeights{
        {GeographicProjection::MAXIMUM_GLOBE_RECTANGLE, -1000.0, 9000.0}});

    // check tile availability before loading
    const auto& loaderLayers = loader.getLayers();
    const auto& layer = loaderLayers[0];
    CHECK(layer.contentAvailability.isTileAvailable(QuadtreeTileID(0, 0, 0)));
    CHECK(!layer.contentAvailability.isTileAvailable(QuadtreeTileID(1, 0, 1)));
    CHECK(!layer.contentAvailability.isTileAvailable(QuadtreeTileID(8, 177, 177)));

    auto tileLoadResultFuture = loader.loadTileContent(
        tile,
        {},
        asyncSystem,
        pMockedAssetAccessor,
        spdlog::default_logger(),
        {});

    asyncSystem.dispatchMainThreadTasks();

    // check the load result
    auto tileLoadResult = tileLoadResultFuture.wait();
    const auto& renderContent =
        std::get<TileRenderContent>(tileLoadResult.contentKind);
    CHECK(renderContent.model);
    CHECK(tileLoadResult.updatedBoundingVolume);
    CHECK(!tileLoadResult.updatedContentBoundingVolume);
    CHECK(!tileLoadResult.tileInitializer);
    CHECK(tileLoadResult.state == TileLoadResultState::Success);

    // check that the layer receive new rectangle availability
    CHECK(layer.contentAvailability.isTileAvailable(QuadtreeTileID(0, 0, 0)));
    CHECK(layer.contentAvailability.isTileAvailable(QuadtreeTileID(1, 0, 1)));
    CHECK(layer.contentAvailability.isTileAvailable(QuadtreeTileID(8, 177, 177)));
    CHECK(!layer.contentAvailability.isTileAvailable(QuadtreeTileID(9, 0, 0)));
  }

  SECTION("Load tile when layer have no availabilityLevels field") {
    // create loader
    std::vector<LayerJsonTerrainLoader::Layer> layers;
    layers.emplace_back(
        "layer.json",
        "1.0.0",
        std::vector<std::string>{"{level}.{x}.{y}/{version}.terrain"},
        std::move(contentAvailability),
        maxZoom,
        -1);

    LayerJsonTerrainLoader loader{tilingScheme, projection, std::move(layers)};

    // mock tile content request
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"0.0.0/1.0.0.terrain",
         createMockAssetRequest(
             testDataPath / "CesiumTerrainTileJson" / "tile.metadataavailability.terrain")});

    // check tile availability before loading
    const auto& loaderLayers = loader.getLayers();
    const auto& layer = loaderLayers[0];
    CHECK(layer.contentAvailability.isTileAvailable(QuadtreeTileID(0, 0, 0)));
    CHECK(!layer.contentAvailability.isTileAvailable(QuadtreeTileID(1, 0, 1)));
    CHECK(!layer.contentAvailability.isTileAvailable(QuadtreeTileID(8, 177, 177)));

    // try to request tile
    Tile tile(&loader);
    tile.setTileID(QuadtreeTileID(0, 0, 0));
    tile.setBoundingVolume(BoundingRegionWithLooseFittingHeights{
        {GeographicProjection::MAXIMUM_GLOBE_RECTANGLE, -1000.0, 9000.0}});

    auto tileLoadResultFuture = loader.loadTileContent(
        tile,
        {},
        asyncSystem,
        pMockedAssetAccessor,
        spdlog::default_logger(),
        {});

    asyncSystem.dispatchMainThreadTasks();

    // check the load result
    auto tileLoadResult = tileLoadResultFuture.wait();
    const auto& renderContent =
        std::get<TileRenderContent>(tileLoadResult.contentKind);
    CHECK(renderContent.model);
    CHECK(tileLoadResult.updatedBoundingVolume);
    CHECK(!tileLoadResult.updatedContentBoundingVolume);
    CHECK(!tileLoadResult.tileInitializer);
    CHECK(tileLoadResult.state == TileLoadResultState::Success);

    // layer won't add the availability range even the tile content contains them
    CHECK(layer.contentAvailability.isTileAvailable(QuadtreeTileID(0, 0, 0)));
    CHECK(!layer.contentAvailability.isTileAvailable(QuadtreeTileID(1, 0, 1)));
    CHECK(!layer.contentAvailability.isTileAvailable(QuadtreeTileID(8, 177, 177)));
  }

  SECTION("Load tile with multiple layers. Ensure layer is chosen correctly to load tile") {
    // create loader
    std::vector<LayerJsonTerrainLoader::Layer> layers;

    CesiumGeometry::QuadtreeRectangleAvailability layer0ContentAvailability{
        tilingScheme,
        maxZoom};
    layer0ContentAvailability.addAvailableTileRange({0, 0, 0, 1, 0});
    layer0ContentAvailability.addAvailableTileRange({1, 0, 0, 1, 0});
    layer0ContentAvailability.addAvailableTileRange({2, 0, 0, 1, 1});
    layer0ContentAvailability.addAvailableTileRange({2, 2, 0, 2, 0});
    layers.emplace_back(
        "layer.json",
        "1.0.0",
        std::vector<std::string>{"{level}.{x}.{y}/{version}_layer0.terrain"},
        std::move(layer0ContentAvailability),
        maxZoom,
        -1);

    CesiumGeometry::QuadtreeRectangleAvailability layer1ContentAvailability{
        tilingScheme,
        maxZoom};
    layer1ContentAvailability.addAvailableTileRange({0, 0, 0, 1, 0});
    layer1ContentAvailability.addAvailableTileRange({1, 0, 0, 1, 1});
    layer1ContentAvailability.addAvailableTileRange({2, 0, 0, 3, 3});
    layers.emplace_back(
        "layer.json",
        "1.0.0",
        std::vector<std::string>{"{level}.{x}.{y}/{version}_layer1.terrain"},
        std::move(layer1ContentAvailability),
        maxZoom,
        -1);

    LayerJsonTerrainLoader loader{tilingScheme, projection, std::move(layers)};

    // mock tile content request
    auto pMockRequest = createMockAssetRequest(
        testDataPath / "CesiumTerrainTileJson" /
        "tile.terrain"); 

    // load tile from the first layer
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"0.0.0/1.0.0_layer0.terrain", pMockRequest});
    {
      Tile tile(&loader);
      tile.setTileID(QuadtreeTileID(0, 0, 0));
      tile.setBoundingVolume(BoundingRegionWithLooseFittingHeights{
          {GeographicProjection::MAXIMUM_GLOBE_RECTANGLE, -1000.0, 9000.0}});

      auto tileLoadResultFuture = loader.loadTileContent(
          tile,
          {},
          asyncSystem,
          pMockedAssetAccessor,
          spdlog::default_logger(),
          {});

      asyncSystem.dispatchMainThreadTasks();

      auto tileLoadResult = tileLoadResultFuture.wait();
      const auto& renderContent =
          std::get<TileRenderContent>(tileLoadResult.contentKind);
      CHECK(renderContent.model);
      CHECK(tileLoadResult.updatedBoundingVolume);
      CHECK(!tileLoadResult.updatedContentBoundingVolume);
      CHECK(!tileLoadResult.tileInitializer);
      CHECK(tileLoadResult.state == TileLoadResultState::Success);
    }

    // load tile from the second layer
    pMockedAssetAccessor->mockCompletedRequests.clear();
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"2.3.3/1.0.0_layer1.terrain", pMockRequest});
    {
      Tile tile(&loader);
      tile.setTileID(QuadtreeTileID(2, 3, 3));
      tile.setBoundingVolume(BoundingRegionWithLooseFittingHeights{
          {GeographicProjection::MAXIMUM_GLOBE_RECTANGLE, -1000.0, 9000.0}});

      auto tileLoadResultFuture = loader.loadTileContent(
          tile,
          {},
          asyncSystem,
          pMockedAssetAccessor,
          spdlog::default_logger(),
          {});

      asyncSystem.dispatchMainThreadTasks();

      auto tileLoadResult = tileLoadResultFuture.wait();
      const auto& renderContent =
          std::get<TileRenderContent>(tileLoadResult.contentKind);
      CHECK(renderContent.model);
      CHECK(tileLoadResult.updatedBoundingVolume);
      CHECK(!tileLoadResult.updatedContentBoundingVolume);
      CHECK(!tileLoadResult.tileInitializer);
      CHECK(tileLoadResult.state == TileLoadResultState::Success);
    }
  }
}
