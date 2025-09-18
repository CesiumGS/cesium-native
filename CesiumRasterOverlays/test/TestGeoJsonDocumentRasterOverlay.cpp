#include <CesiumAsync/AsyncSystem.h>
#include <CesiumGeospatial/BoundingRegionBuilder.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/readFile.h>
#include <CesiumRasterOverlays/ActivatedRasterOverlay.h>
#include <CesiumRasterOverlays/GeoJsonDocumentRasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumUtility/Math.h>
#include <CesiumUtility/Result.h>

#include <doctest/doctest.h>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <memory>
#include <random>

using namespace CesiumAsync;
using namespace CesiumGeospatial;
using namespace CesiumUtility;
using namespace CesiumRasterOverlays;
using namespace CesiumVectorData;

const size_t BENCHMARK_ITERATIONS = 100000;

TEST_CASE(
    "GeoJsonDocumentRasterOverlay vienna-streets benchmark" * doctest::skip()) {
  AsyncSystem asyncSystem(
      std::make_shared<CesiumNativeTests::SimpleTaskProcessor>());

  const std::filesystem::path testDataPath =
      std::filesystem::path(CesiumRasterOverlays_TEST_DATA_DIR) /
      "vienna-streets.geojson";
  Result<GeoJsonDocument> docResult =
      GeoJsonDocument::fromGeoJson(readFile(testDataPath));
  CHECK(!docResult.errors.hasErrors());
  CHECK(docResult.errors.warnings.empty());
  REQUIRE(docResult.value);

  BoundingRegionBuilder builder;
  for (const std::vector<glm::dvec3>& line :
       docResult.value->rootObject.lines()) {
    for (const glm::dvec3& point : line) {
      builder.expandToIncludePosition(
          Cartographic::fromDegrees(point.x, point.y, point.z));
    }
  }

  GeoJsonDocumentRasterOverlayOptions options{
      VectorStyle{
          LineStyle{
              ColorStyle{Color{255, 0, 0, 255}, ColorMode::Normal},
              2.0,
              LineWidthMode::Pixels},
          PolygonStyle{std::nullopt, std::nullopt}},
      Ellipsoid::WGS84,
      0};

  IntrusivePointer<GeoJsonDocumentRasterOverlay> pOverlay;
  pOverlay.emplace(
      asyncSystem,
      "overlay0",
      std::make_shared<GeoJsonDocument>(std::move(*docResult.value)),
      options);

  std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor =
      std::make_shared<CesiumNativeTests::SimpleAssetAccessor>(
          std::map<
              std::string,
              std::shared_ptr<CesiumNativeTests::SimpleAssetRequest>>());

  IntrusivePointer<ActivatedRasterOverlay> pActivated = pOverlay->activate(
      RasterOverlayExternals{
          .pAssetAccessor = pAssetAccessor,
          .pPrepareRendererResources = nullptr,
          .asyncSystem = asyncSystem,
          .pCreditSystem = nullptr,
          .pLogger = spdlog::default_logger()},
      Ellipsoid::WGS84);

  pActivated->getReadyEvent().waitInMainThread();

  REQUIRE(pActivated->getTileProvider() != nullptr);

  const CesiumGeometry::Rectangle fullRectangle =
      builder.toGlobeRectangle().toSimpleRectangle();
  RasterOverlayTile tile{*pActivated, glm::dvec2(256, 256), fullRectangle};

  // Generate random tiles but use a constant seed so the results are the same
  // every run.
  std::default_random_engine rand(0xabcdabcd);
  std::uniform_real_distribution<double> dist(0, 1);

  for (size_t i = 0; i < BENCHMARK_ITERATIONS; i++) {
    const double x1 = dist(rand);
    const double x2 = dist(rand);
    const double y1 = dist(rand);
    const double y2 = dist(rand);

    const CesiumGeometry::Rectangle thisRect{
        std::min(x1, x2),
        std::max(x1, x2),
        std::min(y1, y2),
        std::max(y1, y2)};

    IntrusivePointer<RasterOverlayTile> pTile;
    pTile.emplace(*pActivated, glm::dvec2(256, 256), thisRect);

    pActivated->loadTile(*pTile).waitInMainThread();
  }
}
