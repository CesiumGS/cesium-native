#include "Cesium3DTilesSelection/QuadtreeRasterOverlayTileProvider.h"
#include "Cesium3DTilesSelection/RasterOverlay.h"
#include "SimpleAssetAccessor.h"

#include <CesiumGeospatial/WebMercatorProjection.h>

#include <catch2/catch.hpp>

using namespace Cesium3DTilesSelection;
using namespace CesiumAsync;
using namespace CesiumGeometry;
using namespace CesiumGeospatial;
using namespace CesiumGltf;
using namespace CesiumUtility;

namespace {

class TestTileProvider : public QuadtreeRasterOverlayTileProvider {
public:
  TestTileProvider(
      const IntrusivePointer<const RasterOverlay>& pOwner,
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      std::optional<Credit> credit,
      const std::shared_ptr<IPrepareRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      const CesiumGeospatial::Projection& projection,
      const CesiumGeometry::QuadtreeTilingScheme& tilingScheme,
      const CesiumGeometry::Rectangle& coverageRectangle,
      uint32_t minimumLevel,
      uint32_t maximumLevel,
      uint32_t imageWidth,
      uint32_t imageHeight) noexcept
      : QuadtreeRasterOverlayTileProvider(
            pOwner,
            asyncSystem,
            pAssetAccessor,
            credit,
            pPrepareRendererResources,
            pLogger,
            projection,
            tilingScheme,
            coverageRectangle,
            minimumLevel,
            maximumLevel,
            imageWidth,
            imageHeight) {}

  // The tiles that will return an error from loadQuadtreeTileImage.
  std::vector<QuadtreeTileID> errorTiles;

  virtual CesiumAsync::Future<LoadedRasterOverlayImage>
  loadQuadtreeTileImage(const QuadtreeTileID& tileID) const {
    LoadedRasterOverlayImage result;
    result.rectangle = this->getTilingScheme().tileToRectangle(tileID);

    if (std::find(errorTiles.begin(), errorTiles.end(), tileID) !=
        errorTiles.end()) {
      result.errors.emplace_back("Tile errored.");
    } else {
      // Return an image where every component of every pixel is equal to the
      // tile level.
      result.image.emplace();
      result.image->width = int32_t(this->getWidth());
      result.image->height = int32_t(this->getHeight());
      result.image->bytesPerChannel = 1;
      result.image->channels = 4;
      result.image->pixelData.resize(
          this->getWidth() * this->getHeight() * 4,
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
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CreditSystem>& /* pCreditSystem */,
      const std::shared_ptr<IPrepareRendererResources>&
          pPrepareRendererResources,
      const std::shared_ptr<spdlog::logger>& pLogger,
      CesiumUtility::IntrusivePointer<const RasterOverlay> pOwner)
      const override {
    if (!pOwner) {
      pOwner = this;
    }

    return asyncSystem.createResolvedFuture<CreateTileProviderResult>(
        new TestTileProvider(
            pOwner,
            asyncSystem,
            pAssetAccessor,
            std::nullopt,
            pPrepareRendererResources,
            pLogger,
            WebMercatorProjection(),
            QuadtreeTilingScheme(
                WebMercatorProjection::computeMaximumProjectedRectangle(),
                1,
                1),
            WebMercatorProjection::computeMaximumProjectedRectangle(),
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

  IntrusivePointer<RasterOverlayTileProvider> pProvider = nullptr;

  pOverlay
      ->createTileProvider(
          asyncSystem,
          pAssetAccessor,
          nullptr,
          nullptr,
          spdlog::default_logger(),
          nullptr)
      .thenInMainThread(
          [&pProvider](RasterOverlay::CreateTileProviderResult&& created) {
            CHECK(created);
            pProvider = *created;
          });

  asyncSystem.dispatchMainThreadTasks();

  REQUIRE(pProvider);
  REQUIRE(!pProvider->isPlaceholder());

  SECTION("uses root tile for a large area") {
    Rectangle rectangle =
        GeographicProjection::computeMaximumProjectedRectangle();
    IntrusivePointer<RasterOverlayTile> pTile =
        pProvider->getTile(rectangle, glm::dvec2(256));
    pProvider->loadTile(*pTile);

    for (int i = 0; pTile->getState() != RasterOverlayTile::LoadState::Loaded;
         ++i) {
      asyncSystem.dispatchMainThreadTasks();
    }

    CHECK(pTile->getState() == RasterOverlayTile::LoadState::Loaded);

    const ImageCesium& image = pTile->getImage();
    CHECK(image.width > 0);
    CHECK(image.height > 0);
    CHECK(image.pixelData.size() > 0);
    CHECK(std::all_of(
        image.pixelData.begin(),
        image.pixelData.end(),
        [](std::byte b) { return b == std::byte(0); }));
  }

  SECTION("uses a mix of levels when a tile returns an error") {
    glm::dvec2 center(0.1, 0.2);

    TestTileProvider* pTestProvider =
        static_cast<TestTileProvider*>(pProvider.get());

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
        pProvider->getTile(tileRectangle, targetScreenPixels);
    pProvider->loadTile(*pTile);

    for (int i = 0; pTile->getState() != RasterOverlayTile::LoadState::Loaded;
         ++i) {
      asyncSystem.dispatchMainThreadTasks();
    }

    CHECK(pTile->getState() == RasterOverlayTile::LoadState::Loaded);

    const ImageCesium& image = pTile->getImage();
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
}
