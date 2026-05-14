#include <Cesium3DTilesContent/ImplicitTilingUtilities.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumGeospatial/BoundingRegion.h>
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

using CesiumVectorOverlays::VectorTilesRasterOverlay;
using CesiumAsync::AsyncSystem;
using CesiumRasterOverlays::CreateRasterOverlayTileProviderParameters;
using CesiumRasterOverlays::RasterOverlayOptions;
using CesiumUtility::IntrusivePointer;

TEST_CASE("Test VectorTilesRasterOverlay") {
  const std::filesystem::path dataPath =
      std::filesystem::path(CesiumVectorOverlays_TEST_DATA_DIR);
  const std::filesystem::path inputPath =
      dataPath / "ViennaStreets" / "tileset.json";
  const std::filesystem::path referencePath =
      dataPath / "ViennaStreets" / "rasterized.tga";
  const std::filesystem::path tempOutPath =
      std::filesystem::path(CESIUM_NATIVE_TEMP_DIR) / "vector-tile-test.tga";

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
      CesiumVectorData::VectorStyle{CesiumUtility::Color(255, 0, 0, 255)},
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
  /*const std::filesystem::path dataPath =
      std::filesystem::path(CesiumVectorOverlays_TEST_DATA_DIR);*/
  const std::filesystem::path inputPath =
      "/mnt/d/Dev/vector/data/nyc_poly/tileset.json";
  /*const std::filesystem::path referencePath =
      dataPath / "ViennaStreets" / "rasterized.tga";*/
  const std::filesystem::path tempOutPath =
      std::filesystem::path(CESIUM_NATIVE_TEMP_DIR) / "vector-poly-test.tga";

  const glm::dvec2 imageSize(2048, 2048);

  // A rough rectangle around the Albertgarten in Vienna, as an arbitrary
  // testing area.
  const CesiumGeospatial::BoundingRegion& tileRegion =
      Cesium3DTilesContent::ImplicitTilingUtilities::computeBoundingVolume(
          CesiumGeospatial::BoundingRegion{
              CesiumGeospatial::GlobeRectangle{
                  -1.2960028825865837,
                  0.7068309334853016,
                  -1.2863087490444578,
                  0.7141023380205147},
              0,
              0.005,
              CesiumGeospatial::Ellipsoid::WGS84},
          CesiumGeometry::QuadtreeTileID(3, 5, 3),
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
  options.maximumScreenSpaceError = 1.0;

  IntrusivePointer<VectorTilesRasterOverlay> pOverlay;
  pOverlay.emplace(
      "overlay0",
      "file:///" + inputPath.string(),
      CesiumVectorData::VectorStyle{CesiumUtility::Color(255, 0, 0, 255)},
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

  // CesiumNativeTests::checkFilesEqual(tempOutPath, referencePath);
}