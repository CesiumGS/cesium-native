#include "LayerJsonTerrainLoader.h"
#include "MockTilesetContentManager.h"
#include "SimplePrepareRendererResource.h"

#include <Cesium3DTilesContent/registerAllTileContentTypes.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileLoadResult.h>
#include <Cesium3DTilesSelection/TileRefine.h>
#include <Cesium3DTilesSelection/TilesetContentLoader.h>
#include <Cesium3DTilesSelection/TilesetExternals.h>
#include <Cesium3DTilesSelection/TilesetOptions.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumGltf/Model.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumNativeTests/SimpleAssetResponse.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/readFile.h>
#include <CesiumUtility/CreditSystem.h>
#include <CesiumUtility/Math.h>

#include <doctest/doctest.h>
#include <spdlog/sinks/ringbuffer_sink.h>
#include <spdlog/spdlog.h>

#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace doctest;
using namespace Cesium3DTilesSelection;
using namespace CesiumGeospatial;
using namespace CesiumGeometry;
using namespace CesiumAsync;
using namespace CesiumUtility;
using namespace CesiumNativeTests;

namespace {
std::filesystem::path testDataPath = Cesium3DTilesSelection_TEST_DATA_DIR;

std::shared_ptr<SimpleAssetRequest>
createMockAssetRequest(const std::filesystem::path& requestContentPath) {
  std::unique_ptr<SimpleAssetResponse> pMockCompletedResponse;
  if (std::filesystem::exists(requestContentPath)) {
    pMockCompletedResponse = std::make_unique<SimpleAssetResponse>(
        static_cast<uint16_t>(200),
        "doesn't matter",
        CesiumAsync::HttpHeaders{},
        readFile(requestContentPath));
  } else {
    pMockCompletedResponse = std::make_unique<SimpleAssetResponse>(
        static_cast<uint16_t>(404),
        "doesn't matter",
        CesiumAsync::HttpHeaders{},
        std::vector<std::byte>{});
  }
  auto pMockCompletedRequest = std::make_shared<SimpleAssetRequest>(
      "GET",
      requestContentPath.filename().string(),
      CesiumAsync::HttpHeaders{},
      std::move(pMockCompletedResponse));
  return pMockCompletedRequest;
}

Future<TileLoadResult> loadTile(
    const QuadtreeTileID& tileID,
    LayerJsonTerrainLoader& loader,
    AsyncSystem& asyncSystem,
    const std::shared_ptr<IAssetAccessor>& pAssetAccessor) {
  Tile tile(&loader);
  tile.setTileID(tileID);
  tile.setBoundingVolume(BoundingRegionWithLooseFittingHeights{
      {GlobeRectangle(-Math::OnePi, -Math::PiOverTwo, 0.0, Math::PiOverTwo),
       -1000.0,
       9000.0,
       Ellipsoid::WGS84}});

  TileLoadInput loadInput{
      tile,
      {},
      asyncSystem,
      pAssetAccessor,
      spdlog::default_logger(),
      {}};

  auto tileLoadResultFuture = loader.loadTileContent(loadInput);

  asyncSystem.dispatchMainThreadTasks();

  return tileLoadResultFuture;
}
} // namespace

TEST_CASE("Test create layer json terrain loader") {
  Cesium3DTilesContent::registerAllTileContentTypes();

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

  SUBCASE("Create layer json loader") {
    auto layerJsonPath =
        testDataPath / "CesiumTerrainTileJson" / "QuantizedMesh.tile.json";
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"layer.json", createMockAssetRequest(layerJsonPath)});

    auto loaderFuture =
        LayerJsonTerrainLoader::createLoader(externals, {}, "layer.json", {});

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

  SUBCASE("Load error layer json with empty tiles array") {
    auto layerJsonPath =
        testDataPath / "CesiumTerrainTileJson" / "EmptyTilesArray.tile.json";
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"layer.json", createMockAssetRequest(layerJsonPath)});

    auto loaderFuture =
        LayerJsonTerrainLoader::createLoader(externals, {}, "layer.json", {});

    asyncSystem.dispatchMainThreadTasks();

    auto loaderResult = loaderFuture.wait();
    CHECK(!loaderResult.pLoader);
    CHECK(!loaderResult.pRootTile);
    CHECK(loaderResult.errors.errors.size() == 1);
    CHECK(
        loaderResult.errors.errors[0] ==
        "Layer Json does not specify any tile URL templates");
  }

  SUBCASE("Load error layer json with no tiles field") {
    auto layerJsonPath =
        testDataPath / "CesiumTerrainTileJson" / "NoTiles.tile.json";
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"layer.json", createMockAssetRequest(layerJsonPath)});

    auto loaderFuture =
        LayerJsonTerrainLoader::createLoader(externals, {}, "layer.json", {});

    asyncSystem.dispatchMainThreadTasks();

    auto loaderResult = loaderFuture.wait();
    CHECK(!loaderResult.pLoader);
    CHECK(!loaderResult.pRootTile);
    CHECK(loaderResult.errors.errors.size() == 1);
    CHECK(
        loaderResult.errors.errors[0] ==
        "Layer Json does not specify any tile URL templates");
  }

  SUBCASE("Load layer json with metadataAvailability field") {
    auto layerJsonPath = testDataPath / "CesiumTerrainTileJson" /
                         "MetadataAvailability.tile.json";
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"layer.json", createMockAssetRequest(layerJsonPath)});

    auto loaderFuture =
        LayerJsonTerrainLoader::createLoader(externals, {}, "layer.json", {});

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
        "{z}/{x}/{y}.terrain?v={version}");
    CHECK(layers[0].extensionsToRequest == "octvertexnormals-metadata");
    CHECK(layers[0].loadedSubtrees.size() == 2);
    CHECK(layers[0].availabilityLevels == 10);
  }

  SUBCASE("Load layer json with OctVertexNormals extension") {
    auto layerJsonPath =
        testDataPath / "CesiumTerrainTileJson" / "OctVertexNormals.tile.json";
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"layer.json", createMockAssetRequest(layerJsonPath)});

    auto loaderFuture =
        LayerJsonTerrainLoader::createLoader(externals, {}, "layer.json", {});

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
        "{z}/{x}/{y}.terrain?v={version}");
    CHECK(layers[0].extensionsToRequest == "octvertexnormals");
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

  SUBCASE("Load multiple layers") {
    auto layerJsonPath =
        testDataPath / "CesiumTerrainTileJson" / "ParentUrl.tile.json";
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"layer.json", createMockAssetRequest(layerJsonPath)});

    auto parentJsonPath =
        testDataPath / "CesiumTerrainTileJson" / "Parent.tile.json";
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"Parent/layer.json", createMockAssetRequest(parentJsonPath)});

    auto loaderFuture =
        LayerJsonTerrainLoader::createLoader(externals, {}, "layer.json", {});

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

  SUBCASE("Load layer json with partial availability") {
    auto layerJsonPath = testDataPath / "CesiumTerrainTileJson" /
                         "PartialAvailability.tile.json";
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"layer.json", createMockAssetRequest(layerJsonPath)});

    auto loaderFuture =
        LayerJsonTerrainLoader::createLoader(externals, {}, "layer.json", {});

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

  SUBCASE("Load layer json with attribution") {
    auto layerJsonPath =
        testDataPath / "CesiumTerrainTileJson" / "WithAttribution.tile.json";
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"layer.json", createMockAssetRequest(layerJsonPath)});

    auto loaderFuture =
        LayerJsonTerrainLoader::createLoader(externals, {}, "layer.json", {});

    asyncSystem.dispatchMainThreadTasks();

    auto loaderResult = loaderFuture.wait();
    CHECK(loaderResult.pLoader);
    CHECK(loaderResult.pRootTile);
    CHECK(loaderResult.credits.size() == 1);
    CHECK(
        loaderResult.credits.front().creditText ==
        "This amazing data is courtesy The Amazing Data Source!");
  }

  SUBCASE("Load layer json with watermask") {
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
        {});

    asyncSystem.dispatchMainThreadTasks();

    auto loaderResult = loaderFuture.wait();
    CHECK(loaderResult.pLoader);
    CHECK(loaderResult.pRootTile);

    const auto& layers = loaderResult.pLoader->getLayers();
    CHECK(layers.size() == 1);
    CHECK(layers[0].tileTemplateUrls.size() == 1);
    CHECK(layers[0].tileTemplateUrls[0] == "{z}/{x}/{y}.terrain?v={version}");
    CHECK(layers[0].extensionsToRequest == "octvertexnormals-watermask");
  }
}

TEST_CASE("Test load layer json tile content") {
  Cesium3DTilesContent::registerAllTileContentTypes();

  auto pMockedAssetAccessor = std::make_shared<SimpleAssetAccessor>(
      std::map<std::string, std::shared_ptr<SimpleAssetRequest>>{});

  CesiumAsync::AsyncSystem asyncSystem{std::make_shared<SimpleTaskProcessor>()};

  GeographicProjection projection(Ellipsoid::WGS84);
  auto quadtreeRectangleProjected =
      projection.project(GeographicProjection::MAXIMUM_GLOBE_RECTANGLE);

  QuadtreeTilingScheme tilingScheme{quadtreeRectangleProjected, 2, 1};

  const uint32_t maxZoom = 10;

  CesiumGeometry::QuadtreeRectangleAvailability contentAvailability{
      tilingScheme,
      maxZoom};

  SUBCASE("Load tile when layer have availabilityLevels field") {
    // create loader
    std::vector<LayerJsonTerrainLoader::Layer> layers;
    layers.emplace_back(
        "layer.json",
        "1.0.0",
        std::vector<std::string>{"{level}.{x}.{y}/{version}.terrain"},
        "one-two",
        std::move(contentAvailability),
        maxZoom,
        10);

    LayerJsonTerrainLoader loader{tilingScheme, projection, std::move(layers)};

    // mock tile content request
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"0.0.0/1.0.0.terrain?extensions=one-two",
         createMockAssetRequest(
             testDataPath / "CesiumTerrainTileJson" /
             "tile.metadataavailability.terrain")});

    // check tile availability before loading
    const auto& loaderLayers = loader.getLayers();
    const auto& layer = loaderLayers[0];
    CHECK(layer.contentAvailability.isTileAvailable(QuadtreeTileID(0, 0, 0)));
    CHECK(!layer.contentAvailability.isTileAvailable(QuadtreeTileID(1, 0, 1)));
    CHECK(!layer.contentAvailability.isTileAvailable(
        QuadtreeTileID(8, 177, 177)));

    // check the load result
    auto tileLoadResultFuture = loadTile(
        QuadtreeTileID(0, 0, 0),
        loader,
        asyncSystem,
        pMockedAssetAccessor);
    auto tileLoadResult = tileLoadResultFuture.wait();
    CHECK(
        std::holds_alternative<CesiumGltf::Model>(tileLoadResult.contentKind));
    CHECK(tileLoadResult.updatedBoundingVolume);
    CHECK(!tileLoadResult.updatedContentBoundingVolume);
    CHECK(!tileLoadResult.tileInitializer);
    CHECK(tileLoadResult.state == TileLoadResultState::Success);

    // check that the layer receive new rectangle availability
    CHECK(layer.contentAvailability.isTileAvailable(QuadtreeTileID(0, 0, 0)));
    CHECK(layer.contentAvailability.isTileAvailable(QuadtreeTileID(1, 0, 1)));
    CHECK(
        layer.contentAvailability.isTileAvailable(QuadtreeTileID(8, 177, 177)));
    CHECK(!layer.contentAvailability.isTileAvailable(QuadtreeTileID(9, 0, 0)));
  }

  SUBCASE("Load tile with query parameters from base URL") {
    // create loader
    std::vector<LayerJsonTerrainLoader::Layer> layers;
    layers.emplace_back(
        "layer.json?param=some_parameter_here",
        "1.0.0",
        std::vector<std::string>{"{level}.{x}.{y}/{version}.terrain"},
        "one-two",
        std::move(contentAvailability),
        maxZoom,
        10);

    LayerJsonTerrainLoader loader{tilingScheme, projection, std::move(layers)};

    // mock tile content request
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"0.0.0/1.0.0.terrain?param=some_parameter_here&extensions=one-two",
         createMockAssetRequest(
             testDataPath / "CesiumTerrainTileJson" /
             "tile.metadataavailability.terrain")});

    // check the load result
    auto tileLoadResultFuture = loadTile(
        QuadtreeTileID(0, 0, 0),
        loader,
        asyncSystem,
        pMockedAssetAccessor);
    auto tileLoadResult = tileLoadResultFuture.wait();
    CHECK(
        std::holds_alternative<CesiumGltf::Model>(tileLoadResult.contentKind));
    CHECK(tileLoadResult.updatedBoundingVolume);
    CHECK(!tileLoadResult.updatedContentBoundingVolume);
    CHECK(!tileLoadResult.tileInitializer);
    CHECK(tileLoadResult.state == TileLoadResultState::Success);
  }

  SUBCASE("Load tile when layer have no availabilityLevels field") {
    // create loader
    std::vector<LayerJsonTerrainLoader::Layer> layers;
    layers.emplace_back(
        "layer.json",
        "1.0.0",
        std::vector<std::string>{"{level}.{x}.{y}/{version}.terrain"},
        std::string(),
        std::move(contentAvailability),
        maxZoom,
        -1);

    LayerJsonTerrainLoader loader{tilingScheme, projection, std::move(layers)};

    // mock tile content request
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"0.0.0/1.0.0.terrain",
         createMockAssetRequest(
             testDataPath / "CesiumTerrainTileJson" /
             "tile.metadataavailability.terrain")});

    // check tile availability before loading
    const auto& loaderLayers = loader.getLayers();
    const auto& layer = loaderLayers[0];
    CHECK(layer.contentAvailability.isTileAvailable(QuadtreeTileID(0, 0, 0)));
    CHECK(!layer.contentAvailability.isTileAvailable(QuadtreeTileID(1, 0, 1)));
    CHECK(!layer.contentAvailability.isTileAvailable(
        QuadtreeTileID(8, 177, 177)));

    // try to request tile
    auto tileLoadResultFuture = loadTile(
        QuadtreeTileID(0, 0, 0),
        loader,
        asyncSystem,
        pMockedAssetAccessor);

    // check the load result
    auto tileLoadResult = tileLoadResultFuture.wait();
    CHECK(
        std::holds_alternative<CesiumGltf::Model>(tileLoadResult.contentKind));
    CHECK(tileLoadResult.updatedBoundingVolume);
    CHECK(!tileLoadResult.updatedContentBoundingVolume);
    CHECK(!tileLoadResult.tileInitializer);
    CHECK(tileLoadResult.state == TileLoadResultState::Success);

    // layer won't add the availability range even the tile content contains
    // them
    CHECK(layer.contentAvailability.isTileAvailable(QuadtreeTileID(0, 0, 0)));
    CHECK(!layer.contentAvailability.isTileAvailable(QuadtreeTileID(1, 0, 1)));
    CHECK(!layer.contentAvailability.isTileAvailable(
        QuadtreeTileID(8, 177, 177)));
  }

  SUBCASE("Load tile with multiple layers. Ensure layer is chosen correctly to "
          "load tile") {
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
        std::string(),
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
        std::string(),
        std::move(layer1ContentAvailability),
        maxZoom,
        -1);

    LayerJsonTerrainLoader loader{tilingScheme, projection, std::move(layers)};

    // mock tile content request
    auto pMockRequest = createMockAssetRequest(
        testDataPath / "CesiumTerrainTileJson" / "tile.terrain");

    // load tile from the first layer
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"0.0.0/1.0.0_layer0.terrain", pMockRequest});
    {
      auto tileLoadResultFuture = loadTile(
          QuadtreeTileID(0, 0, 0),
          loader,
          asyncSystem,
          pMockedAssetAccessor);

      auto tileLoadResult = tileLoadResultFuture.wait();
      CHECK(std::holds_alternative<CesiumGltf::Model>(
          tileLoadResult.contentKind));
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
      auto tileLoadResultFuture = loadTile(
          QuadtreeTileID(2, 3, 3),
          loader,
          asyncSystem,
          pMockedAssetAccessor);

      auto tileLoadResult = tileLoadResultFuture.wait();
      CHECK(std::holds_alternative<CesiumGltf::Model>(
          tileLoadResult.contentKind));
      CHECK(tileLoadResult.updatedBoundingVolume);
      CHECK(!tileLoadResult.updatedContentBoundingVolume);
      CHECK(!tileLoadResult.tileInitializer);
      CHECK(tileLoadResult.state == TileLoadResultState::Success);
    }
  }

  SUBCASE("Ensure layers metadata does not load twice when tile at "
          "availability level is loaded the 2nd time") {
    // create loader
    std::vector<LayerJsonTerrainLoader::Layer> layers;

    CesiumGeometry::QuadtreeRectangleAvailability layer0ContentAvailability{
        tilingScheme,
        maxZoom};
    layers.emplace_back(
        "layer.json",
        "1.0.0",
        std::vector<std::string>{"{level}.{x}.{y}/{version}_layer0.terrain"},
        std::string(),
        std::move(layer0ContentAvailability),
        maxZoom,
        10);

    CesiumGeometry::QuadtreeRectangleAvailability layer1ContentAvailability{
        tilingScheme,
        maxZoom};
    layers.emplace_back(
        "layer.json",
        "1.0.0",
        std::vector<std::string>{"{level}.{x}.{y}/{version}_layer1.terrain"},
        std::string(),
        std::move(layer1ContentAvailability),
        maxZoom,
        10);

    LayerJsonTerrainLoader loader{tilingScheme, projection, std::move(layers)};

    // mock tile content request
    auto pMockRequest = createMockAssetRequest(
        testDataPath / "CesiumTerrainTileJson" /
        "tile.metadataavailability.terrain");

    // load tile from the first layer. But the tile from the second layer
    // will be loaded as well to add the availability to the second layer
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"0.0.0/1.0.0_layer0.terrain", pMockRequest});
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"0.0.0/1.0.0_layer1.terrain", pMockRequest});
    {
      auto tileLoadResultFuture = loadTile(
          QuadtreeTileID(0, 0, 0),
          loader,
          asyncSystem,
          pMockedAssetAccessor);

      auto tileLoadResult = tileLoadResultFuture.wait();
      CHECK(std::holds_alternative<CesiumGltf::Model>(
          tileLoadResult.contentKind));
      CHECK(tileLoadResult.updatedBoundingVolume);
      CHECK(!tileLoadResult.updatedContentBoundingVolume);
      CHECK(!tileLoadResult.tileInitializer);
      CHECK(tileLoadResult.state == TileLoadResultState::Success);

      // make sure that layers has all the availabilities it needs
      const auto& loaderLayers = loader.getLayers();
      CHECK(
          loaderLayers[0].loadedSubtrees[0].find(0) !=
          loaderLayers[0].loadedSubtrees[0].end());

      CHECK(
          loaderLayers[1].loadedSubtrees[0].find(0) !=
          loaderLayers[1].loadedSubtrees[0].end());
    }

    // remove the second layer request to make sure that
    // the second layer availability is not requested again
    pMockedAssetAccessor->mockCompletedRequests.erase(
        "0.0.0/1.0.0_layer1.terrain");
    {
      auto tileLoadResultFuture = loadTile(
          QuadtreeTileID(0, 0, 0),
          loader,
          asyncSystem,
          pMockedAssetAccessor);

      auto tileLoadResult = tileLoadResultFuture.wait();
      CHECK(std::holds_alternative<CesiumGltf::Model>(
          tileLoadResult.contentKind));
      CHECK(tileLoadResult.updatedBoundingVolume);
      CHECK(!tileLoadResult.updatedContentBoundingVolume);
      CHECK(!tileLoadResult.tileInitializer);
      CHECK(tileLoadResult.state == TileLoadResultState::Success);
    }
  }

  SUBCASE("Should error when fetching non-existent .terrain tiles") {
    auto pLog = std::make_shared<spdlog::sinks::ringbuffer_sink_mt>(3);
    spdlog::default_logger()->sinks().emplace_back(pLog);

    // create loader
    std::vector<LayerJsonTerrainLoader::Layer> layers;
    layers.emplace_back(
        "layer.json",
        "1.0.0",
        std::vector<std::string>{"{level}.{x}.{y}/{version}.terrain"},
        std::string{},
        std::move(contentAvailability),
        maxZoom,
        10);

    LayerJsonTerrainLoader loader{tilingScheme, projection, std::move(layers)};

    // mock tile content request
    pMockedAssetAccessor->mockCompletedRequests.insert(
        {"0.0.0/1.0.0.terrain",
         createMockAssetRequest(
             testDataPath / "CesiumTerrainTileJson" / "nonexistent.terrain")});

    // check the load result
    auto tileLoadResultFuture = loadTile(
        QuadtreeTileID(0, 0, 0),
        loader,
        asyncSystem,
        pMockedAssetAccessor);
    auto tileLoadResult = tileLoadResultFuture.wait();
    CHECK(tileLoadResult.state == TileLoadResultState::Failed);

    std::vector<std::string> logMessages = pLog->last_formatted();
    REQUIRE(logMessages.size() == 1);
    REQUIRE(logMessages.back()
                .substr(0, logMessages.back().find_last_not_of("\n\r") + 1)
                .ends_with("Received status code 404 for tile content "
                           "nonexistent.terrain"));
  }
}

TEST_CASE("Test creating tile children for layer json") {
  Cesium3DTilesContent::registerAllTileContentTypes();

  auto pMockedAssetAccessor = std::make_shared<SimpleAssetAccessor>(
      std::map<std::string, std::shared_ptr<SimpleAssetRequest>>{});

  CesiumAsync::AsyncSystem asyncSystem{std::make_shared<SimpleTaskProcessor>()};

  GeographicProjection projection(Ellipsoid::WGS84);
  auto quadtreeRectangleProjected =
      projection.project(GeographicProjection::MAXIMUM_GLOBE_RECTANGLE);

  QuadtreeTilingScheme tilingScheme{quadtreeRectangleProjected, 2, 1};

  const uint32_t maxZoom = 10;

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
      std::string(),
      std::move(layer0ContentAvailability),
      maxZoom,
      10);
  layers.back().loadedSubtrees[0].insert(0);

  CesiumGeometry::QuadtreeRectangleAvailability layer1ContentAvailability{
      tilingScheme,
      maxZoom};
  layer1ContentAvailability.addAvailableTileRange({0, 0, 0, 1, 0});
  layer1ContentAvailability.addAvailableTileRange({1, 0, 0, 1, 1});
  layer1ContentAvailability.addAvailableTileRange({2, 0, 0, 1, 3});
  layers.emplace_back(
      "layer.json",
      "1.0.0",
      std::vector<std::string>{"{level}.{x}.{y}/{version}_layer1.terrain"},
      std::string(),
      std::move(layer1ContentAvailability),
      maxZoom,
      10);
  layers.back().loadedSubtrees[0].insert(0);

  LayerJsonTerrainLoader loader{tilingScheme, projection, std::move(layers)};

  SUBCASE("Create children for tile that is at the root of subtree") {
    Tile tile(&loader);
    tile.setTileID(QuadtreeTileID(0, 0, 0));
    tile.setBoundingVolume(BoundingRegion(
        GlobeRectangle(-Math::OnePi, -Math::PiOverTwo, 0.0, Math::PiOverTwo),
        -1000.0,
        9000.0,
        Ellipsoid::WGS84));

    {
      MockTilesetContentManagerTestFixture::setTileLoadState(
          tile,
          TileLoadState::FailedTemporarily);
      auto tileChildrenResult = loader.createTileChildren(tile);
      CHECK(tileChildrenResult.state == TileLoadResultState::RetryLater);
    }

    {
      MockTilesetContentManagerTestFixture::setTileLoadState(
          tile,
          TileLoadState::Unloaded);
      auto tileChildrenResult = loader.createTileChildren(tile);
      CHECK(tileChildrenResult.state == TileLoadResultState::RetryLater);
    }

    {
      MockTilesetContentManagerTestFixture::setTileLoadState(
          tile,
          TileLoadState::ContentLoading);
      auto tileChildrenResult = loader.createTileChildren(tile);
      CHECK(tileChildrenResult.state == TileLoadResultState::RetryLater);
    }

    {
      MockTilesetContentManagerTestFixture::setTileLoadState(
          tile,
          TileLoadState::ContentLoaded);
      auto tileChildrenResult = loader.createTileChildren(tile);
      CHECK(tileChildrenResult.state == TileLoadResultState::Success);

      const auto& tileChildren = tileChildrenResult.children;
      CHECK(tileChildren.size() == 4);

      const auto& tile_1_0_0 = tileChildren[0];
      const auto& looseRegion_1_0_0 =
          std::get<BoundingRegionWithLooseFittingHeights>(
              tile_1_0_0.getBoundingVolume());
      const auto& region_1_0_0 = looseRegion_1_0_0.getBoundingRegion();
      CHECK(
          std::get<QuadtreeTileID>(tile_1_0_0.getTileID()) ==
          QuadtreeTileID(1, 0, 0));
      CHECK(region_1_0_0.getRectangle().getWest() == Approx(-Math::OnePi));
      CHECK(region_1_0_0.getRectangle().getSouth() == Approx(-Math::PiOverTwo));
      CHECK(region_1_0_0.getRectangle().getEast() == Approx(-Math::PiOverTwo));
      CHECK(region_1_0_0.getRectangle().getNorth() == 0.0);

      const auto& tile_1_1_0 = tileChildren[1];
      const auto& looseRegion_1_1_0 =
          std::get<BoundingRegionWithLooseFittingHeights>(
              tile_1_1_0.getBoundingVolume());
      const auto& region_1_1_0 = looseRegion_1_1_0.getBoundingRegion();
      CHECK(
          std::get<QuadtreeTileID>(tile_1_1_0.getTileID()) ==
          QuadtreeTileID(1, 1, 0));
      CHECK(region_1_1_0.getRectangle().getWest() == Approx(-Math::PiOverTwo));
      CHECK(region_1_1_0.getRectangle().getSouth() == Approx(-Math::PiOverTwo));
      CHECK(region_1_1_0.getRectangle().getEast() == 0.0);
      CHECK(region_1_1_0.getRectangle().getNorth() == 0.0);

      const auto& tile_1_0_1 = tileChildren[2];
      const auto& looseRegion_1_0_1 =
          std::get<BoundingRegionWithLooseFittingHeights>(
              tile_1_0_1.getBoundingVolume());
      const auto& region_1_0_1 = looseRegion_1_0_1.getBoundingRegion();
      CHECK(
          std::get<QuadtreeTileID>(tile_1_0_1.getTileID()) ==
          QuadtreeTileID(1, 0, 1));
      CHECK(region_1_0_1.getRectangle().getWest() == Approx(-Math::OnePi));
      CHECK(region_1_0_1.getRectangle().getSouth() == 0.0);
      CHECK(region_1_0_1.getRectangle().getEast() == Approx(-Math::PiOverTwo));
      CHECK(region_1_0_1.getRectangle().getNorth() == Approx(Math::PiOverTwo));

      const auto& tile_1_1_1 = tileChildren[3];
      const auto& looseRegion_1_1_1 =
          std::get<BoundingRegionWithLooseFittingHeights>(
              tile_1_1_1.getBoundingVolume());
      const auto& region_1_1_1 = looseRegion_1_1_1.getBoundingRegion();
      CHECK(
          std::get<QuadtreeTileID>(tile_1_1_1.getTileID()) ==
          QuadtreeTileID(1, 1, 1));
      CHECK(region_1_1_1.getRectangle().getWest() == Approx(-Math::PiOverTwo));
      CHECK(region_1_1_1.getRectangle().getSouth() == 0.0);
      CHECK(region_1_1_1.getRectangle().getEast() == 0.0);
      CHECK(region_1_1_1.getRectangle().getNorth() == Approx(Math::PiOverTwo));
    }
  }

  SUBCASE("Create children for tile that is in the middle of subtree") {
    Tile tile(&loader);
    tile.setTileID(QuadtreeTileID(1, 0, 1));
    tile.setBoundingVolume(BoundingRegion(
        GlobeRectangle(-Math::OnePi, 0, -Math::PiOverTwo, Math::PiOverTwo),
        -1000.0,
        9000.0,
        Ellipsoid::WGS84));
    auto tileChildrenResult = loader.createTileChildren(tile);
    CHECK(tileChildrenResult.state == TileLoadResultState::Success);

    const auto& tileChildren = tileChildrenResult.children;
    CHECK(tileChildren.size() == 4);

    const auto& tile_2_0_2 = tileChildren[0];
    const auto& looseRegion_2_0_2 =
        std::get<BoundingRegionWithLooseFittingHeights>(
            tile_2_0_2.getBoundingVolume());
    const auto& region_2_0_2 = looseRegion_2_0_2.getBoundingRegion();
    CHECK(
        std::get<QuadtreeTileID>(tile_2_0_2.getTileID()) ==
        QuadtreeTileID(2, 0, 2));
    CHECK(region_2_0_2.getRectangle().getWest() == Approx(-Math::OnePi));
    CHECK(region_2_0_2.getRectangle().getSouth() == 0.0);
    CHECK(
        region_2_0_2.getRectangle().getEast() ==
        Approx(-Math::OnePi * 3.0 / 4));
    CHECK(region_2_0_2.getRectangle().getNorth() == Approx(Math::OnePi / 4.0));

    const auto& tile_2_1_2 = tileChildren[1];
    const auto& looseRegion_2_1_2 =
        std::get<BoundingRegionWithLooseFittingHeights>(
            tile_2_1_2.getBoundingVolume());
    const auto& region_2_1_2 = looseRegion_2_1_2.getBoundingRegion();
    CHECK(
        std::get<QuadtreeTileID>(tile_2_1_2.getTileID()) ==
        QuadtreeTileID(2, 1, 2));
    CHECK(
        region_2_1_2.getRectangle().getWest() ==
        Approx(-Math::OnePi * 3.0 / 4));
    CHECK(region_2_1_2.getRectangle().getSouth() == 0.0);
    CHECK(region_2_1_2.getRectangle().getEast() == Approx(-Math::PiOverTwo));
    CHECK(region_2_1_2.getRectangle().getNorth() == Approx(Math::OnePi / 4.0));

    const auto& tile_2_0_3 = tileChildren[2];
    const auto& looseRegion_2_0_3 =
        std::get<BoundingRegionWithLooseFittingHeights>(
            tile_2_0_3.getBoundingVolume());
    const auto& region_2_0_3 = looseRegion_2_0_3.getBoundingRegion();
    CHECK(
        std::get<QuadtreeTileID>(tile_2_0_3.getTileID()) ==
        QuadtreeTileID(2, 0, 3));
    CHECK(region_2_0_3.getRectangle().getWest() == Approx(-Math::OnePi));
    CHECK(region_2_0_3.getRectangle().getSouth() == Approx(Math::OnePi / 4.0));
    CHECK(
        region_2_0_3.getRectangle().getEast() ==
        Approx(-Math::OnePi * 3.0 / 4));
    CHECK(region_2_0_3.getRectangle().getNorth() == Approx(Math::PiOverTwo));

    const auto& tile_2_1_3 = tileChildren[3];
    const auto& looseRegion_2_1_3 =
        std::get<BoundingRegionWithLooseFittingHeights>(
            tile_2_1_3.getBoundingVolume());
    const auto& region_2_1_3 = looseRegion_2_1_3.getBoundingRegion();
    CHECK(
        std::get<QuadtreeTileID>(tile_2_1_3.getTileID()) ==
        QuadtreeTileID(2, 1, 3));
    CHECK(
        region_2_1_3.getRectangle().getWest() ==
        Approx(-Math::OnePi * 3.0 / 4));
    CHECK(region_2_1_3.getRectangle().getSouth() == Approx(Math::OnePi / 4.0));
    CHECK(region_2_1_3.getRectangle().getEast() == Approx(-Math::PiOverTwo));
    CHECK(region_2_1_3.getRectangle().getNorth() == Approx(Math::PiOverTwo));
  }

  SUBCASE("Create upsample children for tile") {
    Tile tile(&loader);
    tile.setTileID(QuadtreeTileID(1, 1, 0));
    tile.setBoundingVolume(BoundingRegion(
        GlobeRectangle(-Math::PiOverTwo, -Math::PiOverTwo, 0, 0),
        -1000.0,
        9000.0,
        Ellipsoid::WGS84));
    auto tileChildrenResult = loader.createTileChildren(tile);
    CHECK(tileChildrenResult.state == TileLoadResultState::Success);

    const auto& tileChildren = tileChildrenResult.children;
    CHECK(tileChildren.size() == 4);

    const auto& tile_2_2_0 = tileChildren[0];
    CHECK(
        std::get<QuadtreeTileID>(tile_2_2_0.getTileID()) ==
        QuadtreeTileID(2, 2, 0));

    const auto& tile_2_3_0 = tileChildren[1];
    const auto& upsampleID_2_3_0 =
        std::get<UpsampledQuadtreeNode>(tile_2_3_0.getTileID());
    CHECK(upsampleID_2_3_0.tileID == QuadtreeTileID(2, 3, 0));

    const auto& tile_2_2_1 = tileChildren[2];
    const auto& upsampleID_2_2_1 =
        std::get<UpsampledQuadtreeNode>(tile_2_2_1.getTileID());
    CHECK(upsampleID_2_2_1.tileID == QuadtreeTileID(2, 2, 1));

    const auto& tile_2_3_1 = tileChildren[3];
    const auto& upsampleID_2_3_1 =
        std::get<UpsampledQuadtreeNode>(tile_2_3_1.getTileID());
    CHECK(upsampleID_2_3_1.tileID == QuadtreeTileID(2, 3, 1));
  }
}
