#include <CesiumAsync/AsyncSystem.h>
#include <CesiumGeospatial/BoundingRegionBuilder.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumImage/ImageAsset.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/checkFilesEqual.h>
#include <CesiumNativeTests/readFile.h>
#include <CesiumNativeTests/writeTga.h>
#include <CesiumRasterOverlays/ActivatedRasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumUtility/Color.h>
#include <CesiumUtility/Math.h>
#include <CesiumUtility/Result.h>
#include <CesiumVectorOverlays/GeoJsonDocumentRasterOverlay.h>

#include <doctest/doctest.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <filesystem>
#include <memory>
#include <random>
#include <vector>

using namespace CesiumAsync;
using namespace CesiumGeospatial;
using namespace CesiumUtility;
using namespace CesiumRasterOverlays;
using namespace CesiumVectorOverlays;
using namespace CesiumVectorData;

const size_t BENCHMARK_ITERATIONS = 100000;

namespace {
CesiumImage::ImageAsset rasterizeOverlayTileFromDocument(
    const GlobeRectangle& rectangle,
    const glm::dvec2& imageSize,
    GeoJsonDocument&& document,
    const GeoJsonDocumentRasterOverlayOptions& overlayOptions) {
  AsyncSystem asyncSystem(
      std::make_shared<CesiumNativeTests::SimpleTaskProcessor>());

  IntrusivePointer<GeoJsonDocumentRasterOverlay> pOverlay;
  pOverlay.emplace(
      asyncSystem,
      "overlay0",
      std::make_shared<GeoJsonDocument>(std::move(document)),
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

CesiumImage::ImageAsset rasterizeOverlayTile(
    const GlobeRectangle& rectangle,
    const glm::dvec2& imageSize,
    const std::filesystem::path& testDataPath,
    const GeoJsonDocumentRasterOverlayOptions& overlayOptions) {
  Result<GeoJsonDocument> docResult =
      GeoJsonDocument::fromGeoJson(readFile(testDataPath));
  CHECK(!docResult.errors.hasErrors());
  CHECK(docResult.errors.warnings.empty());
  REQUIRE(docResult.value);

  return rasterizeOverlayTileFromDocument(
      rectangle,
      imageSize,
      std::move(*docResult.value),
      overlayOptions);
}

bool imageHasPixelWithColor(
    const CesiumImage::ImageAsset& image,
    uint8_t r,
    uint8_t g,
    uint8_t b,
    uint8_t a) {
  REQUIRE(image.channels == 4);
  REQUIRE(image.bytesPerChannel == 1);
  REQUIRE(
      static_cast<size_t>(
          image.width * image.height * image.channels *
          image.bytesPerChannel) == image.pixelData.size());
  for (int32_t y = 0; y < image.height; y++) {
    for (int32_t x = 0; x < image.width; x++) {
      size_t idx = static_cast<size_t>((y * image.width + x) * 4);
      if (static_cast<uint8_t>(image.pixelData[idx]) == r &&
          static_cast<uint8_t>(image.pixelData[idx + 1]) == g &&
          static_cast<uint8_t>(image.pixelData[idx + 2]) == b &&
          static_cast<uint8_t>(image.pixelData[idx + 3]) == a) {
        return true;
      }
    }
  }
  return false;
}

GeoJsonDocument createFeatureCollectionWithLineString(
    const std::optional<VectorStyle>& geometryStyle,
    const std::optional<VectorStyle>& featureStyle,
    const std::optional<VectorStyle>& collectionStyle) {
  GeoJsonLineString lineString;
  lineString.coordinates = {
      glm::dvec3(0.0, 0.0, 0.0),
      glm::dvec3(5.0, 0.0, 0.0)};
  lineString.style = geometryStyle;

  GeoJsonFeature feature;
  feature.geometry =
      std::make_unique<GeoJsonObject>(GeoJsonObject{std::move(lineString)});
  feature.style = featureStyle;

  GeoJsonFeatureCollection collection;
  collection.features.push_back(GeoJsonObject{std::move(feature)});
  collection.style = collectionStyle;

  return GeoJsonDocument(GeoJsonObject{std::move(collection)}, {});
}

GeoJsonDocument createDocumentWithPoint(const glm::dvec3& coordinates) {
  GeoJsonPoint point;
  point.coordinates = coordinates;
  return GeoJsonDocument(GeoJsonObject{std::move(point)}, {});
}

GeoJsonDocument
createDocumentWithMultiPoint(const std::vector<glm::dvec3>& coordinates) {
  GeoJsonMultiPoint multiPoint;
  multiPoint.coordinates = coordinates;
  return GeoJsonDocument(GeoJsonObject{std::move(multiPoint)}, {});
}
} // namespace

TEST_CASE(
    "GeoJsonDocumentRasterOverlay vienna-streets benchmark" * doctest::skip()) {
  AsyncSystem asyncSystem(
      std::make_shared<CesiumNativeTests::SimpleTaskProcessor>());

  const std::filesystem::path testDataPath =
      std::filesystem::path(CesiumVectorOverlays_TEST_DATA_DIR) /
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
      std::filesystem::path(CesiumVectorOverlays_TEST_DATA_DIR) /
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

  CesiumImage::ImageAsset image = rasterizeOverlayTile(
      GlobeRectangle::fromDegrees(0.0, -5.0, 5.0, 5.0),
      glm::dvec2(256, 256),
      testDataPath,
      options);

  // Tile size is divided by options.maximumScreenSpaceError which defaults
  // to 2.
  CHECK(image.width == 128);
  CHECK(image.height == 128);
  CesiumNativeTests::writeImageToTgaFile(
      image,
      std::filesystem::path(CESIUM_NATIVE_TEMP_DIR) /
          "out-equator-meridian.tga");
  CesiumNativeTests::checkFilesEqual(
      std::filesystem::path(CESIUM_NATIVE_TEMP_DIR) /
          "out-equator-meridian.tga",
      std::filesystem::path(CesiumVectorOverlays_TEST_DATA_DIR) /
          "equator-meridian.tga");
}

TEST_CASE("GeoJsonDocumentRasterOverlay can render lines with bounding box "
          "height set by meters") {
  const std::filesystem::path testDataPath =
      std::filesystem::path(CesiumVectorOverlays_TEST_DATA_DIR) /
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

  CesiumImage::ImageAsset image = rasterizeOverlayTile(
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
      std::filesystem::path(CESIUM_NATIVE_TEMP_DIR) /
          "out-equator-meridian-meters.tga");
  // equator-meridian-meters *should* be identical to equator-meridian, except
  // that because of floating point imprecision the line width gets calculated
  // to be 1.999999999989966 instead of 2 and so it's not a perfect solid two
  // pixel line. But this is more or less "working as intended" and it would be
  // a lot of work to fix without any benefit to the end user.
  CesiumNativeTests::checkFilesEqual(
      std::filesystem::path(CESIUM_NATIVE_TEMP_DIR) /
          "out-equator-meridian-meters.tga",
      std::filesystem::path(CesiumVectorOverlays_TEST_DATA_DIR) /
          "equator-meridian-meters.tga");
}

TEST_CASE("GeoJsonDocumentRasterOverlay can correctly rasterize line strings "
          "wrapping around the earth") {
  const std::filesystem::path testDataPath =
      std::filesystem::path(CesiumVectorOverlays_TEST_DATA_DIR) /
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
    CesiumImage::ImageAsset image = rasterizeOverlayTile(
        GlobeRectangle::fromDegrees(-175.0, -5.0, 175.0, 5.0),
        glm::dvec2(64, 64),
        testDataPath,
        options);

    CHECK(image.width == 32);
    CHECK(image.height == 32);
    CesiumNativeTests::writeImageToTgaFile(
        image,
        std::filesystem::path(CESIUM_NATIVE_TEMP_DIR) /
            "out-equator-antimeridian-1.tga");
    CesiumNativeTests::checkFilesEqual(
        std::filesystem::path(CESIUM_NATIVE_TEMP_DIR) /
            "out-equator-antimeridian-1.tga",
        std::filesystem::path(CesiumVectorOverlays_TEST_DATA_DIR) /
            "equator-antimeridian.tga");
  }

  {
    CesiumImage::ImageAsset image = rasterizeOverlayTile(
        GlobeRectangle::fromDegrees(-180.0, -5.0, -170.0, 5.0),
        glm::dvec2(64, 64),
        testDataPath,
        options);

    CHECK(image.width == 32);
    CHECK(image.height == 32);
    CesiumNativeTests::writeImageToTgaFile(
        image,
        std::filesystem::path(CESIUM_NATIVE_TEMP_DIR) /
            "out-equator-antimeridian-2.tga");
    CesiumNativeTests::checkFilesEqual(
        std::filesystem::path(CESIUM_NATIVE_TEMP_DIR) /
            "out-equator-antimeridian-2.tga",
        std::filesystem::path(CesiumVectorOverlays_TEST_DATA_DIR) /
            "equator-antimeridian.tga");
  }

  {
    CesiumImage::ImageAsset image = rasterizeOverlayTile(
        GlobeRectangle::fromDegrees(170.0, -5.0, 180.0, 5.0),
        glm::dvec2(64, 64),
        testDataPath,
        options);

    CHECK(image.width == 32);
    CHECK(image.height == 32);
    CesiumNativeTests::writeImageToTgaFile(
        image,
        std::filesystem::path(CESIUM_NATIVE_TEMP_DIR) /
            "out-equator-antimeridian-3.tga");
    CesiumNativeTests::checkFilesEqual(
        std::filesystem::path(CESIUM_NATIVE_TEMP_DIR) /
            "out-equator-antimeridian-3.tga",
        std::filesystem::path(CesiumVectorOverlays_TEST_DATA_DIR) /
            "equator-antimeridian.tga");
  }
}

TEST_CASE("GeoJsonDocumentRasterOverlay FeatureCollection uses geometry style "
          "over feature and collection styles") {
  const VectorStyle geometryStyle{
      LineStyle{
          ColorStyle{Color{0, 255, 0, 255}, ColorMode::Normal},
          2.0,
          LineWidthMode::Pixels},
      PolygonStyle{std::nullopt, std::nullopt}};
  const VectorStyle featureStyle{
      LineStyle{
          ColorStyle{Color{0, 0, 255, 255}, ColorMode::Normal},
          2.0,
          LineWidthMode::Pixels},
      PolygonStyle{std::nullopt, std::nullopt}};
  const VectorStyle collectionStyle{
      LineStyle{
          ColorStyle{Color{255, 255, 0, 255}, ColorMode::Normal},
          2.0,
          LineWidthMode::Pixels},
      PolygonStyle{std::nullopt, std::nullopt}};

  GeoJsonDocumentRasterOverlayOptions options{
      VectorStyle{
          LineStyle{
              ColorStyle{Color{255, 0, 0, 255}, ColorMode::Normal},
              2.0,
              LineWidthMode::Pixels},
          PolygonStyle{std::nullopt, std::nullopt}},
      Ellipsoid::WGS84,
      0};

  GeoJsonDocument doc = createFeatureCollectionWithLineString(
      geometryStyle,
      featureStyle,
      collectionStyle);

  CesiumImage::ImageAsset image = rasterizeOverlayTileFromDocument(
      GlobeRectangle::fromDegrees(-1.0, -5.0, 6.0, 5.0),
      glm::dvec2(256, 256),
      std::move(doc),
      options);

  CHECK(imageHasPixelWithColor(image, 0, 255, 0, 255));
  CHECK_FALSE(imageHasPixelWithColor(image, 0, 0, 255, 255));
  CHECK_FALSE(imageHasPixelWithColor(image, 255, 255, 0, 255));
  CHECK_FALSE(imageHasPixelWithColor(image, 255, 0, 0, 255));
}

TEST_CASE("GeoJsonDocumentRasterOverlay FeatureCollection falls back to "
          "feature style when geometry has no style") {
  const VectorStyle featureStyle{
      LineStyle{
          ColorStyle{Color{0, 0, 255, 255}, ColorMode::Normal},
          2.0,
          LineWidthMode::Pixels},
      PolygonStyle{std::nullopt, std::nullopt}};
  const VectorStyle collectionStyle{
      LineStyle{
          ColorStyle{Color{255, 255, 0, 255}, ColorMode::Normal},
          2.0,
          LineWidthMode::Pixels},
      PolygonStyle{std::nullopt, std::nullopt}};

  GeoJsonDocumentRasterOverlayOptions options{
      VectorStyle{
          LineStyle{
              ColorStyle{Color{255, 0, 0, 255}, ColorMode::Normal},
              2.0,
              LineWidthMode::Pixels},
          PolygonStyle{std::nullopt, std::nullopt}},
      Ellipsoid::WGS84,
      0};

  GeoJsonDocument doc = createFeatureCollectionWithLineString(
      std::nullopt,
      featureStyle,
      collectionStyle);

  CesiumImage::ImageAsset image = rasterizeOverlayTileFromDocument(
      GlobeRectangle::fromDegrees(-1.0, -5.0, 6.0, 5.0),
      glm::dvec2(256, 256),
      std::move(doc),
      options);

  CHECK(imageHasPixelWithColor(image, 0, 0, 255, 255));
  CHECK_FALSE(imageHasPixelWithColor(image, 255, 255, 0, 255));
  CHECK_FALSE(imageHasPixelWithColor(image, 255, 0, 0, 255));
}

TEST_CASE("GeoJsonDocumentRasterOverlay FeatureCollection falls back to "
          "collection style when geometry and feature have no style") {
  const VectorStyle collectionStyle{
      LineStyle{
          ColorStyle{Color{255, 255, 0, 255}, ColorMode::Normal},
          2.0,
          LineWidthMode::Pixels},
      PolygonStyle{std::nullopt, std::nullopt}};

  GeoJsonDocumentRasterOverlayOptions options{
      VectorStyle{
          LineStyle{
              ColorStyle{Color{255, 0, 0, 255}, ColorMode::Normal},
              2.0,
              LineWidthMode::Pixels},
          PolygonStyle{std::nullopt, std::nullopt}},
      Ellipsoid::WGS84,
      0};

  GeoJsonDocument doc = createFeatureCollectionWithLineString(
      std::nullopt,
      std::nullopt,
      collectionStyle);

  CesiumImage::ImageAsset image = rasterizeOverlayTileFromDocument(
      GlobeRectangle::fromDegrees(-1.0, -5.0, 6.0, 5.0),
      glm::dvec2(256, 256),
      std::move(doc),
      options);

  CHECK(imageHasPixelWithColor(image, 255, 255, 0, 255));
  CHECK_FALSE(imageHasPixelWithColor(image, 255, 0, 0, 255));
}

TEST_CASE("GeoJsonDocumentRasterOverlay FeatureCollection falls back to "
          "default style when no styles are set") {
  GeoJsonDocumentRasterOverlayOptions options{
      VectorStyle{
          LineStyle{
              ColorStyle{Color{255, 0, 0, 255}, ColorMode::Normal},
              2.0,
              LineWidthMode::Pixels},
          PolygonStyle{std::nullopt, std::nullopt}},
      Ellipsoid::WGS84,
      0};

  GeoJsonDocument doc = createFeatureCollectionWithLineString(
      std::nullopt,
      std::nullopt,
      std::nullopt);

  CesiumImage::ImageAsset image = rasterizeOverlayTileFromDocument(
      GlobeRectangle::fromDegrees(-1.0, -5.0, 6.0, 5.0),
      glm::dvec2(256, 256),
      std::move(doc),
      options);

  CHECK(imageHasPixelWithColor(image, 255, 0, 0, 255));
}

TEST_CASE(
    "GeoJsonDocumentRasterOverlay can render a Point as a filled circle") {
  GeoJsonDocumentRasterOverlayOptions options{
      VectorStyle{
          LineStyle{
              ColorStyle{Color{255, 0, 0, 255}, ColorMode::Normal},
              2.0,
              LineWidthMode::Pixels},
          PolygonStyle{std::nullopt, std::nullopt},
          PointStyle{
              5.0,
              ColorStyle{Color{0, 255, 0, 255}, ColorMode::Normal},
              std::nullopt}},
      Ellipsoid::WGS84,
      0};

  CesiumImage::ImageAsset image = rasterizeOverlayTileFromDocument(
      GlobeRectangle::fromDegrees(-1.0, -5.0, 6.0, 5.0),
      glm::dvec2(256, 256),
      createDocumentWithPoint(glm::dvec3(2.5, 0.0, 0.0)),
      options);

  // The point's fill color should appear in the rasterized tile.
  CHECK(imageHasPixelWithColor(image, 0, 255, 0, 255));
}

TEST_CASE("GeoJsonDocumentRasterOverlay can render a Point with an outline") {
  GeoJsonDocumentRasterOverlayOptions options{
      VectorStyle{
          LineStyle{
              ColorStyle{Color{255, 0, 0, 255}, ColorMode::Normal},
              2.0,
              LineWidthMode::Pixels},
          PolygonStyle{std::nullopt, std::nullopt},
          PointStyle{
              6.0,
              ColorStyle{Color{0, 255, 0, 255}, ColorMode::Normal},
              LineStyle{
                  ColorStyle{Color{0, 0, 255, 255}, ColorMode::Normal},
                  2.0,
                  LineWidthMode::Pixels}}},
      Ellipsoid::WGS84,
      0};

  CesiumImage::ImageAsset image = rasterizeOverlayTileFromDocument(
      GlobeRectangle::fromDegrees(-1.0, -5.0, 6.0, 5.0),
      glm::dvec2(256, 256),
      createDocumentWithPoint(glm::dvec3(2.5, 0.0, 0.0)),
      options);

  // Both the fill and outline colors should appear in the rasterized tile.
  CHECK(imageHasPixelWithColor(image, 0, 255, 0, 255));
  CHECK(imageHasPixelWithColor(image, 0, 0, 255, 255));
}

TEST_CASE("GeoJsonDocumentRasterOverlay renders a Point whose outline width is "
          "specified in meters") {
  // A point's fill radius is always in pixels, but its outline is a LineStyle
  // and may be specified in meters. Such an outline must be treated as meters
  // (folded into the geometry's bounding region) rather than being mistaken for
  // a very large pixel footprint, which previously distorted point rendering.
  GeoJsonDocumentRasterOverlayOptions options{
      VectorStyle{
          LineStyle{
              ColorStyle{Color{255, 0, 0, 255}, ColorMode::Normal},
              2.0,
              LineWidthMode::Pixels},
          PolygonStyle{std::nullopt, std::nullopt},
          PointStyle{
              6.0,
              ColorStyle{Color{0, 255, 0, 255}, ColorMode::Normal},
              LineStyle{
                  ColorStyle{Color{0, 0, 255, 255}, ColorMode::Normal},
                  50000.0,
                  LineWidthMode::Meters}}},
      Ellipsoid::WGS84,
      0};

  CesiumImage::ImageAsset image = rasterizeOverlayTileFromDocument(
      GlobeRectangle::fromDegrees(-1.0, -5.0, 6.0, 5.0),
      glm::dvec2(256, 256),
      createDocumentWithPoint(glm::dvec3(2.5, 0.0, 0.0)),
      options);

  // The point should still render as a filled, outlined circle rather than a
  // distorted shape: both the fill and outline colors should be present.
  CHECK(imageHasPixelWithColor(image, 0, 255, 0, 255));
  CHECK(imageHasPixelWithColor(image, 0, 0, 255, 255));
  // A corner far from the point must remain transparent; a meters outline
  // wrongly treated as a huge pixel footprint would bleed the point across the
  // tile.
  const size_t cornerIdx = 0;
  CHECK(static_cast<uint8_t>(image.pixelData[cornerIdx + 3]) == 0);
}

TEST_CASE("GeoJsonDocumentRasterOverlay can render a MultiPoint") {
  GeoJsonDocumentRasterOverlayOptions options{
      VectorStyle{
          LineStyle{
              ColorStyle{Color{255, 0, 0, 255}, ColorMode::Normal},
              2.0,
              LineWidthMode::Pixels},
          PolygonStyle{std::nullopt, std::nullopt},
          PointStyle{
              5.0,
              ColorStyle{Color{0, 255, 0, 255}, ColorMode::Normal},
              std::nullopt}},
      Ellipsoid::WGS84,
      0};

  CesiumImage::ImageAsset image = rasterizeOverlayTileFromDocument(
      GlobeRectangle::fromDegrees(-5.0, -5.0, 5.0, 5.0),
      glm::dvec2(256, 256),
      createDocumentWithMultiPoint(
          {glm::dvec3(-2.5, 0.0, 0.0), glm::dvec3(2.5, 0.0, 0.0)}),
      options);

  // The MultiPoint's fill color should appear in the rasterized tile.
  CHECK(imageHasPixelWithColor(image, 0, 255, 0, 255));
}

namespace {
VectorStyle createLineStyle(const Color& color, double width = 2.0) {
  return VectorStyle{
      LineStyle{
          ColorStyle{color, ColorMode::Normal},
          width,
          LineWidthMode::Pixels},
      PolygonStyle{std::nullopt, std::nullopt}};
}

const Color RED{255, 0, 0, 255};
const Color GREEN{0, 255, 0, 255};
const Color BLUE{0, 0, 255, 255};

bool imageHasColor(const CesiumImage::ImageAsset& image, const Color& color) {
  return imageHasPixelWithColor(image, color.r, color.g, color.b, color.a);
}

// A FeatureCollection with one unstyled line feature per entry, each along the
// equator from `west` to `west + 5` degrees.
std::shared_ptr<GeoJsonDocument>
createLinesDocument(const std::vector<double>& wests) {
  GeoJsonFeatureCollection collection;
  for (double west : wests) {
    GeoJsonLineString lineString;
    lineString.coordinates = {
        glm::dvec3(west, 0.0, 0.0),
        glm::dvec3(west + 5.0, 0.0, 0.0)};

    GeoJsonFeature feature;
    feature.geometry =
        std::make_unique<GeoJsonObject>(GeoJsonObject{std::move(lineString)});
    collection.features.push_back(GeoJsonObject{std::move(feature)});
  }
  return std::make_shared<GeoJsonDocument>(
      GeoJsonObject{std::move(collection)},
      std::vector<VectorDocumentAttribution>{});
}

GeoJsonFeature& getFeature(GeoJsonDocument& document, size_t index) {
  GeoJsonFeatureCollection* pCollection =
      document.rootObject.getIf<GeoJsonFeatureCollection>();
  REQUIRE(pCollection != nullptr);
  REQUIRE(index < pCollection->features.size());
  GeoJsonFeature* pFeature =
      pCollection->features[index].getIf<GeoJsonFeature>();
  REQUIRE(pFeature != nullptr);
  return *pFeature;
}

GlobeRectangle aroundLine(double west) {
  return GlobeRectangle::fromDegrees(west - 1.0, -5.0, west + 6.0, 5.0);
}

IntrusivePointer<ActivatedRasterOverlay>
activateOverlay(const AsyncSystem& asyncSystem, const RasterOverlay& overlay) {
  std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor =
      std::make_shared<CesiumNativeTests::SimpleAssetAccessor>(
          std::map<
              std::string,
              std::shared_ptr<CesiumNativeTests::SimpleAssetRequest>>());

  IntrusivePointer<ActivatedRasterOverlay> pActivated = overlay.activate(
      RasterOverlayExternals{
          .pAssetAccessor = pAssetAccessor,
          .pPrepareRendererResources = nullptr,
          .asyncSystem = asyncSystem,
          .pCreditSystem = nullptr,
          .pLogger = spdlog::default_logger()},
      Ellipsoid::WGS84);
  pActivated->getReadyEvent().waitInMainThread();
  REQUIRE(pActivated->getTileProvider() != nullptr);
  return pActivated;
}

IntrusivePointer<RasterOverlayTile> loadTile(
    AsyncSystem& asyncSystem,
    ActivatedRasterOverlay& activated,
    const GlobeRectangle& rectangle) {
  GeographicProjection projection(Ellipsoid::WGS84);
  IntrusivePointer<RasterOverlayTile> pTile =
      activated.getTile(projection.project(rectangle), glm::dvec2(256, 256));
  REQUIRE(pTile != nullptr);
  activated.loadTile(*pTile);
  while (pTile->getState() != RasterOverlayTile::LoadState::Loaded &&
         pTile->getState() != RasterOverlayTile::LoadState::Failed) {
    asyncSystem.dispatchMainThreadTasks();
  }
  REQUIRE(pTile->getState() == RasterOverlayTile::LoadState::Loaded);
  REQUIRE(pTile->getImage() != nullptr);
  return pTile;
}

// Gives the overlay's tile provider a chance to pick up changes, like a tileset
// does every frame.
void update(AsyncSystem& asyncSystem, ActivatedRasterOverlay& activated) {
  activated.tick();
  asyncSystem.dispatchMainThreadTasks();
}

const GeoJsonDocumentRasterOverlayOptions defaultOptions{
    createLineStyle(RED),
    Ellipsoid::WGS84,
    0};
} // namespace

TEST_CASE("GeoJsonDocumentRasterOverlay refreshStyles re-rasterizes only the "
          "tiles whose styles changed") {
  AsyncSystem asyncSystem(
      std::make_shared<CesiumNativeTests::SimpleTaskProcessor>());
  std::shared_ptr<GeoJsonDocument> pDocument = createLinesDocument({0.0, 50.0});
  IntrusivePointer<GeoJsonDocumentRasterOverlay> pOverlay =
      new GeoJsonDocumentRasterOverlay(
          asyncSystem,
          "overlay",
          pDocument,
          defaultOptions);
  IntrusivePointer<ActivatedRasterOverlay> pActivated =
      activateOverlay(asyncSystem, *pOverlay);

  IntrusivePointer<RasterOverlayTile> pTileA =
      loadTile(asyncSystem, *pActivated, aroundLine(0.0));
  IntrusivePointer<RasterOverlayTile> pTileB =
      loadTile(asyncSystem, *pActivated, aroundLine(50.0));
  CHECK(imageHasColor(*pTileA->getImage(), RED));

  // Changing a style in place has no effect until refreshStyles is called. In
  // particular, tiles rasterized in the meantime still use the old style.
  getFeature(*pDocument, 0).style = createLineStyle(BLUE);
  update(asyncSystem, *pActivated);
  CHECK(!pActivated->isTileInvalidated(*pTileA));
  IntrusivePointer<RasterOverlayTile> pTileA2 =
      loadTile(asyncSystem, *pActivated, aroundLine(0.0));
  CHECK(imageHasColor(*pTileA2->getImage(), RED));
  CHECK_FALSE(imageHasColor(*pTileA2->getImage(), BLUE));

  pOverlay->refreshStyles();
  update(asyncSystem, *pActivated);

  CHECK(pActivated->isTileInvalidated(*pTileA));
  CHECK(pActivated->isTileInvalidated(*pTileA2));
  CHECK(!pActivated->isTileInvalidated(*pTileB));

  IntrusivePointer<RasterOverlayTile> pTileA3 =
      loadTile(asyncSystem, *pActivated, aroundLine(0.0));
  CHECK(imageHasColor(*pTileA3->getImage(), BLUE));
  CHECK_FALSE(imageHasColor(*pTileA3->getImage(), RED));
  CHECK(!pActivated->isTileInvalidated(*pTileA3));

  // Refreshing again without changes invalidates nothing.
  pOverlay->refreshStyles();
  update(asyncSystem, *pActivated);
  CHECK(!pActivated->isTileInvalidated(*pTileA3));
  CHECK(!pActivated->isTileInvalidated(*pTileB));
}

TEST_CASE("GeoJsonDocumentRasterOverlay refreshStyles invalidates neighboring "
          "tiles that a line with a pixel width spills into") {
  AsyncSystem asyncSystem(
      std::make_shared<CesiumNativeTests::SimpleTaskProcessor>());
  std::shared_ptr<GeoJsonDocument> pDocument = createLinesDocument({0.0});
  IntrusivePointer<GeoJsonDocumentRasterOverlay> pOverlay =
      new GeoJsonDocumentRasterOverlay(
          asyncSystem,
          "overlay",
          pDocument,
          defaultOptions);
  IntrusivePointer<ActivatedRasterOverlay> pActivated =
      activateOverlay(asyncSystem, *pOverlay);

  // The line ends at 5 degrees. This tile starts just east of that, closer
  // than the line's width in this tile's pixels (each about 0.04 degrees).
  IntrusivePointer<RasterOverlayTile> pNear = loadTile(
      asyncSystem,
      *pActivated,
      GlobeRectangle::fromDegrees(5.05, -5.0, 10.0, 5.0));
  // This one is well clear of it.
  IntrusivePointer<RasterOverlayTile> pFar = loadTile(
      asyncSystem,
      *pActivated,
      GlobeRectangle::fromDegrees(7.0, -5.0, 12.0, 5.0));

  getFeature(*pDocument, 0).style = createLineStyle(BLUE, 4.0);
  pOverlay->refreshStyles();
  update(asyncSystem, *pActivated);

  CHECK(pActivated->isTileInvalidated(*pNear));
  CHECK(!pActivated->isTileInvalidated(*pFar));
}

TEST_CASE("GeoJsonDocumentRasterOverlay setDefaultStyle re-rasterizes "
          "geometry that uses the default style") {
  AsyncSystem asyncSystem(
      std::make_shared<CesiumNativeTests::SimpleTaskProcessor>());
  std::shared_ptr<GeoJsonDocument> pDocument = createLinesDocument({0.0, 50.0});
  // The second line has its own style, so the default doesn't apply to it.
  getFeature(*pDocument, 1).style = createLineStyle(BLUE);

  IntrusivePointer<GeoJsonDocumentRasterOverlay> pOverlay =
      new GeoJsonDocumentRasterOverlay(
          asyncSystem,
          "overlay",
          pDocument,
          defaultOptions);
  IntrusivePointer<ActivatedRasterOverlay> pActivated =
      activateOverlay(asyncSystem, *pOverlay);

  IntrusivePointer<RasterOverlayTile> pTileA =
      loadTile(asyncSystem, *pActivated, aroundLine(0.0));
  IntrusivePointer<RasterOverlayTile> pTileB =
      loadTile(asyncSystem, *pActivated, aroundLine(50.0));
  CHECK(imageHasColor(*pTileA->getImage(), RED));

  pOverlay->setDefaultStyle(createLineStyle(GREEN));
  CHECK(pOverlay->getDefaultStyle() == createLineStyle(GREEN));
  update(asyncSystem, *pActivated);

  CHECK(pActivated->isTileInvalidated(*pTileA));
  CHECK(!pActivated->isTileInvalidated(*pTileB));

  IntrusivePointer<RasterOverlayTile> pTileA2 =
      loadTile(asyncSystem, *pActivated, aroundLine(0.0));
  CHECK(imageHasColor(*pTileA2->getImage(), GREEN));
  CHECK_FALSE(imageHasColor(*pTileA2->getImage(), RED));
}

TEST_CASE("GeoJsonDocumentRasterOverlay setDocument re-rasterizes the old and "
          "new geometry") {
  AsyncSystem asyncSystem(
      std::make_shared<CesiumNativeTests::SimpleTaskProcessor>());
  IntrusivePointer<GeoJsonDocumentRasterOverlay> pOverlay =
      new GeoJsonDocumentRasterOverlay(
          asyncSystem,
          "overlay",
          createLinesDocument({0.0}),
          defaultOptions);
  IntrusivePointer<ActivatedRasterOverlay> pActivated =
      activateOverlay(asyncSystem, *pOverlay);

  IntrusivePointer<RasterOverlayTile> pOld =
      loadTile(asyncSystem, *pActivated, aroundLine(0.0));
  IntrusivePointer<RasterOverlayTile> pNew =
      loadTile(asyncSystem, *pActivated, aroundLine(50.0));
  IntrusivePointer<RasterOverlayTile> pElsewhere =
      loadTile(asyncSystem, *pActivated, aroundLine(100.0));
  CHECK(imageHasColor(*pOld->getImage(), RED));
  CHECK_FALSE(imageHasColor(*pNew->getImage(), RED));

  pOverlay->setDocument(createLinesDocument({50.0}));
  update(asyncSystem, *pActivated);

  CHECK(pActivated->isTileInvalidated(*pOld));
  CHECK(pActivated->isTileInvalidated(*pNew));
  CHECK(!pActivated->isTileInvalidated(*pElsewhere));

  IntrusivePointer<RasterOverlayTile> pOld2 =
      loadTile(asyncSystem, *pActivated, aroundLine(0.0));
  IntrusivePointer<RasterOverlayTile> pNew2 =
      loadTile(asyncSystem, *pActivated, aroundLine(50.0));
  CHECK_FALSE(imageHasColor(*pOld2->getImage(), RED));
  CHECK(imageHasColor(*pNew2->getImage(), RED));
}

TEST_CASE("GeoJsonDocumentRasterOverlay can be activated more than once") {
  AsyncSystem asyncSystem(
      std::make_shared<CesiumNativeTests::SimpleTaskProcessor>());
  IntrusivePointer<GeoJsonDocumentRasterOverlay> pOverlay =
      new GeoJsonDocumentRasterOverlay(
          asyncSystem,
          "overlay",
          createLinesDocument({0.0}),
          defaultOptions);

  SUBCASE("at the same time, as when added to two tilesets") {
    IntrusivePointer<ActivatedRasterOverlay> pFirst =
        activateOverlay(asyncSystem, *pOverlay);
    IntrusivePointer<ActivatedRasterOverlay> pSecond =
        activateOverlay(asyncSystem, *pOverlay);

    IntrusivePointer<RasterOverlayTile> pFirstTile =
        loadTile(asyncSystem, *pFirst, aroundLine(0.0));
    IntrusivePointer<RasterOverlayTile> pSecondTile =
        loadTile(asyncSystem, *pSecond, aroundLine(0.0));
    CHECK(imageHasColor(*pFirstTile->getImage(), RED));
    CHECK(imageHasColor(*pSecondTile->getImage(), RED));

    // A change to the overlay reaches both.
    pOverlay->setDefaultStyle(createLineStyle(GREEN));
    update(asyncSystem, *pFirst);
    update(asyncSystem, *pSecond);
    CHECK(pFirst->isTileInvalidated(*pFirstTile));
    CHECK(pSecond->isTileInvalidated(*pSecondTile));
  }

  SUBCASE("one after another, as when removed and added again") {
    {
      IntrusivePointer<ActivatedRasterOverlay> pFirst =
          activateOverlay(asyncSystem, *pOverlay);
      IntrusivePointer<RasterOverlayTile> pTile =
          loadTile(asyncSystem, *pFirst, aroundLine(0.0));
      CHECK(imageHasColor(*pTile->getImage(), RED));
    }

    IntrusivePointer<ActivatedRasterOverlay> pSecond =
        activateOverlay(asyncSystem, *pOverlay);
    IntrusivePointer<RasterOverlayTile> pTile =
        loadTile(asyncSystem, *pSecond, aroundLine(0.0));
    CHECK(imageHasColor(*pTile->getImage(), RED));
  }
}

TEST_CASE("GeoJsonDocumentRasterOverlay renders a document with no geometry") {
  AsyncSystem asyncSystem(
      std::make_shared<CesiumNativeTests::SimpleTaskProcessor>());
  IntrusivePointer<GeoJsonDocumentRasterOverlay> pOverlay =
      new GeoJsonDocumentRasterOverlay(
          asyncSystem,
          "overlay",
          createLinesDocument({}),
          defaultOptions);
  IntrusivePointer<ActivatedRasterOverlay> pActivated =
      activateOverlay(asyncSystem, *pOverlay);

  IntrusivePointer<RasterOverlayTile> pTile =
      loadTile(asyncSystem, *pActivated, aroundLine(0.0));
  CHECK_FALSE(imageHasColor(*pTile->getImage(), RED));

  // Geometry added later shows up.
  pOverlay->setDocument(createLinesDocument({0.0}));
  update(asyncSystem, *pActivated);
  CHECK(pActivated->isTileInvalidated(*pTile));
  IntrusivePointer<RasterOverlayTile> pTile2 =
      loadTile(asyncSystem, *pActivated, aroundLine(0.0));
  CHECK(imageHasColor(*pTile2->getImage(), RED));
}
