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
#include <glm/geometric.hpp>

#include <random>

const size_t BENCHMARK_ITERATIONS = 100000;

using namespace CesiumAsync;
using namespace CesiumRasterOverlays;
using namespace CesiumUtility;
using namespace CesiumVectorOverlays;
using namespace CesiumVectorData;

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

namespace {

class TestVectorStylingProvider final : public VectorStylingProvider {
  virtual CesiumAsync::Future<
      std::vector<std::optional<CesiumVectorData::PointStyle>>>
  onStylePoints(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const CesiumGltf::Model& /*model*/,
      const std::vector<int64_t>& /*featureIds*/,
      const std::vector<CesiumGeospatial::Cartographic>& points) override {
    if (points.empty()) {
      return asyncSystem.createResolvedFuture<
          std::vector<std::optional<CesiumVectorData::PointStyle>>>(
          std::vector<std::optional<CesiumVectorData::PointStyle>>{});
    }

    std::vector<std::optional<CesiumVectorData::PointStyle>> styles;
    styles.reserve(points.size());
    for (const CesiumGeospatial::Cartographic& point : points) {
      const glm::dvec3 projectedPoint = this->projection.project(point);
      const double longNorm = (projectedPoint.x - rectangle.minimumX) /
                              (rectangle.maximumX - rectangle.minimumX);
      const double latNorm = (projectedPoint.y - rectangle.minimumY) /
                             (rectangle.maximumY - rectangle.minimumY);
      CesiumVectorData::PointStyle style;
      style.radius = 2.0 + 4.0 * longNorm;
      style.fill = ColorStyle{
          CesiumUtility::Color(
              static_cast<uint8_t>(255 * longNorm),
              static_cast<uint8_t>(255 * latNorm),
              0,
              static_cast<uint8_t>(127 + 127 * longNorm)),
          ColorMode::Normal};
      styles.emplace_back(style);
    }

    return asyncSystem.createResolvedFuture<
        std::vector<std::optional<CesiumVectorData::PointStyle>>>(
        std::move(styles));
  }

  virtual CesiumAsync::Future<
      std::vector<std::optional<CesiumVectorData::LineStyle>>>
  onStylePolylines(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const CesiumGltf::Model& /*model*/,
      const std::vector<int64_t>& /*featureIds*/,
      const std::vector<std::vector<CesiumGeospatial::Cartographic>>& polylines)
      override {
    if (polylines.empty() || polylines[0].size() < 2) {
      return asyncSystem.createResolvedFuture<
          std::vector<std::optional<CesiumVectorData::LineStyle>>>(
          std::vector<std::optional<CesiumVectorData::LineStyle>>{});
    }

    std::vector<std::optional<CesiumVectorData::LineStyle>> styles;
    styles.reserve(polylines.size());
    for (const std::vector<CesiumGeospatial::Cartographic>& polyline :
         polylines) {
      const glm::dvec3 projectedPoint = this->projection.project(polyline[0]);
      double longNorm = (projectedPoint.x - rectangle.minimumX) /
                        (rectangle.maximumX - rectangle.minimumX);
      double latNorm = (projectedPoint.y - rectangle.minimumY) /
                       (rectangle.maximumY - rectangle.minimumY);
      CesiumVectorData::LineStyle style;
      style.width = 0.5;
      style.color = CesiumUtility::Color(
          static_cast<uint8_t>(255 * longNorm),
          static_cast<uint8_t>(255 * latNorm),
          static_cast<uint8_t>(255 * (1.0 - latNorm)),
          static_cast<uint8_t>(127 + 127 * (1.0 - longNorm)));
      styles.emplace_back(style);
    }

    return asyncSystem.createResolvedFuture<
        std::vector<std::optional<CesiumVectorData::LineStyle>>>(
        std::move(styles));
  }

  virtual CesiumAsync::Future<
      std::vector<std::optional<CesiumVectorData::PolygonStyle>>>
  onStylePolygons(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const CesiumGltf::Model& /*model*/,
      const std::vector<int64_t>& /*featureIds*/,
      const std::vector<std::vector<CesiumGeospatial::Cartographic>>& polygons)
      override {
    if (polygons.empty() || polygons[0].size() < 3) {
      return asyncSystem.createResolvedFuture<
          std::vector<std::optional<CesiumVectorData::PolygonStyle>>>(
          std::vector<std::optional<CesiumVectorData::PolygonStyle>>{});
    }

    std::vector<std::optional<CesiumVectorData::PolygonStyle>> styles;
    styles.reserve(polygons.size());
    for (const std::vector<CesiumGeospatial::Cartographic>& polygon :
         polygons) {
      const glm::dvec3 projectedPoint = this->projection.project(polygon[0]);
      double longNorm = (projectedPoint.x - rectangle.minimumX) /
                        (rectangle.maximumX - rectangle.minimumX);
      double latNorm = (projectedPoint.y - rectangle.minimumY) /
                       (rectangle.maximumY - rectangle.minimumY);
      CesiumVectorData::PolygonStyle style;
      style.fill = ColorStyle{
          CesiumUtility::Color(
              0,
              static_cast<uint8_t>(255 * latNorm),
              static_cast<uint8_t>(255 * longNorm),
              static_cast<uint8_t>(127 + 127 * latNorm)),
          ColorMode::Normal};
      styles.emplace_back(style);
    }

    return asyncSystem.createResolvedFuture<
        std::vector<std::optional<CesiumVectorData::PolygonStyle>>>(
        std::move(styles));
  }

public:
  TestVectorStylingProvider(const CesiumGeometry::Rectangle& rectangle)
      : rectangle(rectangle), projection(CesiumGeospatial::Ellipsoid::WGS84) {}

  CesiumGeometry::Rectangle rectangle;
  CesiumGeospatial::GeographicProjection projection;
};
} // namespace

TEST_CASE("VectorTilesRasterOverlay works with a styling provider") {
  Cesium3DTilesContent::registerAllTileContentTypes();

  const glm::dvec2 imageSize(256, 256);
  const std::filesystem::path dataPath =
      std::filesystem::path(CesiumVectorOverlays_TEST_DATA_DIR);
  const std::filesystem::path inputPath =
      dataPath / "PhillyStressTest" / "tileset.json";
  const std::filesystem::path referencePath =
      dataPath / "PhillyStressTest" / "rasterized.tga";
  const std::filesystem::path tempOutPath =
      std::filesystem::path(CESIUM_NATIVE_TEMP_DIR) / "vector-tile-styling.tga";
  const CesiumGeospatial::GlobeRectangle bounds =
      CesiumGeospatial::GlobeRectangle::fromDegrees(
          -75.169901705834391,
          39.944477809501223,
          -75.158766626966226,
          39.950235009555549);
  CesiumGeospatial::GeographicProjection projection(
      CesiumGeospatial::Ellipsoid::WGS84);
  const CesiumGeometry::Rectangle fullRectangle = projection.project(bounds);

  std::shared_ptr<CesiumNativeTests::FileAccessor> pAssetAccessor =
      std::make_shared<CesiumNativeTests::FileAccessor>(
          CesiumNativeTests::FileAccessor{});
  AsyncSystem asyncSystem{
      std::make_shared<CesiumNativeTests::SimpleTaskProcessor>()};

  CreateRasterOverlayTileProviderParameters parameters{
      {pAssetAccessor, nullptr, asyncSystem}};

  CesiumVectorData::VectorStyle defaultStyle;
  RasterOverlayOptions options;
  options.maximumScreenSpaceError = 1.0;

  std::shared_ptr<VectorStylingProvider> pStylingProvider =
      std::make_shared<TestVectorStylingProvider>(fullRectangle);

  IntrusivePointer<VectorTilesRasterOverlay> pOverlay;
  pOverlay.emplace(
      "overlay0",
      "file:///" + inputPath.string(),
      CesiumVectorOverlays::VectorTilesRasterOverlayOptions{
          defaultStyle,
          {},
          pStylingProvider},
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

  IntrusivePointer<RasterOverlayTile> pTile;
  pTile.emplace(*pActivated, imageSize, fullRectangle);
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