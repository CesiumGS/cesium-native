#include <CesiumAsync/AsyncSystem.h>
#include <CesiumGeospatial/BoundingRegionBuilder.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/checkFilesEqual.h>
#include <CesiumNativeTests/readFile.h>
#include <CesiumNativeTests/writeTga.h>
#include <CesiumRasterOverlays/ActivatedRasterOverlay.h>
#include <CesiumRasterOverlays/GeoJsonDocumentRasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumUtility/Math.h>
#include <CesiumUtility/Result.h>

#include <doctest/doctest.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <filesystem>
#include <memory>
#include <random>

using namespace CesiumAsync;
using namespace CesiumGeospatial;
using namespace CesiumUtility;
using namespace CesiumRasterOverlays;
using namespace CesiumVectorData;

const size_t BENCHMARK_ITERATIONS = 100000;

namespace {
CesiumGltf::ImageAsset rasterizeOverlayTile(
    const GlobeRectangle& rectangle,
    const glm::dvec2& imageSize,
    const std::filesystem::path& testDataPath,
    const GeoJsonDocumentRasterOverlayOptions& overlayOptions) {
  AsyncSystem asyncSystem(
      std::make_shared<CesiumNativeTests::SimpleTaskProcessor>());

  Result<GeoJsonDocument> docResult =
      GeoJsonDocument::fromGeoJson(readFile(testDataPath));
  CHECK(!docResult.errors.hasErrors());
  CHECK(docResult.errors.warnings.empty());
  REQUIRE(docResult.value);

  IntrusivePointer<GeoJsonDocumentRasterOverlay> pOverlay;
  pOverlay.emplace(
      asyncSystem,
      "overlay0",
      std::make_shared<GeoJsonDocument>(std::move(*docResult.value)),
      overlayOptions);

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
  GeographicProjection projection(Ellipsoid::WGS84);
  const CesiumGeometry::Rectangle& tileRect = projection.project(rectangle);

  IntrusivePointer<RasterOverlayTile> pTile =
      pActivated->getTile(tileRect, imageSize);
  pActivated->loadTile(*pTile);
  while (pTile->getState() != RasterOverlayTile::LoadState::Loaded) {
    asyncSystem.dispatchMainThreadTasks();
  }
  return *pTile->getImage();
}
} // namespace

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

  const std::chrono::time_point start = std::chrono::steady_clock::now();

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

  const std::chrono::time_point end = std::chrono::steady_clock::now();
  const int64_t duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
          .count();
  spdlog::info(
      "GeoJsonDocumentRasterOverlay vienna-streets benchmark time: {}",
      duration);
}

TEST_CASE("GeoJsonDocumentRasterOverlay can render lines with bounding box "
          "height set by pixels") {
  const std::filesystem::path testDataPath =
      std::filesystem::path(CesiumRasterOverlays_TEST_DATA_DIR) /
      "equator.geojson";

  GeoJsonDocumentRasterOverlayOptions options{
      VectorStyle{
          LineStyle{
              ColorStyle{Color{255, 0, 0, 255}, ColorMode::Normal},
              2.0,
              LineWidthMode::Pixels},
          PolygonStyle{std::nullopt, std::nullopt}},
      Ellipsoid::WGS84,
      0};

  CesiumGltf::ImageAsset image = rasterizeOverlayTile(
      GlobeRectangle::fromDegrees(0.0, -5.0, 5.0, 5.0),
      glm::dvec2(256, 256),
      testDataPath,
      options);

  // Tile size is divided by options.maximumScreenSpaceError which defaults
  // to 2.
  CHECK(image.width == 128);
  CHECK(image.height == 128);
  CesiumNativeTests::writeImageToTgaFile(image, "out-equator-meridian.tga");
  CesiumNativeTests::checkFilesEqual(
      std::filesystem::current_path() / "out-equator-meridian.tga",
      std::filesystem::path(CesiumRasterOverlays_TEST_DATA_DIR) /
          "equator-meridian.tga");
}

TEST_CASE("GeoJsonDocumentRasterOverlay can render lines with bounding box "
          "height set by meters") {
  const std::filesystem::path testDataPath =
      std::filesystem::path(CesiumRasterOverlays_TEST_DATA_DIR) /
      "equator.geojson";

  GeoJsonDocumentRasterOverlayOptions options{
      VectorStyle{
          LineStyle{
              ColorStyle{Color{255, 0, 0, 255}, ColorMode::Normal},
              // The equation for pixel width is (iw * lw) / (bw * r), where iw
              // = image width, lw = line width, bw = bounds width, and r is the
              // first radius of the ellipsoid. The bounds width is 2.0, the
              // image width is 128, the ellipsoid radius is 6378137.0, and we
              // want the pixel width to be 2, so solving (128 * x) / (2.0 *
              // 6378137.0) = 2 gives us x = 199316.78125
              199316.78125,
              LineWidthMode::Meters},
          PolygonStyle{std::nullopt, std::nullopt}},
      Ellipsoid::WGS84,
      0};

  CesiumGltf::ImageAsset image = rasterizeOverlayTile(
      GlobeRectangle(-1.0, -1.0, 1.0, 1.0),
      glm::dvec2(256, 256),
      testDataPath,
      options);

  // Tile size is divided by options.maximumScreenSpaceError which defaults
  // to 2.
  CHECK(image.width == 128);
  CHECK(image.height == 128);
  CesiumNativeTests::writeImageToTgaFile(
      image,
      "out-equator-meridian-meters.tga");
  // equator-meridian-meters *should* be identical to equator-meridian, except
  // that because of floating point imprecision the line width gets calculated
  // to be 1.999999999989966 instead of 2 and so it's not a perfect solid two
  // pixel line. But this is more or less "working as intended" and it would be
  // a lot of work to fix without any benefit to the end user.
  CesiumNativeTests::checkFilesEqual(
      std::filesystem::current_path() / "out-equator-meridian-meters.tga",
      std::filesystem::path(CesiumRasterOverlays_TEST_DATA_DIR) /
          "equator-meridian-meters.tga");
}

TEST_CASE("GeoJsonDocumentRasterOverlay can correctly rasterize line strings "
          "wrapping around the earth") {
  const std::filesystem::path testDataPath =
      std::filesystem::path(CesiumRasterOverlays_TEST_DATA_DIR) /
      "equator.geojson";

  GeoJsonDocumentRasterOverlayOptions options{
      VectorStyle{
          LineStyle{
              ColorStyle{Color{255, 0, 0, 255}, ColorMode::Normal},
              2.0,
              LineWidthMode::Pixels},
          PolygonStyle{std::nullopt, std::nullopt}},
      Ellipsoid::WGS84,
      0};

  {
    CesiumGltf::ImageAsset image = rasterizeOverlayTile(
        GlobeRectangle::fromDegrees(-175.0, -5.0, 175.0, 5.0),
        glm::dvec2(64, 64),
        testDataPath,
        options);

    CHECK(image.width == 32);
    CHECK(image.height == 32);
    CesiumNativeTests::writeImageToTgaFile(
        image,
        "out-equator-antimeridian-1.tga");
    CesiumNativeTests::checkFilesEqual(
        std::filesystem::current_path() / "out-equator-antimeridian-1.tga",
        std::filesystem::path(CesiumRasterOverlays_TEST_DATA_DIR) /
            "equator-antimeridian.tga");
  }

  {
    CesiumGltf::ImageAsset image = rasterizeOverlayTile(
        GlobeRectangle::fromDegrees(-180.0, -5.0, -170.0, 5.0),
        glm::dvec2(64, 64),
        testDataPath,
        options);

    CHECK(image.width == 32);
    CHECK(image.height == 32);
    CesiumNativeTests::writeImageToTgaFile(
        image,
        "out-equator-antimeridian-2.tga");
    CesiumNativeTests::checkFilesEqual(
        std::filesystem::current_path() / "out-equator-antimeridian-2.tga",
        std::filesystem::path(CesiumRasterOverlays_TEST_DATA_DIR) /
            "equator-antimeridian.tga");
  }

  {
    CesiumGltf::ImageAsset image = rasterizeOverlayTile(
        GlobeRectangle::fromDegrees(170.0, -5.0, 180.0, 5.0),
        glm::dvec2(64, 64),
        testDataPath,
        options);

    CHECK(image.width == 32);
    CHECK(image.height == 32);
    CesiumNativeTests::writeImageToTgaFile(
        image,
        "out-equator-antimeridian-3.tga");
    CesiumNativeTests::checkFilesEqual(
        std::filesystem::current_path() / "out-equator-antimeridian-3.tga",
        std::filesystem::path(CesiumRasterOverlays_TEST_DATA_DIR) /
            "equator-antimeridian.tga");
  }
}