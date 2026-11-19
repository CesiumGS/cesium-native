#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/ITaskProcessor.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeometry/Rectangle.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GeographicProjection.h>
#include <CesiumGeospatial/Projection.h>
#include <CesiumGeospatial/WebMercatorProjection.h>
#include <CesiumImage/ImageAsset.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumRasterOverlays/ActivatedRasterOverlay.h>
#include <CesiumRasterOverlays/CreateRasterOverlayTileProviderParameters.h>
#include <CesiumRasterOverlays/QuadtreeRasterOverlayTileProvider.h>
#include <CesiumRasterOverlays/RasterOverlay.h>
#include <CesiumRasterOverlays/RasterOverlayTile.h>
#include <CesiumRasterOverlays/RasterOverlayTileProvider.h>
#include <CesiumUtility/IntrusivePointer.h>

#include <doctest/doctest.h>
#include <glm/ext/vector_double2.hpp>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumImage;
using namespace CesiumUtility;
using namespace CesiumNativeTests;
using namespace CesiumRasterOverlays;

namespace {

class TestTileProvider : public QuadtreeRasterOverlayTileProvider {
public:
  TestTileProvider(
      const IntrusivePointer<const RasterOverlay>& pCreator,
      const CreateRasterOverlayTileProviderParameters& parameters,
      const CesiumGeospatial::Projection& projection,
      const CesiumGeometry::QuadtreeTilingScheme& tilingScheme,
      const CesiumGeometry::Rectangle& coverageRectangle,
      uint32_t minimumLevel,
      uint32_t maximumLevel,
      uint32_t imageWidth,
      uint32_t imageHeight) noexcept
      : QuadtreeRasterOverlayTileProvider(
            pCreator,
            parameters,
            projection,
            tilingScheme,
            coverageRectangle,
            minimumLevel,
            maximumLevel,
            imageWidth,
            imageHeight) {}

  // The tiles that will return an error from loadQuadtreeTileImage.
  std::vector<QuadtreeTileID> errorTiles;

  // Tiles that will return an image of a size other than the declared one.
  struct TileImageSize {
    QuadtreeTileID tileID;
    int32_t width;
    int32_t height;
  };
  std::vector<TileImageSize> tileImageSizes;

  virtual CesiumAsync::Future<LoadedRasterOverlayImage>
  loadQuadtreeTileImage(const QuadtreeTileID& tileID) const {
    LoadedRasterOverlayImage result;
    result.rectangle = this->getTilingScheme().tileToRectangle(tileID);

    if (std::find(errorTiles.begin(), errorTiles.end(), tileID) !=
        errorTiles.end()) {
      result.errorList.emplaceError("Tile errored.");
    } else {
      int32_t width = int32_t(this->getWidth());
      int32_t height = int32_t(this->getHeight());

      auto sizeIt = std::find_if(
          this->tileImageSizes.begin(),
          this->tileImageSizes.end(),
          [&tileID](const TileImageSize& size) {
            return size.tileID == tileID;
          });
      if (sizeIt != this->tileImageSizes.end()) {
        width = sizeIt->width;
        height = sizeIt->height;
      }

      // Return an image where every component of every pixel is equal to the
      // tile level.
      result.pImage.emplace();
      result.pImage->width = width;
      result.pImage->height = height;
      result.pImage->bytesPerChannel = 1;
      result.pImage->channels = 4;
      result.pImage->pixelData.resize(
          size_t(width) * size_t(height) * 4,
          std::byte(tileID.level));
    }

    return this->getAsyncSystem().createResolvedFuture(std::move(result));
  }
};

class TestRasterOverlay : public RasterOverlay {
public:
  TestRasterOverlay(
      const std::string& name,
      const RasterOverlayOptions& options = RasterOverlayOptions())
      : RasterOverlay(name, options) {}

  virtual CesiumAsync::Future<CreateTileProviderResult> createTileProvider(
      const CreateRasterOverlayTileProviderParameters& parameters)
      const override {
    return parameters.externals.asyncSystem
        .createResolvedFuture<CreateTileProviderResult>(new TestTileProvider(
            this,
            parameters,
            WebMercatorProjection(Ellipsoid::WGS84),
            QuadtreeTilingScheme(
                WebMercatorProjection::computeMaximumProjectedRectangle(
                    Ellipsoid::WGS84),
                1,
                1),
            WebMercatorProjection::computeMaximumProjectedRectangle(
                Ellipsoid::WGS84),
            0,
            10,
            256,
            256));
  }
};

class MockTaskProcessor : public ITaskProcessor {
public:
  virtual void startTask(std::function<void()> f) { std::thread(f).detach(); }
};

} // namespace

TEST_CASE("QuadtreeRasterOverlayTileProvider getTile") {
  auto pTaskProcessor = std::make_shared<MockTaskProcessor>();
  auto pAssetAccessor = std::make_shared<SimpleAssetAccessor>(
      std::map<std::string, std::shared_ptr<SimpleAssetRequest>>());

  AsyncSystem asyncSystem(pTaskProcessor);
  IntrusivePointer<TestRasterOverlay> pOverlay = new TestRasterOverlay("Test");

  IntrusivePointer<ActivatedRasterOverlay> pActivated = pOverlay->activate(
      RasterOverlayExternals{
          pAssetAccessor,
          nullptr,
          asyncSystem,
          nullptr,
          spdlog::default_logger()},
      Ellipsoid::WGS84);

  asyncSystem.dispatchMainThreadTasks();

  REQUIRE(pActivated->getTileProvider() != nullptr);

  SUBCASE("uses root tile for a large area") {
    Rectangle rectangle =
        GeographicProjection::computeMaximumProjectedRectangle(
            Ellipsoid::WGS84);
    IntrusivePointer<RasterOverlayTile> pTile =
        pActivated->getTile(rectangle, glm::dvec2(256));
    pActivated->loadTile(*pTile);

    while (pTile->getState() != RasterOverlayTile::LoadState::Loaded) {
      asyncSystem.dispatchMainThreadTasks();
    }

    CHECK(pTile->getState() == RasterOverlayTile::LoadState::Loaded);

    REQUIRE(pTile->getImage());

    const ImageAsset& image = *pTile->getImage();
    CHECK(image.width > 0);
    CHECK(image.height > 0);
    CHECK(image.pixelData.size() > 0);
    CHECK(std::all_of(
        image.pixelData.begin(),
        image.pixelData.end(),
        [](std::byte b) { return b == std::byte(0); }));
  }

  SUBCASE("uses a mix of levels when a tile returns an error") {
    glm::dvec2 center(0.1, 0.2);

    TestTileProvider* pTestProvider =
        static_cast<TestTileProvider*>(pActivated->getTileProvider());

    // Select a rectangle that spans four tiles at tile level 8.
    const uint32_t expectedLevel = 8;
    std::optional<QuadtreeTileID> centerTileID =
        pTestProvider->getTilingScheme().positionToTile(center, expectedLevel);
    REQUIRE(centerTileID);

    Rectangle centerRectangle =
        pTestProvider->getTilingScheme().tileToRectangle(*centerTileID);
    Rectangle tileRectangle(
        centerRectangle.minimumX - centerRectangle.computeWidth() * 0.5,
        centerRectangle.minimumY - centerRectangle.computeHeight() * 0.5,
        centerRectangle.maximumX + centerRectangle.computeWidth() * 0.5,
        centerRectangle.maximumY + centerRectangle.computeHeight() * 0.5);

    uint32_t rasterSSE = 2;
    glm::dvec2 targetScreenPixels = glm::dvec2(
        pTestProvider->getWidth() * 2 * rasterSSE,
        pTestProvider->getHeight() * 2 * rasterSSE);

    // The tile in the southeast corner will fail to load.
    std::optional<QuadtreeTileID> southeastID =
        pTestProvider->getTilingScheme().positionToTile(
            tileRectangle.getLowerRight(),
            expectedLevel);
    REQUIRE(southeastID);

    pTestProvider->errorTiles.emplace_back(*southeastID);

    IntrusivePointer<RasterOverlayTile> pTile =
        pActivated->getTile(tileRectangle, targetScreenPixels);
    pActivated->loadTile(*pTile);

    while (pTile->getState() != RasterOverlayTile::LoadState::Loaded) {
      asyncSystem.dispatchMainThreadTasks();
    }

    CHECK(pTile->getState() == RasterOverlayTile::LoadState::Loaded);

    REQUIRE(pTile->getImage());

    const ImageAsset& image = *pTile->getImage();
    CHECK(image.width > 0);
    CHECK(image.height > 0);
    CHECK(image.pixelData.size() > 0);

    // We should have pixels from both level 7 and level 8.
    CHECK(std::all_of(
        image.pixelData.begin(),
        image.pixelData.end(),
        [](std::byte b) { return b == std::byte(7) || b == std::byte(8); }));
    CHECK(std::any_of(
        image.pixelData.begin(),
        image.pixelData.end(),
        [](std::byte b) { return b == std::byte(7); }));
    CHECK(std::any_of(
        image.pixelData.begin(),
        image.pixelData.end(),
        [](std::byte b) { return b == std::byte(8); }));
  }

  SUBCASE("bounds the combined image when tiles report inconsistent "
          "dimensions") {
    glm::dvec2 center(0.1, 0.2);

    TestTileProvider* pTestProvider =
        static_cast<TestTileProvider*>(pActivated->getTileProvider());

    // Select a rectangle that spans four tiles at tile level 8.
    uint32_t expectedLevel = 8;
    std::optional<QuadtreeTileID> centerTileID =
        pTestProvider->getTilingScheme().positionToTile(center, expectedLevel);
    REQUIRE(centerTileID);

    Rectangle centerRectangle =
        pTestProvider->getTilingScheme().tileToRectangle(*centerTileID);
    Rectangle tileRectangle(
        centerRectangle.minimumX - centerRectangle.computeWidth() * 0.5,
        centerRectangle.minimumY - centerRectangle.computeHeight() * 0.5,
        centerRectangle.maximumX + centerRectangle.computeWidth() * 0.5,
        centerRectangle.maximumY + centerRectangle.computeHeight() * 0.5);

    uint32_t rasterSSE = 2;
    glm::dvec2 targetScreenPixels = glm::dvec2(
        pTestProvider->getWidth() * 2 * rasterSSE,
        pTestProvider->getHeight() * 2 * rasterSSE);

    // A very wide tile and a very tall tile set the two resolutions apart.
    std::optional<QuadtreeTileID> southwestID =
        pTestProvider->getTilingScheme().positionToTile(
            tileRectangle.getLowerLeft(),
            expectedLevel);
    REQUIRE(southwestID);

    std::optional<QuadtreeTileID> northeastID =
        pTestProvider->getTilingScheme().positionToTile(
            tileRectangle.getUpperRight(),
            expectedLevel);
    REQUIRE(northeastID);

    pTestProvider->tileImageSizes.push_back({*southwestID, 32768, 1});
    pTestProvider->tileImageSizes.push_back({*northeastID, 1, 32769});

    IntrusivePointer<RasterOverlayTile> pTile =
        pActivated->getTile(tileRectangle, targetScreenPixels);
    pActivated->loadTile(*pTile);

    while (pTile->getState() != RasterOverlayTile::LoadState::Loaded) {
      asyncSystem.dispatchMainThreadTasks();
    }

    REQUIRE(pTile->getImage());

    int32_t maximumTextureSize = pOverlay->getOptions().maximumTextureSize;

    const ImageAsset& image = *pTile->getImage();
    size_t expectedBytes = size_t(image.width) * size_t(image.height) *
                           size_t(image.channels) *
                           size_t(image.bytesPerChannel);
    CHECK(image.width > 0);
    CHECK(image.height > 0);
    CHECK(image.width <= maximumTextureSize);
    CHECK(image.height <= maximumTextureSize);
    CHECK(image.pixelData.size() == expectedBytes);
  }
}
