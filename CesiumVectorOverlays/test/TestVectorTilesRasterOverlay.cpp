#include <Cesium3DTilesContent/ImplicitTilingUtilities.h>
#include <Cesium3DTilesContent/registerAllTileContentTypes.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/BoundingRegion.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumImage/ImageAsset.h>
#include <CesiumNativeTests/FileAccessor.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/checkFilesEqual.h>
#include <CesiumNativeTests/writeTga.h>
#include <CesiumRasterOverlays/ActivatedRasterOverlay.h>
#include <CesiumRasterOverlays/CreateRasterOverlayTileProviderParameters.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayExternals.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumVectorData/VectorStyle.h>
#include <CesiumVectorOverlays/VectorTilesRasterOverlay.h>

#include <doctest/doctest.h>

#include <random>

const size_t BENCHMARK_ITERATIONS = 100000;

using namespace CesiumAsync;
using namespace CesiumRasterOverlays;
using namespace CesiumUtility;
using namespace CesiumVectorOverlays;

TEST_CASE("Test VectorTilesRasterOverlay polylines") {
  Cesium3DTilesContent::registerAllTileContentTypes();

  const std::filesystem::path dataPath =
      std::filesystem::path(CesiumVectorOverlays_TEST_DATA_DIR);
  const std::filesystem::path inputPath =
      dataPath / "ViennaStreets" / "tileset.json";
  const std::filesystem::path referencePath =
      dataPath / "ViennaStreets" / "rasterized.tga";
  const std::filesystem::path tempOutPath =
      std::filesystem::path(CESIUM_NATIVE_TEMP_DIR) /
      "vector-tile-polylines.tga";

  const glm::dvec2 imageSize(256, 256);

  // A rough rectangle around the Albertgarten in Vienna, as an arbitrary
  // testing area.
  const CesiumGeospatial::GlobeRectangle& tileGlobeRectangle =
      CesiumGeospatial::GlobeRectangle{
          0.2852306824588304,
          0.8414742026919637,
          0.28529452339585826,
          0.8415167430037157};

  std::shared_ptr<CesiumNativeTests::FileAccessor> pAssetAccessor =
      std::make_shared<CesiumNativeTests::FileAccessor>(
          CesiumNativeTests::FileAccessor{});
  AsyncSystem asyncSystem{
      std::make_shared<CesiumNativeTests::SimpleTaskProcessor>()};

  CreateRasterOverlayTileProviderParameters parameters{
      {pAssetAccessor, nullptr, asyncSystem}};
  IntrusivePointer<VectorTilesRasterOverlay> pOverlay;
  pOverlay.emplace(
      "overlay0",
      "file:///" + inputPath.string(),
      CesiumVectorOverlays::VectorTilesRasterOverlayOptions{
          CesiumVectorData::VectorStyle{CesiumUtility::Color(255, 0, 0, 255)},
          {},
          nullptr},
      RasterOverlayOptions{});

  IntrusivePointer<CesiumRasterOverlays::ActivatedRasterOverlay> pActivated =
      pOverlay->activate(
          CesiumRasterOverlays::RasterOverlayExternals{
              .pAssetAccessor = pAssetAccessor,
              .pPrepareRendererResources = nullptr,
              .asyncSystem = asyncSystem,
              .pCreditSystem = nullptr,
              .pLogger = spdlog::default_logger()},
          CesiumGeospatial::Ellipsoid::WGS84);

  pActivated->getReadyEvent().waitInMainThread();

  REQUIRE(pActivated->getTileProvider() != nullptr);
  CesiumGeospatial::GeographicProjection projection(
      CesiumGeospatial::Ellipsoid::WGS84);
  const CesiumGeometry::Rectangle tileRectangle =
      projection.project(tileGlobeRectangle);

  IntrusivePointer<CesiumRasterOverlays::RasterOverlayTile> pTile =
      pActivated->getTile(tileRectangle, imageSize);
  pActivated->loadTile(*pTile);
  while (pTile->getState() !=
         CesiumRasterOverlays::RasterOverlayTile::LoadState::Loaded) {
    asyncSystem.dispatchMainThreadTasks();
    pActivated->tick();
  }

  REQUIRE(pTile->getImage()->width > 1);
  CesiumNativeTests::writeImageToTgaFile(*pTile->getImage(), tempOutPath);

  CesiumNativeTests::checkFilesEqual(tempOutPath, referencePath);
}

TEST_CASE("Test VectorTilesRasterOverlay polygons") {
  Cesium3DTilesContent::registerAllTileContentTypes();

  const std::filesystem::path dataPath =
      std::filesystem::path(CesiumVectorOverlays_TEST_DATA_DIR);
  const std::filesystem::path inputPath =
      dataPath / "switzerland" / "tileset.json";
  const std::filesystem::path referencePath =
      dataPath / "switzerland" / "rasterized.tga";
  const std::filesystem::path tempOutPath =
      std::filesystem::path(CESIUM_NATIVE_TEMP_DIR) /
      "vector-tile-polygons.tga";

  const glm::dvec2 imageSize(256, 256);

  // A rough rectangle around the Albertgarten in Vienna, as an arbitrary
  // testing area.
  const CesiumGeospatial::BoundingRegion& tileRegion =
      Cesium3DTilesContent::ImplicitTilingUtilities::computeBoundingVolume(
          CesiumGeospatial::BoundingRegion{
              CesiumGeospatial::GlobeRectangle{
                  0.10511435661024317,
                  0.7989584641142331,
                  0.18225951525130435,
                  0.8348054325550944},
              0,
              0.005,
              CesiumGeospatial::Ellipsoid::WGS84},
          CesiumGeometry::QuadtreeTileID(1, 0, 0),
          CesiumGeospatial::Ellipsoid::WGS84);
  const CesiumGeospatial::GlobeRectangle& tileGlobeRectangle =
      tileRegion.getRectangle();

  std::shared_ptr<CesiumNativeTests::FileAccessor> pAssetAccessor =
      std::make_shared<CesiumNativeTests::FileAccessor>(
          CesiumNativeTests::FileAccessor{});
  AsyncSystem asyncSystem{
      std::make_shared<CesiumNativeTests::SimpleTaskProcessor>()};

  CreateRasterOverlayTileProviderParameters parameters{
      {pAssetAccessor, nullptr, asyncSystem}};
  RasterOverlayOptions options;

  IntrusivePointer<VectorTilesRasterOverlay> pOverlay;
  pOverlay.emplace(
      "overlay0",
      "file:///" + inputPath.string(),
      CesiumVectorOverlays::VectorTilesRasterOverlayOptions{
          CesiumVectorData::VectorStyle{CesiumUtility::Color(255, 0, 0, 255)},
          {},
          nullptr},
      options);

  IntrusivePointer<CesiumRasterOverlays::ActivatedRasterOverlay> pActivated =
      pOverlay->activate(
          CesiumRasterOverlays::RasterOverlayExternals{
              .pAssetAccessor = pAssetAccessor,
              .pPrepareRendererResources = nullptr,
              .asyncSystem = asyncSystem,
              .pCreditSystem = nullptr,
              .pLogger = spdlog::default_logger()},
          CesiumGeospatial::Ellipsoid::WGS84);

  pActivated->getReadyEvent().waitInMainThread();

  REQUIRE(pActivated->getTileProvider() != nullptr);
  CesiumGeospatial::GeographicProjection projection(
      CesiumGeospatial::Ellipsoid::WGS84);
  const CesiumGeometry::Rectangle tileRectangle =
      projection.project(tileGlobeRectangle);

  IntrusivePointer<CesiumRasterOverlays::RasterOverlayTile> pTile =
      pActivated->getTile(tileRectangle, imageSize);
  pActivated->loadTile(*pTile);
  while (pTile->getState() !=
         CesiumRasterOverlays::RasterOverlayTile::LoadState::Loaded) {
    asyncSystem.dispatchMainThreadTasks();
    pActivated->tick();
  }

  REQUIRE(pTile->getImage()->width > 1);
  CesiumNativeTests::writeImageToTgaFile(*pTile->getImage(), tempOutPath);

  CesiumNativeTests::checkFilesEqual(tempOutPath, referencePath);
}


TEST_CASE(
    "VectorTilesRasterOverlay vienna-streets benchmark" * doctest::skip()) {
  Cesium3DTilesContent::registerAllTileContentTypes();

  const std::filesystem::path dataPath =
      std::filesystem::path(CesiumVectorOverlays_TEST_DATA_DIR);
  const std::filesystem::path inputPath =
      dataPath / "ViennaStreets" / "tileset.json";

  const glm::dvec2 imageSize(256, 256);

  std::shared_ptr<CesiumNativeTests::FileAccessor> pAssetAccessor =
      std::make_shared<CesiumNativeTests::FileAccessor>(
          CesiumNativeTests::FileAccessor{});
  AsyncSystem asyncSystem{
      std::make_shared<CesiumNativeTests::SimpleTaskProcessor>()};

  CreateRasterOverlayTileProviderParameters parameters{
      {pAssetAccessor, nullptr, asyncSystem}};
  IntrusivePointer<VectorTilesRasterOverlay> pOverlay;
  pOverlay.emplace(
      "overlay0",
      "file:///" + inputPath.string(),
      CesiumVectorOverlays::VectorTilesRasterOverlayOptions{
          CesiumVectorData::VectorStyle{CesiumUtility::Color(255, 0, 0, 255)},
          {},
          nullptr},
      RasterOverlayOptions{});

  IntrusivePointer<CesiumRasterOverlays::ActivatedRasterOverlay> pActivated =
      pOverlay->activate(
          CesiumRasterOverlays::RasterOverlayExternals{
              .pAssetAccessor = pAssetAccessor,
              .pPrepareRendererResources = nullptr,
              .asyncSystem = asyncSystem,
              .pCreditSystem = nullptr,
              .pLogger = spdlog::default_logger()},
          CesiumGeospatial::Ellipsoid::WGS84);

  pActivated->getReadyEvent().waitInMainThread();
  REQUIRE(pActivated->getTileProvider() != nullptr);

  CesiumGeospatial::GeographicProjection projection(
      CesiumGeospatial::Ellipsoid::WGS84);
  const CesiumGeometry::Rectangle fullRectangle =
      projection.project(CesiumGeospatial::GlobeRectangle{
          0.2847674617338015,
          0.8410564758787692,
          0.2867604043374349,
          0.8416788934512869});

  // Generate random tiles but use a constant seed so the results are the same
  // every run.
  std::default_random_engine rand(0xabcdabcd);
  std::uniform_real_distribution<double> dist(0, 1);

  const std::chrono::time_point start = std::chrono::steady_clock::now();

  for (size_t i = 0; i < BENCHMARK_ITERATIONS; i++) {
    const double x1 = dist(rand);
    const double x2 = dist(rand);
    const double y1 = dist(rand);
    const double y2 = dist(rand);

    const CesiumGeometry::Rectangle thisRect{
        std::min(x1, x2) * fullRectangle.computeWidth() +
            fullRectangle.minimumX,
        std::max(x1, x2) * fullRectangle.computeWidth() +
            fullRectangle.minimumX,
        std::min(y1, y2) * fullRectangle.computeHeight() +
            fullRectangle.minimumY,
        std::max(y1, y2) * fullRectangle.computeHeight() +
            fullRectangle.minimumY};

    IntrusivePointer<RasterOverlayTile> pTile;
    pTile.emplace(*pActivated, imageSize, thisRect);

    while (pTile->getState() !=
           CesiumRasterOverlays::RasterOverlayTile::LoadState::Loaded) {
      asyncSystem.dispatchMainThreadTasks();
      pActivated->tick();
    }
  }

  const std::chrono::time_point end = std::chrono::steady_clock::now();
  const int64_t duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
          .count();
  spdlog::info(
      "VectorTilesRasterOverlay vienna-streets benchmark time: {}",
      duration);
}